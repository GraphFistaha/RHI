#include "RenderPass.hpp"

#include "../CommandsExecution/Submitter.hpp"
#include "../Utils/Builders.hpp"
#include "RenderTarget.hpp"
#include "Subpass.hpp"

namespace RHI::vulkan
{

RenderPass::RenderPass(const Context & ctx)
  : m_context(ctx)
  , m_builder(new details::RenderPassBuilder())
{
  std::tie(m_graphicsQueueFamily, m_graphicsQueue) = ctx.GetQueue(QueueType::Graphics);
  m_submitter = std::make_unique<details::Submitter>(ctx, m_graphicsQueue, m_graphicsQueueFamily);
}

RenderPass::~RenderPass()
{
  if (!!m_renderPass)
    vkDestroyRenderPass(m_context.GetDevice(), m_renderPass, nullptr);
}

ISubpass * RenderPass::CreateSubpass()
{
  return &m_subpasses.emplace_back(m_context, *this, m_subpasses.size(), m_graphicsQueueFamily);
}

VkSemaphore RenderPass::Draw(VkSemaphore imageAvailiableSemaphore)
{
  assert(m_renderPass);
  assert(m_boundRenderTarget != nullptr);
  m_submitter->BeginWrite();

  VkFramebuffer buf = m_boundRenderTarget ? m_boundRenderTarget->GetHandle() : VK_NULL_HANDLE;
  VkExtent2D extent = m_boundRenderTarget->GetVkExtent();
  VkClearValue clearValue = m_boundRenderTarget->GetClearValue();

  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = m_renderPass;
  renderPassInfo.framebuffer = buf;
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = extent;
  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = &clearValue;

  vkCmdBeginRenderPass(m_submitter->GetCommandBuffer(), &renderPassInfo,
                       VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

  // execute commands for subpasses
  {
    std::vector<VkCommandBuffer> subpassBuffers;
    for (auto && subpass : m_subpasses)
    {
      subpass.LockWriting(true);
      if (!subpass.HasNoCommands() && subpass.IsEnabled())
        subpassBuffers.push_back(subpass.GetCommandBuffer().GetHandle());
    }
    if (!subpassBuffers.empty())
      vkCmdExecuteCommands(m_submitter->GetCommandBuffer(), subpassBuffers.size(),
                           subpassBuffers.data());
    for (auto && subpass : m_subpasses)
    {
      subpass.LockWriting(false);
    }
  }


  vkCmdEndRenderPass(m_submitter->GetCommandBuffer());
  m_submitter->EndWrite();
  auto res = m_submitter->Submit({imageAvailiableSemaphore});
  m_boundRenderTarget = nullptr;
  UpdateRenderingReadyFlag();
  return res;
}

void RenderPass::BindRenderTarget(const RenderTarget * renderTarget) noexcept
{
  if (!renderTarget)
  {
    m_cachedAttachments.clear();
    m_invalidRenderPass = true;
    return;
  }

  VkFramebuffer fbHandle = renderTarget->GetHandle();
  if (m_cachedAttachments != renderTarget->GetAttachments())
  {
    m_builder->Reset();
    auto && fboAttachments = renderTarget->GetAttachments();
    uint32_t attachmentIndex = 0;
    for (auto && attachment : fboAttachments)
    {
      VkAttachmentDescription renderPassAttachment{};
      renderPassAttachment.format = attachment.GetImageFormat();
      renderPassAttachment.samples = attachment.GetSamplesCount(); // MSAA
      // action for color/depth buffers in pass begins
      renderPassAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      // action for color/depth buffers in pass ends
      renderPassAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      renderPassAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // action
      renderPassAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      renderPassAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      renderPassAttachment.finalLayout = attachment.GetImageLayout();

      m_builder->AddAttachment(renderPassAttachment);
      m_builder->AddSubpass(
        {{ShaderImageSlot::Color /*TODO: it depends on attachment*/, attachmentIndex++}});
    }
    m_cachedAttachments = renderTarget->GetAttachments();
    m_invalidRenderPass = true;
  }
  m_boundRenderTarget = renderTarget;
  UpdateRenderingReadyFlag();
}

void RenderPass::Invalidate()
{
  if (m_invalidRenderPass || !m_renderPass)
  {
    auto new_renderpass = m_builder->Make(m_context.GetDevice());
    m_context.Log(RHI::LogMessageStatus::LOG_DEBUG, "build new VkRenderPass");
    m_context.WaitForIdle();
    if (!!m_renderPass)
      vkDestroyRenderPass(m_context.GetDevice(), m_renderPass, nullptr);
    else
      UpdateRenderingReadyFlag();
    m_renderPass = new_renderpass;
    m_invalidRenderPass = false;
  }
}

void RenderPass::WaitForReadyToRendering() const noexcept
{
  std::atomic_wait(&m_isReadyForRendering, false);
}

void RenderPass::UpdateRenderingReadyFlag() noexcept
{
  m_isReadyForRendering = m_renderPass && m_boundRenderTarget;
  std::atomic_notify_all(&m_isReadyForRendering);
}

} // namespace RHI::vulkan
