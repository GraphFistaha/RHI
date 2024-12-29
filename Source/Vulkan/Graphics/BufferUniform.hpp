#pragma once

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "../Resources/BufferGPU.hpp"

namespace RHI::vulkan
{
struct Context;
struct DescriptorBuffer;
} // namespace RHI::vulkan

namespace RHI::vulkan
{

struct BufferUniform final : public IBufferUniformDescriptor
{
  static constexpr uint32_t InvalidDescriptorIndex = -1;

  explicit BufferUniform(const Context & ctx, DescriptorBuffer & owner, VkDescriptorType type,
                         uint32_t descriptorIndex, uint32_t binding);
  virtual ~BufferUniform() override;
  BufferUniform(BufferUniform && rhs) noexcept;
  BufferUniform & operator=(BufferUniform && rhs) noexcept;

public: // IBufferUniformDescriptor interface
  virtual void AssignBuffer(const IBufferGPU & buffer, size_t offset = 0) override;
  virtual bool IsBufferAssigned() const noexcept override;

public: // IUniformDescriptor interface
  virtual uint32_t GetDescriptorIndex() const noexcept override;
  virtual uint32_t GetBinding() const noexcept override;
  VkDescriptorType GetDescriptorType() const noexcept { return m_type; }

public: // IInvalidable interface
  virtual void Invalidate() override;

public: // public internal API
  size_t GetOffset() const noexcept { return m_offset; }
  VkBuffer GetBuffer() const noexcept { return m_buffer; }
  VkDescriptorBufferInfo CreateDescriptorInfo() const noexcept;

private:
  const Context & m_context;
  DescriptorBuffer * m_owner;
  VkDescriptorType m_type;
  uint32_t m_descriptorIndex;
  uint32_t m_binding;

  VkBuffer m_buffer = VK_NULL_HANDLE;
  size_t m_size = 0;
  size_t m_offset = 0;
};

} // namespace RHI::vulkan
