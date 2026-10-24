#include "RenderPass.hpp"

#include <CommandsExecution/CommandBuffer.hpp>
#include <CommandsExecution/SubmitTask.hpp>
#include <Memory/BufferGPU.hpp>
#include <Pipeline/Pipeline.hpp>
#include <Private/FastDynamicCast.hpp>
#include <RenderPass/Framebuffer.hpp>
#include <RenderPass/SubpassGraph.hpp>
#include <RenderPass/SubpassInfo.hpp>
#include <utils/CastHelper.hpp>
#include <VulkanContext.hpp>


namespace RHI::vulkan
{

RenderPass::RenderPass(Context & ctx, FramebufferPtr framebuffer, PipelinePtr initialPipeline)
  : OwnedBy<Context>(ctx)
  , m_framebuffer(FastDynamicCast<Framebuffer>(framebuffer))
  , m_renderGraph(new SubpassGraph(ctx))
{
  m_subpasses.emplace_back(
    std::make_unique<SubpassInfo>(*this, FastDynamicCast<Pipeline>(initialPipeline)));
}

RenderPass::~RenderPass()
{
}

void RenderPass::Barrier(IBufferGPU & buffer)
{
  if (auto res = FastDynamicCast<IInternalBuffer>(&buffer))
    m_subpasses.back()->AddBarrier(*res);
}

void RenderPass::Barrier(ITexture & texture)
{
  if (auto res = FastDynamicCast<IInternalTexture>(&texture))
    m_subpasses.back()->AddBarrier(*res);
}

void RenderPass::Barrier(IAttachment & attachment)
{
  if (auto res = FastDynamicCast<IInternalAttachment>(&attachment))
    m_subpasses.back()->AddBarrier(*res);
}

void RenderPass::NextPipeline(PipelinePtr pipeline)
{
  m_subpasses.emplace_back(
    std::make_unique<SubpassInfo>(*this, FastDynamicCast<Pipeline>(pipeline)));
}

void RenderPass::ClearFramebuffer()
{
  m_loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
}

void RenderPass::LoadFramebuffer()
{
  m_loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
}

void RenderPass::StoreFramebuffer()
{
  m_storeOp = VK_ATTACHMENT_STORE_OP_STORE;
}

void RenderPass::ResetResourcesSynchronization()
{
  for (auto && subpass : m_subpasses)
  {
    subpass->ResetSynchronization();
  }
}

void RenderPass::RecordCommands(details::CommandBuffer & commands,
                                std::vector<VkSemaphore> & waitSemaphores)
{
  if (RenderTargetInfo renderTargetInfo = m_framebuffer->BeginFrame())
  {
    m_activeTarget = (m_activeTarget + 1) % m_targets.size();
    auto && renderTarget = m_targets[m_activeTarget];
    //AcquireForRendering can return random imageView set, so probably it could rebuild VkFramebuffer for each frame
    renderTarget.SetExtent(m_framebuffer->GetInternalExtent());
    renderTarget.SetAttachments(std::move(renderTargetInfo));
    renderTarget.RebuildFramebuffer(); // rebuilds VkFramebuffer if need it

    auto && imageSemaphores = renderTarget.GetImageAvailableForRenderSemaphores();
    waitSemaphores.insert(waitSemaphores.end(), imageSemaphores.begin(), imageSemaphores.end());

    const VkFramebuffer buf = renderTarget.GetHandle();
    const VkExtent3D extent = renderTarget.GetVkExtent();
    auto && clearValues = renderTarget.GetClearValues();

    // here must be buffer synchronization
    for (auto && subpass : m_subpasses)
    {
      subpass->SynchroniseResources(SynchronizationFilter::BufferOnly, commands);
    }

    VkRenderPassBeginInfo renderPassInfo{};
    {
      renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
      renderPassInfo.renderPass = m_renderGraph->GetHandle();
      renderPassInfo.framebuffer = buf;
      renderPassInfo.renderArea.offset = {0, 0};
      renderPassInfo.renderArea.extent = {extent.width, extent.height};
      renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
      renderPassInfo.pClearValues = clearValues.data();
    }
    commands.PushCommand(
      vkCmdBeginRenderPass, &renderPassInfo,
      VK_SUBPASS_CONTENTS_INLINE); //TODO: VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS
    m_renderGraph->SynchronizeAttachmentsDuringRenderPass(SubpassIndex::initialRenderPass,
                                                          m_framebuffer->GetAttachments());


    for (size_t i = 0; auto && subpass : m_subpasses)
    {
      subpass->RecordCommands(commands);
      m_renderGraph->SynchronizeAttachmentsDuringRenderPass(static_cast<SubpassIndex>(i),
                                                            m_framebuffer->GetAttachments());

      ++i;
      if (i < m_subpasses.size())
      {
        commands.PushCommand(vkCmdNextSubpass, m_subpasses[i]->IsInlineCommands()
                                                 ? VK_SUBPASS_CONTENTS_INLINE
                                                 : VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
      }
    }

    commands.PushCommand(vkCmdEndRenderPass);
    // probably it doesn't needed
    m_renderGraph->SynchronizeAttachmentsDuringRenderPass(SubpassIndex::finalRenderPass,
                                                          m_framebuffer->GetAttachments());
  }
}

void RenderPass::DrawVertices(std::uint32_t vertexCount, std::uint32_t instanceCount,
                              std::uint32_t firstVertex, std::uint32_t firstInstance)
{
  m_subpasses.back()->DrawVertices(vertexCount, instanceCount, firstVertex, firstInstance);
}

void RenderPass::DrawIndexedVertices(std::uint32_t indexCount, std::uint32_t instanceCount,
                                     std::uint32_t firstIndex, int32_t vertexOffset,
                                     std::uint32_t firstInstance)
{
  m_subpasses.back()->DrawIndexedVertices(indexCount, instanceCount, firstIndex, vertexOffset,
                                          firstInstance);
}

void RenderPass::SetViewport(float width, float height)
{
  m_subpasses.back()->SetViewport(width, height);
}

void RenderPass::SetScissor(int32_t x, int32_t y, std::uint32_t width, std::uint32_t height)
{
  m_subpasses.back()->SetScissor(x, y, width, height);
}

void RenderPass::BindVertexBuffer(std::uint32_t binding, IBufferGPU * buffer, std::uint32_t offset)
{
  m_subpasses.back()->BindVertexBuffer(binding, buffer, offset);
}

void RenderPass::BindIndexBuffer(IBufferGPU * buffer, IndexType type, std::uint32_t offset)
{
  m_subpasses.back()->BindIndexBuffer(buffer, type, offset);
}

void RenderPass::PushConstant(const void * data, size_t size)
{
  m_subpasses.back()->PushConstant(data, size);
}

VkRenderPass RenderPass::GetHandle() const noexcept
{
  return m_renderGraph ? m_renderGraph->GetHandle() : VK_NULL_HANDLE;
}

std::shared_ptr<Framebuffer> RenderPass::GetFramebuffer() const noexcept
{
  return m_framebuffer;
}

void RenderPass::OnSubmit(SubmitTask & submitTask)
{
  m_framebuffer->EndFrame(submitTask.GetSemaphore());
}

void RenderPass::Invalidate()
{
  // collect info about how each attachment is used during render pass
  std::vector<VkImageUsageFlags> attachmentsUsage;
  attachmentsUsage.resize(m_framebuffer->GetAttachmentsCount(), 0);
  for (auto && subpass : m_subpasses)
  {
    subpass->GetPipeline().GetAttachmentUsageInfo().CollectAttachmentsUsageInfo(attachmentsUsage);
  }

  bool renderPassChanged = false;
  // rebuild attachments
  if (m_framebuffer->Invalidate(attachmentsUsage))
  {
    for (auto && subpass : m_subpasses)
    {
      subpass->CollectBarriers();
    }

    std::unique_ptr<SubpassGraph> newRenderGraph = std::make_unique<SubpassGraph>(GetContext());
    for (auto && attachment : m_framebuffer->GetAttachments())
    {
      if (attachment)
      {
        ResourceState initialState{VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE,
                                   VK_IMAGE_LAYOUT_UNDEFINED};
        ResourceState finalState{VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                                 VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                                 attachment->IsPresent()
                                   ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
                                   : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        newRenderGraph->AddAttachment(*attachment, initialState, finalState, m_loadOp, m_storeOp);
      }
    }

    for (auto && subpass : m_subpasses)
    {
      SubpassIndex index = newRenderGraph->AddSubpass(subpass->GetPipeline());
      subpass->AddDependenciesTo(*newRenderGraph, index);
    }

    if (newRenderGraph->Compile())
    {
      m_renderGraph = std::move(newRenderGraph);
      renderPassChanged = true;
    }
  }

  if (renderPassChanged)
  {
    std::vector<RenderTarget> newTargets;
    newTargets.reserve(m_framebuffer->GetBuffering());
    for (size_t i = 0; i < m_framebuffer->GetBuffering(); ++i)
    {
      auto && target = newTargets.emplace_back(GetContext());
      target.BindRenderPass(m_renderGraph->GetHandle());
      //TODO: uncomment code below
      //target.SetExtent();
      //target.SetAttachments();
      //target.RebuildFramebuffer();
    }
    m_targets = std::move(newTargets);
  }

  if (renderPassChanged)
  {
    for (uint32_t i = 0; auto && subpass : m_subpasses)
    {
      subpass->GetPipeline().Invalidate(*this, i);
      ++i;
    }
  }
}

} // namespace RHI::vulkan
