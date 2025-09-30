#include "BufferUniform.hpp"

#include <Descriptors/DescriptorBufferLayout.hpp>
#include <Utils/CastHelper.hpp>
#include <VulkanContext.hpp>

namespace RHI::vulkan
{
BufferUniform::BufferUniform(Context & ctx, DescriptorBufferLayout & owner, VkDescriptorType type,
                             LayoutIndex index, uint32_t arrayIndex)
  : BaseUniform(ctx, owner, type, index, arrayIndex)
{
}

BufferUniform::BufferUniform(BufferUniform && rhs) noexcept
  : BaseUniform(std::move(rhs))
{
  std::swap(m_buffer, rhs.m_buffer);
  std::swap(m_size, rhs.m_size);
  std::swap(m_offset, rhs.m_offset);
}

BufferUniform & BufferUniform::operator=(BufferUniform && rhs) noexcept
{
  if (this != &rhs)
  {
    BaseUniform::operator=(std::move(rhs));
    std::swap(m_buffer, rhs.m_buffer);
    std::swap(m_size, rhs.m_size);
    std::swap(m_offset, rhs.m_offset);
  }
  return *this;
}

void BufferUniform::AssignBuffer(const IBufferGPU & buffer, size_t offset)
{
  Invalidate();
  auto && internalBuffer = utils::CastInterfaceClass2Internal<const BufferGPU &>(buffer);
  m_buffer = internalBuffer.GetHandle();
  m_size = internalBuffer.Size();
  m_offset = offset;
  GetLayout().OnDescriptorChanged(*this);
}

bool BufferUniform::IsBufferAssigned() const noexcept
{
  return m_buffer;
}

void BufferUniform::Invalidate()
{
}

void BufferUniform::SetInvalid()
{
}

VkDescriptorBufferInfo BufferUniform::CreateDescriptorInfo() const noexcept
{
  VkDescriptorBufferInfo bufferInfo{};
  bufferInfo.buffer = m_buffer;
  bufferInfo.range = m_size;
  bufferInfo.offset = m_offset;
  return bufferInfo;
}

} // namespace RHI::vulkan
