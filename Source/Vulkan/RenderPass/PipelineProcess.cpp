#include "PipelineProcess.hpp"

#include <CommandsExecution/CommandBuffer.hpp>
#include <Private/FastDynamicCast.hpp>
#include <Private/Overload.hpp>
#include <Memory/BufferGPU.hpp>
#include <utils/CastHelper.hpp>

namespace RHI::vulkan
{

PipelineProcess::PipelineProcess(Context & ctx)
  : OwnedBy<Context>(ctx)
{
}

PipelineProcess::~PipelineProcess()
{
}

void PipelineProcess::RecordCommands(details::CommandBuffer & commands,
                                     const SubpassConfiguration & pipeline)
{
  for (auto && task : m_commands)
  {
    if (task)
      task(commands, pipeline);
  }
}

void PipelineProcess::SynchroniseResources(details::CommandBuffer & commands)
{
  for (auto && [objPtr, pipelineStage, access, layout] : m_resourceSyncInfos)
  {
    std::visit(std::overload(
                 [pipelineStage, access, layout, &commands](IInternalBuffer * buffer)
                 {
                   if (buffer)
                     buffer->GetSynchronizer().RequireSynchronize(pipelineStage, access, commands,
                                                                  layout);
                 },
                 [pipelineStage, access, layout, &commands](IInternalTexture * texture)
                 {
                   if (texture)
                     texture->GetSynchronizer().RequireSynchronize(pipelineStage, access, commands,
                                                                   layout);
                 }),
               objPtr);
  }
}

void PipelineProcess::ResetSynchronisation()
{
    //TODO: implement it 
}

void PipelineProcess::CommitProcess()
{
  m_editable = false;
}

void PipelineProcess::DrawVertices(std::uint32_t vertexCount, std::uint32_t instanceCount,
                                   std::uint32_t firstVertex, std::uint32_t firstInstance)
{
  if (!m_editable)
    return;
  auto task = [=](details::CommandBuffer & commands, const SubpassConfiguration & pipeline)
  {
    commands.PushCommand(vkCmdDraw, vertexCount, instanceCount, firstVertex, firstInstance);
  };
  m_commands.push_back(task);
}

void PipelineProcess::DrawIndexedVertices(std::uint32_t indexCount, std::uint32_t instanceCount,
                                          std::uint32_t firstIndex, int32_t vertexOffset,
                                          std::uint32_t firstInstance)
{
  if (!m_editable)
    return;
  auto task = [=](details::CommandBuffer & commands, const SubpassConfiguration & pipeline)
  {
    commands.PushCommand(vkCmdDrawIndexed, indexCount, instanceCount, firstIndex, vertexOffset,
                         firstInstance);
  };
  m_commands.push_back(task);
}

void PipelineProcess::SetViewport(float width, float height)
{
  if (!m_editable)
    return;
  auto task = [=](details::CommandBuffer & commands, const SubpassConfiguration & pipeline)
  {
    VkViewport vp{0.0f, 0.0f, width, height, 0.0f, 1.0f};
    commands.PushCommand(vkCmdSetViewport, 0, 1, &vp);
  };
  m_commands.push_back(task);
}

void PipelineProcess::SetScissor(int32_t x, int32_t y, std::uint32_t width, std::uint32_t height)
{
  if (!m_editable)
    return;
  auto task = [=](details::CommandBuffer & commands, const SubpassConfiguration & pipeline)
  {
    VkRect2D scissor{};
    scissor.extent = {width, height};
    scissor.offset = {x, y};
    commands.PushCommand(vkCmdSetScissor, 0, 1, &scissor);
  };
  m_commands.push_back(task);
}

void PipelineProcess::BindVertexBuffer(std::uint32_t binding, IBufferGPU * buffer,
                                       std::uint32_t offset)
{
  if (!m_editable)
    return;
  auto * internalBuffer = FastDynamicCast<IInternalBuffer>(buffer);
  if (!internalBuffer)
    return;
  auto task = [=](details::CommandBuffer & commands, const SubpassConfiguration & pipeline)
  {
    VkDeviceSize vkOffset = offset;
    VkBuffer buf = internalBuffer->GetHandle();
    commands.PushCommand(vkCmdBindVertexBuffers, 0, 1, &buf, &vkOffset);
  };
  m_resourceSyncInfos.push_back({internalBuffer, VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT,
                                 VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED});
  m_commands.push_back(task);
}

void PipelineProcess::BindIndexBuffer(IBufferGPU * buffer, IndexType type, std::uint32_t offset)
{
  if (!m_editable)
    return;
  auto * internalBuffer = FastDynamicCast<IInternalBuffer>(buffer);
  if (!internalBuffer)
    return;
  auto task = [=](details::CommandBuffer & commands, const SubpassConfiguration & pipeline)
  {
    commands.PushCommand(vkCmdBindIndexBuffer, internalBuffer->GetHandle(), VkDeviceSize{offset},
                         utils::CastInterfaceEnum2Vulkan<VkIndexType>(type));
  };
  m_resourceSyncInfos.push_back({internalBuffer, VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT,
                                 VK_ACCESS_2_INDEX_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED});
  m_commands.push_back(task);
}

void PipelineProcess::PushConstant(const void * data, size_t size)
{
  if (!m_editable)
    return;
  auto task = [=](details::CommandBuffer & commands, const SubpassConfiguration & pipeline)
  {
    commands.PushCommand(vkCmdPushConstants, pipeline.GetPipelineLayoutHandle(),
                         VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                         static_cast<uint32_t>(size), data);
  };
}
} // namespace RHI::vulkan
