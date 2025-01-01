#pragma once

#include <vulkan/vulkan.hpp>

namespace RHI::vulkan
{
struct Context;
struct DescriptorBuffer;
} // namespace RHI::vulkan

namespace RHI::vulkan::details
{
struct BaseUniform
{
  explicit BaseUniform(const Context & ctx, DescriptorBuffer & owner, VkDescriptorType type,
                       uint32_t binding, uint32_t arrayIndex = 0)
    : m_context(ctx)
    , m_owner(&owner)
    , m_type(type)
    , m_arrayIndex(arrayIndex)
    , m_binding(binding)
  {
  }
  virtual ~BaseUniform() = default;

  BaseUniform(BaseUniform && rhs) noexcept
    : m_context(rhs.m_context)
  {
    std::swap(m_owner, rhs.m_owner);
    std::swap(m_type, rhs.m_type);
    std::swap(m_arrayIndex, rhs.m_arrayIndex);
    std::swap(m_binding, rhs.m_binding);
  }

  BaseUniform & operator=(BaseUniform && rhs) noexcept
  {
    if (this != &rhs && &m_context == &rhs.m_context)
    {
      std::swap(m_owner, rhs.m_owner);
      std::swap(m_type, rhs.m_type);
      std::swap(m_arrayIndex, rhs.m_arrayIndex);
      std::swap(m_binding, rhs.m_binding);
    }
    return *this;
  }

  VkDescriptorType GetDescriptorType() const noexcept { return m_type; }
  uint32_t GetArrayIndex() const noexcept { return m_arrayIndex; }
  uint32_t GetBinding() const noexcept { return m_binding; }

  const Context & m_context;
  DescriptorBuffer * m_owner;

protected:
  VkDescriptorType m_type;
  uint32_t m_arrayIndex = 0;
  uint32_t m_binding;
};

} // namespace RHI::vulkan::details
