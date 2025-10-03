#pragma once

#include <OwnedBy.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.h>

namespace RHI::vulkan
{
struct Device;

struct Surface final : public OwnedBy<Device>
{
  explicit Surface(Device & device, const SurfaceConfig & config);
  ~Surface();
  Surface(Surface && rhs) noexcept;
  Surface & operator=(Surface && rhs) noexcept;
  MAKE_ALIAS_FOR_GET_OWNER(Device, GetDevice);
  RESTRICTED_COPY(Surface);

public:
  operator VkSurfaceKHR() const noexcept { return m_surface; }

private:
  VkSurfaceKHR m_surface = VK_NULL_HANDLE;
};

} // namespace RHI::vulkan
