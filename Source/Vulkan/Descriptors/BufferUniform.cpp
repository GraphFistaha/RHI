#include "BufferUniform.hpp"

#include <Descriptors/DescriptorBufferLayout.hpp>
#include <Memory/Synchronizer.hpp>
#include <Private/FastDynamicCast.hpp>
#include <RenderPass/SubpassConfiguration.hpp>
#include <Utils/CastHelper.hpp>
#include <VulkanContext.hpp>

namespace RHI::vulkan
{
BufferUniform::BufferUniform(Context & ctx, DescriptorBufferLayout & owner, VkDescriptorType type,
                             LayoutIndex index, uint32_t arrayIndex)
  : BaseDescriptor(ctx, owner, type, index, arrayIndex)
  , IBufferUniformDescriptor()
{
}

BufferUniform::BufferUniform(BufferUniform && rhs) noexcept
  : BaseDescriptor(std::move(rhs))
{
  std::swap(m_buffer, rhs.m_buffer);
  std::swap(m_offset, rhs.m_offset);
}

BufferUniform & BufferUniform::operator=(BufferUniform && rhs) noexcept
{
  if (this != &rhs)
  {
    BaseDescriptor::operator=(std::move(rhs));
    std::swap(m_buffer, rhs.m_buffer);
    std::swap(m_offset, rhs.m_offset);
  }
  return *this;
}

UpdateDescriptorTask BufferUniform::CreateUpdateTask() const noexcept
{
  VkDescriptorBufferInfo bufferInfo{};
  bufferInfo.buffer = m_buffer->GetHandle();
  bufferInfo.range = m_buffer->GetSize();
  bufferInfo.offset = m_offset;
  return
    [bufferInfo, binding = GetBinding(), arrayIdx = GetArrayIndex(), type = GetDescriptorType(),
     setIdx = GetSet()](const Context & ctx, std::span<const VkDescriptorSet> sets) mutable
  {
    VkWriteDescriptorSet writeInfo{};
    writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeInfo.descriptorType = type;
    writeInfo.dstArrayElement = arrayIdx;
    writeInfo.dstBinding = binding;
    writeInfo.descriptorCount = 1;
    writeInfo.dstSet = sets[setIdx];
    writeInfo.pBufferInfo = &bufferInfo;
    vkUpdateDescriptorSets(ctx.GetGpuConnection().GetDevice(), 1, &writeInfo, 0, nullptr);
  };
}

void BufferUniform::AssignBuffer(IBufferGPU * buffer, size_t offset)
{
  Invalidate();
  m_buffer = FastDynamicCast<IInternalBuffer>(buffer);
  m_offset = offset;
  GetLayout().GetSubpass().OnDescriptorChanged(CreateUpdateTask());
}

bool BufferUniform::IsBufferAssigned() const noexcept
{
  return m_buffer;
}

void BufferUniform::CollectResources(std::vector<ResourcePtr> & resources) const
{
  if (m_buffer)
    resources.push_back(m_buffer);
}

void BufferUniform::SynchroniseResources(details::CommandBuffer & commands) const
{
  if (m_buffer)
  {
    m_buffer->GetSynchronizer().RequireSynchronize(VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                                                   VK_ACCESS_2_SHADER_READ_BIT, commands,
                                                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  }
}

void BufferUniform::Invalidate()
{
}

void BufferUniform::SetInvalid()
{
}

} // namespace RHI::vulkan
