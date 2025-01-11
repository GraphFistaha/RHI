#include "RenderPass.hpp"

#include "../CommandsExecution/Submitter.hpp"
#include "../VulkanContext.hpp"
#include "RenderTarget.hpp"
#include "Subpass.hpp"

namespace RHI::vulkan
{

RenderPass::RenderPass(const Context & ctx)
  : m_context(ctx)
  , m_graphicsQueueFamily(ctx.GetQueue(QueueType::Graphics).first)
  , m_graphicsQueue(ctx.GetQueue(QueueType::Graphics).second)
  , m_submitter(ctx, m_graphicsQueue, m_graphicsQueueFamily,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT)
{
}

RenderPass::~RenderPass()
{
  m_context.GetGarbageCollector().PushVkObjectToDestroy(m_renderPass, nullptr);
}

ISubpass * RenderPass::CreateSubpass()
{
  return &m_subpasses.emplace_back(m_context, *this, static_cast<uint32_t>(m_subpasses.size()),
                                   m_graphicsQueueFamily);
}

VkSemaphore RenderPass::Draw(VkSemaphore imageAvailiableSemaphore)
{
  assert(m_renderPass);
  assert(m_boundRenderTarget != nullptr);
  VkFramebuffer buf = m_boundRenderTarget ? m_boundRenderTarget->GetHandle() : VK_NULL_HANDLE;
  VkExtent2D extent = m_boundRenderTarget->GetVkExtent();
  VkClearValue clearValue = m_boundRenderTarget->GetClearValue();

  m_submitter.WaitForSubmitCompleted();
  m_submitter.BeginWriting();


  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = m_renderPass;
  renderPassInfo.framebuffer = buf;
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = extent;
  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = &clearValue;

  m_submitter.PushCommand(vkCmdBeginRenderPass, &renderPassInfo,
                          VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

  // execute commands for subpasses
  {
    std::vector<VkCommandBuffer> subpassBuffers;
    for (auto && subpass : m_subpasses)
    {
      subpass.LockWriting(true);
      if (subpass.IsEnabled())
        subpassBuffers.push_back(subpass.GetCommandBuffer().GetHandle());
    }
    if (!subpassBuffers.empty())
      m_submitter.AddCommands(subpassBuffers);

    for (auto && subpass : m_subpasses)
    {
      subpass.LockWriting(false);
    }
  }


  m_submitter.PushCommand(vkCmdEndRenderPass);
  m_submitter.EndWriting();
  auto res = m_submitter.Submit(false /*waitPrevSubmitOnGPU*/, {imageAvailiableSemaphore});
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

  auto && fboAttachments = renderTarget->GetAttachments();
  if (m_cachedAttachments != fboAttachments)
  {
    m_cachedAttachments = fboAttachments;
    m_invalidRenderPass = true;
  }
  m_boundRenderTarget = renderTarget;
  UpdateRenderingReadyFlag();
}

void RenderPass::ForEachSubpass(std::function<void(Subpass &)> && func)
{
  std::for_each(m_subpasses.begin(), m_subpasses.end(), std::move(func));
}

void RenderPass::Invalidate()
{
  if (m_invalidRenderPass || !m_renderPass)
  {
    m_builder.Reset();
    for (auto && attachment : m_cachedAttachments)
      m_builder.AddAttachment(attachment.BuildAttachmentDescription());
    for (auto && subpass : m_subpasses)
      m_builder.AddSubpass(subpass.GetLayout().BuildDescription());
    auto new_renderpass = m_builder.Make(m_context.GetDevice());
    m_context.Log(RHI::LogMessageStatus::LOG_DEBUG, "build new VkRenderPass");
    m_context.GetGarbageCollector().PushVkObjectToDestroy(m_renderPass, nullptr);
    m_renderPass = new_renderpass;
    UpdateRenderingReadyFlag();
    m_invalidRenderPass = false;
  }
}

void RenderPass::SetInvalid()
{
  m_cachedAttachments.clear();
  m_invalidRenderPass = true;
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
