#include "RenderPass.hpp"

#include <CommandsExecution/Submitter.hpp>
#include <RenderPass/Framebuffer.hpp>
#include <RenderPass/RenderTarget.hpp>
#include <RenderPass/Subpass.hpp>
#include <VulkanContext.hpp>

namespace RHI::vulkan
{

RenderPass::RenderPass(Context & ctx, Framebuffer & framebuffer)
  : OwnedBy<Context>(ctx)
  , OwnedBy<Framebuffer>(framebuffer)
  , m_submitter(ctx, QueueType::Graphics, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT)
{
  auto [family, _] = ctx.GetGpuConnection().GetQueue(QueueType::Graphics);
  // Создает начальный subpass. У RenderPass всегда должен быть subpass,
  // иначе VkRenderPass не создастся и в целом все сломается.
  auto && initialSubpass = m_subpasses.emplace_back(GetContext(), *this, 0, family);
  //TODO: нужен фиктивный шейдер для initialSubpass, иначе не работает HelloWindow
}

RenderPass::~RenderPass()
{
  GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_renderPass, nullptr);
}

ISubpass * RenderPass::CreateSubpass()
{
  if (m_createSubpassCallsCounter++ == 0)
    return &m_subpasses.front();

  auto [family, _] = GetContext().GetGpuConnection().GetQueue(QueueType::Graphics);
  auto && subpass = m_subpasses.emplace_back(GetContext(), *this,
                                             static_cast<uint32_t>(m_subpasses.size()), family);
  m_invalidRenderPass = true;
  return &subpass;
}

void RenderPass::DeleteSubpass(ISubpass * subpass)
{
  size_t c = std::erase_if(m_subpasses, [subpass](const Subpass & sp) { return &sp == subpass; });
  if (c > 0)
    m_invalidRenderPass = true;
}

AsyncTask * RenderPass::Draw(RenderTarget & renderTarget,
                             std::vector<VkSemaphore> && waitSemaphores)
{
  assert(m_renderPass);
  assert(renderTarget.GetAttachmentsCount() == m_cachedAttachments.size());
  VkFramebuffer buf = renderTarget.GetHandle();
  VkExtent3D extent = renderTarget.GetVkExtent();
  auto && clearValues = renderTarget.GetClearValues();

  m_submitter.WaitForSubmitCompleted();
  m_submitter.BeginWriting();

  // here transfer layouts  for subpasses
  for (auto && subpass : m_subpasses)
  {
    subpass.TransitLayoutForUsedImages(m_submitter);
  }


  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = m_renderPass;
  renderPassInfo.framebuffer = buf;
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = {extent.width, extent.height};
  renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
  renderPassInfo.pClearValues = clearValues.data();

  m_submitter.PushCommand(vkCmdBeginRenderPass, &renderPassInfo,
                          VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

  GetFramebuffer().ForEachAttachment(
    [it = m_cachedAttachments.begin()](IInternalAttachment * att) mutable
    {
      if (att)
        att->TransferLayout(it->initialLayout);
      ++it;
    });

  // execute commands for subpasses
  {
    std::vector<VkCommandBuffer> subpassBuffers;
    subpassBuffers.reserve(m_subpasses.size());
    for (auto && subpass : m_subpasses)
    {
      if (subpass.ShouldSwapCommandBuffers())
        subpass.SwapCommandBuffers();
      if (subpass.IsEnabled())
        subpassBuffers.push_back(subpass.GetCommandBufferForExecution().GetHandle());
    }
    if (!subpassBuffers.empty())
      m_submitter.AddCommands(subpassBuffers);
  }


  m_submitter.PushCommand(vkCmdEndRenderPass);

  GetFramebuffer().ForEachAttachment(
    [it = m_cachedAttachments.begin()](IInternalAttachment * att) mutable
    {
      if (att)
        att->TransferLayout(it->finalLayout);
      ++it;
    });

  m_submitter.EndWriting();
  auto res = m_submitter.Submit(false /*waitPrevSubmitOnGPU*/, std::move(waitSemaphores));
  return res;
}

void RenderPass::SetAttachments(const std::vector<VkAttachmentDescription> & attachments) noexcept
{
  if (m_cachedAttachments != attachments)
  {
    m_cachedAttachments = attachments;
    m_invalidRenderPass = true;
  }
}

const VkAttachmentDescription & RenderPass::GetAttachmentDescription(uint32_t idx) const & noexcept
{
  return m_cachedAttachments[idx];
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
      m_builder.AddAttachment(attachment);
    for (auto && subpass : m_subpasses)
      m_builder.AddSubpass(subpass.GetLayout().BuildDescription());
    auto new_renderpass = m_builder.Make(GetContext().GetGpuConnection().GetDevice());
    GetContext().Log(RHI::LogMessageStatus::LOG_DEBUG, "build new VkRenderPass");
    GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_renderPass, nullptr);
    m_renderPass = new_renderpass;
    UpdateRenderPassValidFlag();
    m_invalidRenderPass = false;
  }

  for (auto && subpass : m_subpasses)
  {
    subpass.Invalidate();
  }
}

void RenderPass::SetInvalid()
{
  m_cachedAttachments.clear();
  m_invalidRenderPass = true;
}

void RenderPass::WaitForRenderPassIsValid() const noexcept
{
  std::atomic_wait(&m_isReadyForRendering, false);
}

void RenderPass::UpdateRenderPassValidFlag() noexcept
{
  m_isReadyForRendering = m_renderPass; // m_renderPass != VK_NULL_HANDLE
  std::atomic_notify_all(&m_isReadyForRendering);
}

void RenderPass::WaitForRenderingIsDone() noexcept
{
  m_submitter.WaitForSubmitCompleted();
}

} // namespace RHI::vulkan
