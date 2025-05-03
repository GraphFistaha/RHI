#pragma once

#include <OwnedBy.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan
{
struct Context;
struct DescriptorBuffer;
} // namespace RHI::vulkan

namespace RHI::vulkan::details
{
struct BaseUniform : public OwnedBy<Context>,
                     public OwnedBy<DescriptorBuffer>
{
  explicit BaseUniform(Context & ctx, DescriptorBuffer & owner, VkDescriptorType type,
                       uint32_t binding, uint32_t arrayIndex = 0)
    : OwnedBy<Context>(ctx)
    , OwnedBy<DescriptorBuffer>(owner)
    , m_type(type)
    , m_arrayIndex(arrayIndex)
    , m_binding(binding)
  {
  }
  virtual ~BaseUniform() = default;

  BaseUniform(BaseUniform && rhs) noexcept
    : OwnedBy<Context>(std::move(rhs))
    , OwnedBy<DescriptorBuffer>(std::move(rhs))
  {
    std::swap(m_type, rhs.m_type);
    std::swap(m_arrayIndex, rhs.m_arrayIndex);
    std::swap(m_binding, rhs.m_binding);
  }

  BaseUniform & operator=(BaseUniform && rhs) noexcept
  {
    if (this != &rhs)
    {
      OwnedBy<Context>::operator=(std::move(rhs));
      OwnedBy<DescriptorBuffer>::operator=(std::move(rhs));
      std::swap(m_type, rhs.m_type);
      std::swap(m_arrayIndex, rhs.m_arrayIndex);
      std::swap(m_binding, rhs.m_binding);
    }
    return *this;
  }

  VkDescriptorType GetDescriptorType() const noexcept { return m_type; }
  uint32_t GetArrayIndex() const noexcept { return m_arrayIndex; }
  uint32_t GetBinding() const noexcept { return m_binding; }
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);
  MAKE_ALIAS_FOR_GET_OWNER(DescriptorBuffer, GetDescriptorsBuffer);

protected:
  VkDescriptorType m_type;
  uint32_t m_arrayIndex = 0;
  uint32_t m_binding;
};

} // namespace RHI::vulkan::details
