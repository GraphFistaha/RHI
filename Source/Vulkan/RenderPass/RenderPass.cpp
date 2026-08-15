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
// , m_submitter(ctx, )
{
  // —оздает начальный subpass. ” RenderPass всегда должен быть subpass,
  // иначе VkRenderPass не создастс€ и в целом все сломаетс€.
  auto && initialSubpass = m_subpasses.emplace_back(GetContext(), *this, 0);
  // disable subpass to not build pipeline
  //initialSubpass.SetEnabled(false);
}

RenderPass::~RenderPass()
{
  GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_renderPass, nullptr);
}

SubpassConfiguration * RenderPass::CreateSubpass()
{
  if (m_createSubpassCallsCounter++ == 0)
  {
    //m_subpasses.front().SetEnabled(true);
    return &m_subpasses.front();
  }

  auto && subpass = m_subpasses.emplace_back(GetContext(), *this,
                                             static_cast<uint32_t>(m_subpasses.size()));
  m_invalidRenderPass = true;
  return &subpass;
}

void RenderPass::DeleteSubpass(SubpassConfiguration * subpass)
{
  size_t c = std::erase_if(m_subpasses,
                           [subpass](const SubpassConfiguration & sp) { return &sp == subpass; });
  if (c > 0)
    m_invalidRenderPass = true;
}

void RenderPass::RecordCommands(details::CommandBuffer & commands, RenderTarget & renderTarget)
{
  assert(m_renderPass);
  assert(renderTarget.GetAttachmentsCount() == m_cachedAttachments.size());
  VkFramebuffer buf = renderTarget.GetHandle();
  VkExtent3D extent = renderTarget.GetVkExtent();
  auto && clearValues = renderTarget.GetClearValues();

  // here transfer layouts  for subpasses
  for (auto && subpass : m_subpasses)
  {
    subpass.GetInternal().SynchroniseResources(commands);
  }

  VkRenderPassBeginInfo renderPassInfo{};
  {
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_renderPass;
    renderPassInfo.framebuffer = buf;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = {extent.width, extent.height};
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();
  }

  commands.PushCommand(vkCmdBeginRenderPass, &renderPassInfo,
                       VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

  GetFramebuffer().ForEachAttachment(
    [it = m_cachedAttachments.begin()](IInternalAttachment * att) mutable
    {
      if (att)
        att->OnBeginRenderPass(it->initialLayout);
      ++it;
    });

  // execute commands for subpasses
  {
    size_t i = 0;
    for (auto && subpass : m_subpasses)
    {
      subpass.GetInternal().RecordCommands(commands);
      if (i + 1 != m_subpasses.size())
        commands.PushCommand(vkCmdNextSubpass, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
      ++i;
    }
  }


  commands.PushCommand(vkCmdEndRenderPass);

  GetFramebuffer().ForEachAttachment(
    [it = m_cachedAttachments.begin()](IInternalAttachment * att) mutable
    {
      if (att)
        att->OnEndRenderPass(it->finalLayout);
      ++it;
    });
}

void RenderPass::CollectResources(std::vector<ResourcePtr> & resources) const
{
  for (auto && subpass : m_subpasses)
  {
    subpass.GetInternal().CollectResources(resources);
  }
}

void RenderPass::SynchroniseResources(details::CommandBuffer & commands) const
{
  for (auto && subpass : m_subpasses)
  {
    subpass.GetInternal().SynchroniseResources(commands);
  }
}

void RenderPass::SetAttachments(uint32_t buffersCount,
                                const std::vector<VkAttachmentDescription> & attachments) noexcept
{
  if (m_cachedAttachments != attachments)
  {
    m_cachedAttachments = attachments;
    m_invalidRenderPass = true;
  }
  if (buffersCount != m_buffersCount)
  {
    m_buffersCount = buffersCount;
  }
}

const VkAttachmentDescription & RenderPass::GetAttachmentDescription(uint32_t idx) const & noexcept
{
  return m_cachedAttachments[idx];
}

void RenderPass::ForEachSubpass(std::function<void(SubpassConfiguration &)> && func)
{
  std::for_each(m_subpasses.begin(), m_subpasses.end(), std::move(func));
}

void RenderPass::Invalidate()
{
  bool clearCommands = false;
  if (m_invalidRenderPass || !m_renderPass)
  {
    m_builder.Reset();
    for (auto && attachment : m_cachedAttachments)
      m_builder.AddAttachment(attachment);
    for (auto && subpass : m_subpasses)
      m_builder.AddSubpass(subpass.GetInternal().GetLayout().BuildDescription());
    auto new_renderpass = m_builder.Make(GetContext().GetGpuConnection().GetDevice());
    GetContext().Log(RHI::LogMessageStatus::LOG_DEBUG, "VkRenderPass({}) has been rebuilt - {}",
                     static_cast<void *>(m_renderPass), static_cast<void *>(new_renderpass));
    GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_renderPass, nullptr);
    m_renderPass = new_renderpass;
    UpdateRenderPassValidFlag();
    m_invalidRenderPass = false;
    clearCommands = true;
  }

  for (auto && subpass : m_subpasses)
  {
    if (clearCommands)
      subpass.SetInvalid();
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

} // namespace RHI::vulkan
