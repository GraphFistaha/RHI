#include "ContextualObject.hpp"

#include "VulkanContext.hpp"

namespace RHI::vulkan
{
ContextualObject::ContextualObject(Context & ctx)
  : m_context(&ctx)
{
}
ContextualObject::ContextualObject(ContextualObject && rhs) noexcept
  : m_context(std::move(rhs.m_context))
{

  //rhs.m_context = nullptr;
}

ContextualObject & ContextualObject::operator=(ContextualObject && rhs) noexcept
{
  if (this != &rhs)
  {
    std::swap(m_context, rhs.m_context);
  }
  return *this;
}

const Context & ContextualObject::GetContext() const & noexcept
{
  assert(m_context && m_context->IsValid());
  return *m_context;
}

Context & ContextualObject::GetContext() & noexcept
{
  assert(m_context && m_context->IsValid());
  return *m_context;
}
} // namespace RHI::vulkan
