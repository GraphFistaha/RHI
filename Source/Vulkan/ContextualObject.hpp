#pragma once

namespace RHI::vulkan
{
struct Context;

/// @brief object owned by context
struct ContextualObject
{
  explicit ContextualObject(Context & ctx);
  virtual ~ContextualObject() = default;
  ContextualObject(ContextualObject && rhs) noexcept;
  ContextualObject & operator=(ContextualObject && rhs) noexcept;

  const Context & GetContext() const & noexcept;
  Context & GetContext() & noexcept;

private:
  Context * m_context = nullptr;
};
} // namespace RHI::vulkan
