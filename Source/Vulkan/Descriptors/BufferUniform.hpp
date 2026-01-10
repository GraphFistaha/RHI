#pragma once

#include <Descriptors/BaseUniform.hpp>
#include <Resources/BufferGPU.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>


namespace RHI::vulkan
{

struct BufferUniform final : public IBufferUniformDescriptor,
                             public details::BaseUniform
{
  explicit BufferUniform(Context & ctx, DescriptorBufferLayout & owner, VkDescriptorType type,
                         LayoutIndex index, uint32_t arrayIndex = 0);
  virtual ~BufferUniform() override = default;
  BufferUniform(BufferUniform && rhs) noexcept;
  BufferUniform & operator=(BufferUniform && rhs) noexcept;

public:
  std::vector<VkDescriptorBufferInfo> CreateDescriptorInfo() const;


public: // IBufferUniformDescriptor interface
  virtual void AssignBuffer(const IBufferGPU & buffer, size_t offset = 0) override;
  virtual bool IsBufferAssigned() const noexcept override;

public: // IUniformDescriptor interface
  virtual uint32_t GetSet() const noexcept override { return BaseUniform::GetSet(); }
  virtual uint32_t GetBinding() const noexcept override { return BaseUniform::GetBinding(); }
  virtual uint32_t GetArrayIndex() const noexcept override { return BaseUniform::GetArrayIndex(); }

public: // IInvalidable interface
  void Invalidate();
  void SetInvalid();

public: // public internal API
  size_t GetOffset() const noexcept { return m_offset; }
  VkBuffer GetBuffer() const noexcept { return m_buffer; }
  using BaseUniform::GetDescriptorType;

private:
  VkBuffer m_buffer = VK_NULL_HANDLE;
  size_t m_size = 0;
  size_t m_offset = 0;
};

} // namespace RHI::vulkan
