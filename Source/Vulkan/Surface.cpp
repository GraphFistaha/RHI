#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#else //TODO UNIX
#define VK_USE_PLATFORM_XLIB_KHR
#endif
#include "Surface.hpp"

#include <Device.hpp>
#include <VkBootstrap.h>

namespace
{
VkSurfaceKHR CreateSurface(VkInstance inst, const RHI::SurfaceConfig & config)
{
  VkSurfaceKHR surface = VK_NULL_HANDLE;
  VkResult result;
#ifdef _WIN32
  VkWin32SurfaceCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
  createInfo.hwnd = reinterpret_cast<HWND>(config.hWnd);
  createInfo.hinstance = reinterpret_cast<HINSTANCE>(config.hInstance);
  result = vkCreateWin32SurfaceKHR(inst, &createInfo, nullptr, &surface);
#else
  VkXlibSurfaceCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
  createInfo.window = reinterpret_cast<Window>(config.hWnd);
  createInfo.dpy = reinterpret_cast<Display *>(config.hInstance);
  result = vkCreateXlibSurfaceKHR(inst, &createInfo, nullptr, &surface);
#endif
  if (result != VK_SUCCESS)
    throw std::runtime_error("failed to create window surface!");
  return VkSurfaceKHR(surface);
}
} // namespace

namespace RHI::vulkan
{

Surface::Surface(Device & device, const RHI::SurfaceConfig & config)
  : OwnedBy<Device>(device)
  , m_surface(CreateSurface(device.GetInstance(), config))
{
}

Surface::~Surface()
{
  vkb::destroy_surface(GetDevice().GetInstance(), m_surface);
}

Surface::Surface(Surface && rhs) noexcept
  : OwnedBy<Device>(std::move(rhs))
{
  std::swap(m_surface, rhs.m_surface);
}

Surface & Surface::operator=(Surface && rhs) noexcept
{
  if (this != &rhs)
  {
    OwnedBy<Device>::operator=(std::move(rhs));
    std::swap(m_surface, rhs.m_surface);
  }
  return *this;
}
} // namespace RHI::vulkan
