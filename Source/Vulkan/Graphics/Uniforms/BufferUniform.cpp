#include "BufferUniform.hpp"

#include "../../Utils/CastHelper.hpp"
#include "../../VulkanContext.hpp"
#include "../DescriptorsBuffer.hpp"

namespace RHI::vulkan
{
BufferUniform::BufferUniform(const Context & ctx, DescriptorBuffer & owner, VkDescriptorType type,
                             uint32_t binding, uint32_t arrayIndex)
  : BaseUniform(ctx, owner, type, binding, arrayIndex)
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
  BaseUniform::operator=(std::move(rhs));
  if (this != &rhs)
  {
    std::swap(m_buffer, rhs.m_buffer);
    std::swap(m_size, rhs.m_size);
    std::swap(m_offset, rhs.m_offset);
  }
  return *this;
}

void BufferUniform::AssignBuffer(const IBufferGPU & buffer, size_t offset)
{
  auto && internalBuffer = utils::CastInterfaceClass2Internal<const BufferGPU &>(buffer);
  m_buffer = internalBuffer.GetHandle();
  m_size = internalBuffer.Size();
  m_offset = offset;
  assert(m_owner);
  m_owner->OnDescriptorChanged(*this);
}

bool BufferUniform::IsBufferAssigned() const noexcept
{
  return m_buffer;
}

uint32_t BufferUniform::GetBinding() const noexcept
{
  return BaseUniform::GetBinding();
}

uint32_t BufferUniform::GetArrayIndex() const noexcept
{
  return BaseUniform::GetArrayIndex();
}

void BufferUniform::Invalidate()
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
