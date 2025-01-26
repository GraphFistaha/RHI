#include "ContextualObject.hpp"

#include "VulkanContext.hpp"

namespace RHI::vulkan
{
const Context & ContextualObject::GetContext() const & noexcept
{
  assert(m_context && m_context->IsValid());
  return *m_context;
}
} // namespace RHI::vulkan
