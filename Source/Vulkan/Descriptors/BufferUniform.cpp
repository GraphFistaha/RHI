#include "BufferUniform.hpp"

#include <Descriptors/DescriptorBufferLayout.hpp>
#include <Private/FastDynamicCast.hpp>
#include <Utils/CastHelper.hpp>
#include <VulkanContext.hpp>

namespace RHI::vulkan
{
BufferUniform::BufferUniform(Context & ctx, DescriptorBufferLayout & owner, VkDescriptorType type,
                             LayoutIndex index, uint32_t arrayIndex)
  : BaseUniform(ctx, owner, type, index, arrayIndex)
  , IBufferUniformDescriptor()
{
}

BufferUniform::BufferUniform(BufferUniform && rhs) noexcept
  : BaseUniform(std::move(rhs))
{
  std::swap(m_buffer, rhs.m_buffer);
  std::swap(m_offset, rhs.m_offset);
}

BufferUniform & BufferUniform::operator=(BufferUniform && rhs) noexcept
{
  if (this != &rhs)
  {
    BaseUniform::operator=(std::move(rhs));
    std::swap(m_buffer, rhs.m_buffer);
    std::swap(m_offset, rhs.m_offset);
  }
  return *this;
}

void BufferUniform::AssignBuffer(IBufferGPU * buffer, size_t offset)
{
  Invalidate();
  m_buffer = FastDynamicCast<IInternalBuffer>(buffer);
  m_offset = offset;
  GetLayout().GetConfiguration().GetSubpass().OnDescriptorChanged(*this);
}

bool BufferUniform::IsBufferAssigned() const noexcept
{
  return m_buffer;
}

void BufferUniform::Synchronize(details::CommandBuffer & commands, VkImageLayout layout)
{
  if (m_buffer)
  {
    m_buffer->GetSynchronizer().RequireSynchronize(VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                                                   VK_ACCESS_2_SHADER_READ_BIT, commands, layout);
  }
}

void BufferUniform::Invalidate()
{
}

void BufferUniform::SetInvalid()
{
}

std::vector<VkDescriptorBufferInfo> BufferUniform::CreateDescriptorInfo() const
{
  VkDescriptorBufferInfo bufferInfo{};
  bufferInfo.buffer = m_buffer->GetHandle();
  bufferInfo.range = m_buffer->GetSize();
  bufferInfo.offset = m_offset;
  return {bufferInfo};
}

} // namespace RHI::vulkan
