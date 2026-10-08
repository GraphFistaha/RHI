#include "RenderProcess.hpp"

#include <CommandsExecution/CommandBuffer.hpp>
#include <Memory/BufferGPU.hpp>
#include <Pipeline/Pipeline.hpp>
#include <Private/FastDynamicCast.hpp>
#include <Private/Overload.hpp>
#include <RenderPass/Framebuffer.hpp>
#include <RenderPass/SubpassGraph.hpp>
#include <utils/CastHelper.hpp>
#include <VulkanContext.hpp>

namespace RHI::vulkan
{

RenderProcess::RenderProcess(Context & ctx, FramebufferPtr framebuffer, PipelinePtr initialPipeline)
  : OwnedBy<Context>(ctx)
  , m_framebuffer(FastDynamicCast<Framebuffer>(framebuffer))
{
  m_pipelines.push_back(FastDynamicCast<Pipeline>(initialPipeline));
}

RenderProcess::~RenderProcess()
{
}

void RenderProcess::Barrier(IBufferGPU & buffer)
{
}

void RenderProcess::Barrier(ITexture & texture)
{
}

void RenderProcess::Barrier(IAttachment & attachment)
{
}

void RenderProcess::NextPipeline(PipelinePtr pipeline)
{
}

void RenderProcess::ClearFramebuffer()
{
  m_loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
}

void RenderProcess::LoadFramebuffer()
{
  m_loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
}

void RenderProcess::StoreFramebuffer()
{
  m_storeOp = VK_ATTACHMENT_STORE_OP_STORE;
}

void RenderProcess::RecordCommands(details::CommandBuffer & commands)
{
}

void RenderProcess::DrawVertices(std::uint32_t vertexCount, std::uint32_t instanceCount,
                                 std::uint32_t firstVertex, std::uint32_t firstInstance)
{
  //auto task = [=](details::CommandBuffer & commands)
  //{
  //  commands.PushCommand(vkCmdDraw, vertexCount, instanceCount, firstVertex, firstInstance);
  //};
  //m_commands.push_back(task);
}

void RenderProcess::DrawIndexedVertices(std::uint32_t indexCount, std::uint32_t instanceCount,
                                        std::uint32_t firstIndex, int32_t vertexOffset,
                                        std::uint32_t firstInstance)
{
  //auto task = [=](details::CommandBuffer & commands)
  //{
  //  commands.PushCommand(vkCmdDrawIndexed, indexCount, instanceCount, firstIndex, vertexOffset,
  //                       firstInstance);
  //};
  //m_commands.push_back(task);
}

void RenderProcess::SetViewport(float width, float height)
{
  //auto task = [=](details::CommandBuffer & commands)
  //{
  //  VkViewport vp{0.0f, 0.0f, width, height, 0.0f, 1.0f};
  //  commands.PushCommand(vkCmdSetViewport, 0, 1, &vp);
  //};
  //m_commands.push_back(task);
}

void RenderProcess::SetScissor(int32_t x, int32_t y, std::uint32_t width, std::uint32_t height)
{
  //auto task = [=](details::CommandBuffer & commands)
  //{
  //  VkRect2D scissor{};
  //  scissor.extent = {width, height};
  //  scissor.offset = {x, y};
  //  commands.PushCommand(vkCmdSetScissor, 0, 1, &scissor);
  //};
  //m_commands.push_back(task);
}

void RenderProcess::BindVertexBuffer(std::uint32_t binding, IBufferGPU * buffer,
                                     std::uint32_t offset)
{
  //auto * internalBuffer = FastDynamicCast<IInternalBuffer>(buffer);
  //if (!internalBuffer)
  //  return;
  //auto task = [=](details::CommandBuffer & commands)
  //{
  //  VkDeviceSize vkOffset = offset;
  //  VkBuffer buf = internalBuffer->GetHandle();
  //  commands.PushCommand(vkCmdBindVertexBuffers, binding, 1, &buf, &vkOffset);
  //};
  //m_resourceSyncInfos.push_back({internalBuffer, VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT,
  //                               VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED});
  //m_commands.push_back(task);
}

void RenderProcess::BindIndexBuffer(IBufferGPU * buffer, IndexType type, std::uint32_t offset)
{
  //auto * internalBuffer = FastDynamicCast<IInternalBuffer>(buffer);
  //if (!internalBuffer)
  //  return;
  //auto task = [=](details::CommandBuffer & commands)
  //{
  //  commands.PushCommand(vkCmdBindIndexBuffer, internalBuffer->GetHandle(), VkDeviceSize{offset},
  //                       utils::CastInterfaceEnum2Vulkan<VkIndexType>(type));
  //};

  //m_resourceSyncInfos.push_back({internalBuffer, VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT,
  //                               VK_ACCESS_2_INDEX_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED});
  //m_commands.push_back(task);
}

void RenderProcess::PushConstant(const void * data, size_t size)
{
  //std::vector<uint8_t> capturedData(size, 0);
  //std::memcpy(capturedData.data(), data, size);
  //auto task =
  //  [data = std::move(capturedData)](details::CommandBuffer & commands, const Pipeline & pipeline)
  //{
  //  commands.PushCommand(vkCmdPushConstants, pipeline.GetPipelineLayoutHandle(),
  //                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
  //                       static_cast<uint32_t>(data.size()), data.data());
  //};
  //m_commands.push_back(task);
}

bool RenderProcess::RequireSynchronization() const
{
  return false; //!m_resourceSyncInfos.empty();
}

void RenderProcess::CollectResources(std::vector<ResourcePtr> & resources) const
{
  //for (auto && [ptr, _, __, ___] : m_resourceSyncInfos)
  //{
  //  resources.push_back(ptr);
  //}
}

void RenderProcess::SynchroniseResources(SynchronizationFilter filter,
                                         details::CommandBuffer & commands) const
{
  //for (auto && [objPtr, pipelineStage, access, layout] : m_resourceSyncInfos)
  //{
  //  std::visit(std::overload(
  //               [pipelineStage, access, layout, &commands, filter](IInternalBuffer * buffer)
  //               {
  //                 if (buffer && FilterSatisfied(filter, SynchronizationFilter::BufferOnly))
  //                   buffer->GetSynchronizer().RequireSynchronize(pipelineStage, access, commands,
  //                                                                layout);
  //               },
  //               [pipelineStage, access, layout, &commands, filter](IInternalTexture * texture)
  //               {
  //                 if (texture && FilterSatisfied(filter, SynchronizationFilter::ImageOnly))
  //                   texture->GetSynchronizer().RequireSynchronize(pipelineStage, access, commands,
  //                                                                 layout);
  //               }),
  //             objPtr);
  //}
}

void RenderProcess::CollectAttachmentsUsageInfo(std::span<VkImageUsageFlags> usage) const
{
  for (auto && pipeline : m_pipelines)
  {
    pipeline->GetAttachmentUsageInfo().CollectAttachmentsUsageInfo(usage);
  }
}

void RenderProcess::Invalidate()
{
  // collect info about how each attachment is used during render pass
  std::vector<VkImageUsageFlags> attachmentsUsage;
  attachmentsUsage.resize(m_framebuffer->GetAttachments().size(), 0);
  CollectAttachmentsUsageInfo(attachmentsUsage);

  // rebuild attachments
  if (m_framebuffer->Invalidate(attachmentsUsage))
  {
    std::unique_ptr<SubpassGraph> newRenderGraph = std::make_unique<SubpassGraph>(GetContext(), VK_PIPELINE_BIND_POINT_GRAPHICS);

    for (auto && attachment : m_framebuffer->GetAttachments())
    {
      if (attachment)
      {
        newRenderGraph->AddAttachment(*attachment, {}, {}, m_loadOp, m_storeOp);
      }
    }

    for (auto && pipeline : m_pipelines)
    {
      newRenderGraph->AddSubpass(*pipeline);
    }
  }
  // build description for each attachment
  //std::vector<VkAttachmentDescription> newAttachmentsDescription;
  //newAttachmentsDescription.reserve(m_attachments.size());
  //for (auto * att : m_attachments)
  //{
  //  if (att)
  //  {
  //    newAttachmentsDescription.push_back(
  //      BuildPassAttachmentDescription(*att,
  //                                     MakeAttachmentFinalLayout(att->GetInternalFormat(),
  //                                                               att->IsPresent()),
  //                                     loadOp, storeOp));
  //  }
  //}

  // if attachments have been changed - rebuild RenderPass
  //if (m_attachmentDescriptions != newAttachmentsDescription)
  //{
  //  m_renderPass.SetInvalid();
  //}

  //m_attachmentDescriptions = std::move(newAttachmentsDescription);
}

} // namespace RHI::vulkan
