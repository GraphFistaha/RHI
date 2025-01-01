#pragma once

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "../../Resources/BufferGPU.hpp"
#include "BaseUniform.hpp"


namespace RHI::vulkan
{

struct BufferUniform final : public IBufferUniformDescriptor,
                             private details::BaseUniform
{
  explicit BufferUniform(const Context & ctx, DescriptorBuffer & owner, VkDescriptorType type,
                         uint32_t binding, uint32_t arrayIndex = 0);
  virtual ~BufferUniform() override = default;
  BufferUniform(BufferUniform && rhs) noexcept;
  BufferUniform & operator=(BufferUniform && rhs) noexcept;

public: // IBufferUniformDescriptor interface
  virtual void AssignBuffer(const IBufferGPU & buffer, size_t offset = 0) override;
  virtual bool IsBufferAssigned() const noexcept override;

public: // IUniformDescriptor interface
  virtual uint32_t GetBinding() const noexcept override;
  virtual uint32_t GetArrayIndex() const noexcept override;

public: // IInvalidable interface
  virtual void Invalidate() override;

public: // public internal API
  size_t GetOffset() const noexcept { return m_offset; }
  VkBuffer GetBuffer() const noexcept { return m_buffer; }
  VkDescriptorBufferInfo CreateDescriptorInfo() const noexcept;
  using BaseUniform::GetDescriptorType;

private:
  VkBuffer m_buffer = VK_NULL_HANDLE;
  size_t m_size = 0;
  size_t m_offset = 0;
};

} // namespace RHI::vulkan
