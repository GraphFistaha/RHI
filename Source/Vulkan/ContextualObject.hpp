#pragma once

namespace RHI::vulkan
{
struct Context;

/// @brief object owned by context
struct ContextualObject
{
  ContextualObject() = default;
  explicit ContextualObject(const Context & ctx)
    : m_context(&ctx)
  {
  }
  virtual ~ContextualObject() = default;

  const Context & GetContext() const & noexcept;

protected:
  const Context * m_context = nullptr;
};
} // namespace RHI::vulkan
