#pragma once

#include <OwnedBy.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan
{
struct Context;
struct DescriptorBufferLayout;
} // namespace RHI::vulkan

namespace RHI::vulkan::details
{
struct BaseUniform : public OwnedBy<Context>,
                     public OwnedBy<DescriptorBufferLayout>
{
  explicit BaseUniform(Context & ctx, DescriptorBufferLayout & owner, VkDescriptorType type,
                       LayoutIndex index, uint32_t arrayIndex = 0)
    : OwnedBy<Context>(ctx)
    , OwnedBy<DescriptorBufferLayout>(owner)
    , m_type(type)
    , m_arrayIndex(arrayIndex)
    , m_index(index)
  {
  }
  virtual ~BaseUniform() = default;
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);
  MAKE_ALIAS_FOR_GET_OWNER(DescriptorBufferLayout, GetLayout);

  BaseUniform(BaseUniform && rhs) noexcept
    : OwnedBy<Context>(std::move(rhs))
    , OwnedBy<DescriptorBufferLayout>(std::move(rhs))
  {
    std::swap(m_type, rhs.m_type);
    std::swap(m_arrayIndex, rhs.m_arrayIndex);
    std::swap(m_index, rhs.m_index);
  }

  BaseUniform & operator=(BaseUniform && rhs) noexcept
  {
    if (this != &rhs)
    {
      OwnedBy<Context>::operator=(std::move(rhs));
      OwnedBy<DescriptorBufferLayout>::operator=(std::move(rhs));
      std::swap(m_type, rhs.m_type);
      std::swap(m_arrayIndex, rhs.m_arrayIndex);
      std::swap(m_index, rhs.m_index);
    }
    return *this;
  }

  VkDescriptorType GetDescriptorType() const noexcept { return m_type; }
  uint32_t GetArrayIndex() const noexcept { return m_arrayIndex; }
  uint32_t GetBinding() const noexcept { return m_index.binding; }
  uint32_t GetSet() const noexcept { return m_index.set; }

protected:
  VkDescriptorType m_type;
  uint32_t m_arrayIndex = 0;
  LayoutIndex m_index{0, 0};
};

} // namespace RHI::vulkan::details
