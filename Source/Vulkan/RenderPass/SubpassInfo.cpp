#include "SubpassInfo.hpp"

#include <Memory/Synchronizer.hpp>
#include <Private/Overload.hpp>
#include <RenderPass/RenderPass.hpp>
#include <RenderPass/SubpassGraph.hpp>
#include <Utils/CastHelper.hpp>
#include <VulkanContext.hpp>


namespace RHI::vulkan
{

SubpassInfo::SubpassInfo(RenderPass & renderPass, std::shared_ptr<Pipeline> pipeline)
  : OwnedBy<RenderPass>(renderPass)
  , m_pipeline(std::move(pipeline))
{
}

const Pipeline & SubpassInfo::GetPipeline() const & noexcept
{
  return *m_pipeline;
}

Pipeline & SubpassInfo::GetPipeline() & noexcept
{
  return *m_pipeline;
}

void SubpassInfo::AddDependenciesTo(SubpassGraph & graph, SubpassIndex index) const
{
  for (auto && [resource, requiredState] : m_barriers)
  {
    std::visit(std::overload(
                 [&graph, index, requiredState](IInternalBuffer * buffer)
                 {
                   if (!buffer)
                     return;
                   graph.AddSelfDependency(index, buffer->GetSynchronizer().GetState(),
                                           requiredState);
                 },
                 [&graph, index, requiredState](IInternalTexture * texture)
                 {
                   if (!texture)
                     return;
                   graph.AddSelfDependency(index, texture->GetSynchronizer().GetState(),
                                           requiredState);
                 },
                 [&graph, index](uint32_t attachmentIndex)
                 {
                   auto [prevIndex, prevState] = graph.FindLastUsage(attachmentIndex);
                   if (index == SubpassIndex::Subpass0 ||
                       prevIndex == SubpassIndex::initialRenderPass)
                   {
                     graph.AddIncomingDependency(prevState, index, attachmentIndex);
                   }
                   else
                   {
                     graph.AddDependency(prevIndex, index, attachmentIndex);
                   }
                 }),
               resource);
  }
}

void SubpassInfo::DrawVertices(std::uint32_t vertexCount, std::uint32_t instanceCount,
                               std::uint32_t firstVertex, std::uint32_t firstInstance)
{
  auto task = [=](details::CommandBuffer & commands)
  {
    commands.PushCommand(vkCmdDraw, vertexCount, instanceCount, firstVertex, firstInstance);
  };
  m_commands.push_back(task);
}

void SubpassInfo::DrawIndexedVertices(std::uint32_t indexCount, std::uint32_t instanceCount,
                                      std::uint32_t firstIndex, int32_t vertexOffset,
                                      std::uint32_t firstInstance)
{
  auto task = [=](details::CommandBuffer & commands)
  {
    commands.PushCommand(vkCmdDrawIndexed, indexCount, instanceCount, firstIndex, vertexOffset,
                         firstInstance);
  };
  m_commands.push_back(task);
}

void SubpassInfo::SetViewport(float width, float height)
{
  auto task = [=](details::CommandBuffer & commands)
  {
    VkViewport vp{0.0f, 0.0f, width, height, 0.0f, 1.0f};
    commands.PushCommand(vkCmdSetViewport, 0, 1, &vp);
  };
  m_commands.push_back(task);
}

void SubpassInfo::SetScissor(int32_t x, int32_t y, std::uint32_t width, std::uint32_t height)
{
  auto task = [=](details::CommandBuffer & commands)
  {
    VkRect2D scissor{};
    scissor.extent = {width, height};
    scissor.offset = {x, y};
    commands.PushCommand(vkCmdSetScissor, 0, 1, &scissor);
  };
  m_commands.push_back(task);
}

void SubpassInfo::BindVertexBuffer(std::uint32_t binding, IBufferGPU * buffer, std::uint32_t offset)
{
  auto * internalBuffer = FastDynamicCast<IInternalBuffer>(buffer);
  if (!internalBuffer)
    return;
  auto task = [=](details::CommandBuffer & commands)
  {
    VkDeviceSize vkOffset = offset;
    VkBuffer buf = internalBuffer->GetHandle();
    commands.PushCommand(vkCmdBindVertexBuffers, binding, 1, &buf, &vkOffset);
  };
  m_commands.push_back(task);
  m_barriers.push_back(
    {internalBuffer,
     ResourceState{VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT,
                   VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED}});
}

void SubpassInfo::BindIndexBuffer(IBufferGPU * buffer, IndexType type, std::uint32_t offset)
{
  auto * internalBuffer = FastDynamicCast<IInternalBuffer>(buffer);
  if (!internalBuffer)
    return;
  auto task = [=](details::CommandBuffer & commands)
  {
    commands.PushCommand(vkCmdBindIndexBuffer, internalBuffer->GetHandle(), VkDeviceSize{offset},
                         utils::CastInterfaceEnum2Vulkan<VkIndexType>(type));
  };
  m_commands.push_back(task);

  m_barriers.push_back(
    {internalBuffer, ResourceState{VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT, VK_ACCESS_2_INDEX_READ_BIT,
                                   VK_IMAGE_LAYOUT_UNDEFINED}});
}

void SubpassInfo::PushConstant(const void * data, size_t size)
{
  std::vector<uint8_t> capturedData(size, 0);
  std::memcpy(capturedData.data(), data, size);
  auto task =
    [data = std::move(capturedData), pipeline = m_pipeline](details::CommandBuffer & commands)
  {
    commands.PushCommand(vkCmdPushConstants, pipeline->GetPipelineLayoutHandle(),
                         VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                         static_cast<uint32_t>(data.size()), data.data());
  };
  m_commands.push_back(task);
}

bool SubpassInfo::IsInlineCommands() const
{
  return !m_commandsCache;
}

void SubpassInfo::RecordCommands(details::CommandBuffer & commands)
{
  // in RenderPass you must not include any memoryBarrier,
  // so it's allowed ImageOnly synchronization
  SynchroniseResources(SynchronizationFilter::ImageOnly, commands);
  m_pipeline->BindToCommandBuffer(commands, VK_PIPELINE_BIND_POINT_GRAPHICS);
  for (auto && cmd : m_commands)
  {
    cmd(commands);
  }
}

void SubpassInfo::ResetSynchronization() const
{
  constexpr ResourcePtr nullPtr = static_cast<IInternalTexture *>(nullptr);
  ResourcePtr prevPtr = static_cast<IInternalTexture *>(nullptr);
  // reset synchronization in the begining of pass
  for (auto && barrier : m_barriers)
  {
    if (prevPtr == nullPtr || barrier.first != prevPtr)
    {
      std::visit(std::overload(
                   [](IInternalBuffer * buffer)
                   {
                     if (buffer)
                       buffer->GetSynchronizer().ResetSynchronization();
                   },
                   [](IInternalTexture * texture)
                   {
                     if (texture)
                       texture->GetSynchronizer().ResetSynchronization();
                   },
                   [this](uint32_t attachmentIndex)
                   {
                     if (auto * attachment =
                           GetRenderPass().GetFramebuffer()->GetAttachment(attachmentIndex))
                       attachment->GetSynchronizer().ResetSynchronization();
                   }),
                 barrier.first);
      prevPtr = barrier.first;
    }
  }
}

void SubpassInfo::SynchroniseResources(SynchronizationFilter filter,
                                       details::CommandBuffer & commands) const
{
  for (auto && barrier : m_barriers)
  {
    std::visit(std::overload(
                 [&commands, filter, state = barrier.second](IInternalBuffer * buffer)
                 {
                   if (buffer && FilterSatisfied(filter, SynchronizationFilter::BufferOnly))
                     buffer->GetSynchronizer().RequireSynchronize(state.currentStage,
                                                                  state.requiredAccess, commands,
                                                                  state.requiredLayout);
                 },
                 [&commands, filter, state = barrier.second](IInternalTexture * texture)
                 {
                   if (texture && FilterSatisfied(filter, SynchronizationFilter::ImageOnly))
                     texture->GetSynchronizer().RequireSynchronize(state.currentStage,
                                                                   state.requiredAccess, commands,
                                                                   state.requiredLayout);
                 },
                 [this, &commands, filter, state = barrier.second](uint32_t attachmentIndex)
                 {
                   if (!FilterSatisfied(filter, SynchronizationFilter::AttachmentOnly))
                     return;
                   if (auto * attachment =
                         GetRenderPass().GetFramebuffer()->GetAttachment(attachmentIndex))
                     attachment->GetSynchronizer().RequireSynchronize(state.currentStage,
                                                                      state.requiredAccess,
                                                                      commands,
                                                                      state.requiredLayout);
                 }),
               barrier.first);
  }
}

void SubpassInfo::CollectBarriers()
{
  std::vector<Barrier> newBarriers;
  m_pipeline->CollectBarriers(newBarriers);
  std::ranges::sort(newBarriers, [](auto && a, auto && b)
                    { return a.first > b.first; }); // sort to equal pointers stay close
  m_barriers = std::move(newBarriers);
}

} // namespace RHI::vulkan
