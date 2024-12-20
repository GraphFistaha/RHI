#include "ImageUtils.hpp"

namespace RHI::vulkan::utils
{

constexpr size_t GetSizeOfTexel(VkFormat format) noexcept
{
  switch (format)
  {
    case VK_FORMAT_R8G8B8A8_SRGB:
    case VK_FORMAT_R8G8B8A8_SNORM:
    case VK_FORMAT_R8G8B8A8_UNORM:
    case VK_FORMAT_R8G8B8A8_SINT:
    case VK_FORMAT_R8G8B8A8_UINT:
    case VK_FORMAT_R8G8B8A8_SSCALED:
    case VK_FORMAT_R8G8B8A8_USCALED:
      return 4;
    case VK_FORMAT_R8G8B8_SRGB:
    case VK_FORMAT_R8G8B8_SNORM:
    case VK_FORMAT_R8G8B8_UNORM:
    case VK_FORMAT_R8G8B8_SINT:
    case VK_FORMAT_R8G8B8_UINT:
    case VK_FORMAT_R8G8B8_SSCALED:
    case VK_FORMAT_R8G8B8_USCALED:
      return 3;
  }
}

constexpr size_t GetSizeForImageRegion(const ImageRegion & region, VkFormat format) noexcept
{
  return region.width * region.height * region.depth * GetSizeOfTexel(format);
}

constexpr void CopyImageRegion_Host2GPU(const uint8_t * srcPixelData, const ImageRegion & srcRegion,
                              ImageFormat srcFormat, uint8_t * dstPixelData,
                              const ImageRegion & dstRegion, VkFormat internalFormat)
{
  
}


} // namespace RHI::vulkan::utils
