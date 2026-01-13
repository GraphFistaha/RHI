#pragma once
#include <Private/ImageTraits.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

// Table with all vulkan image formats and their mapping of C types
#define FOR_EACH_VULKAN_IMAGE_FORMAT(IMAGE_FORMAT_MACRO)                                           \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8G8B8A8_SRGB, uint32_t)                                  \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8G8B8A8_SNORM, uint32_t)                                 \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8G8B8A8_UNORM, uint32_t)                                 \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8G8B8A8_SINT, uint32_t)                                  \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8G8B8A8_UINT, uint32_t)                                  \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8G8B8A8_SSCALED, uint32_t)                               \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8G8B8A8_USCALED, uint32_t)                               \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8G8B8_SRGB, RHI::utils::char24_t)                        \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8G8B8_SNORM, RHI::utils::char24_t)                       \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8G8B8_UNORM, RHI::utils::char24_t)                       \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8G8B8_SINT, RHI::utils::char24_t)                        \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8G8B8_UINT, RHI::utils::char24_t)                        \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8G8B8_SSCALED, RHI::utils::char24_t)                     \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8G8B8_USCALED, RHI::utils::char24_t)                     \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8G8_SRGB, uint16_t)                                      \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8G8_SNORM, uint16_t)                                     \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8G8_UNORM, uint16_t)                                     \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8G8_SINT, uint16_t)                                      \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8G8_UINT, uint16_t)                                      \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8G8_SSCALED, uint16_t)                                   \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8G8_USCALED, uint16_t)                                   \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8_SRGB, uint8_t)                                         \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8_SNORM, uint8_t)                                        \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8_UNORM, uint8_t)                                        \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8_SINT, uint8_t)                                         \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8_UINT, uint8_t)                                         \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8_SSCALED, uint8_t)                                      \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8_USCALED, uint8_t)

FOR_EACH_VULKAN_IMAGE_FORMAT(DECLARE_IMAGE_FORMAT)

namespace RHI::utils
{
template<>
inline uint32_t GetSizeOfTexel<VkFormat>(VkFormat format) noexcept
{
#define INIT_MAP_WITH_IMAGE_FORMAT(format_type, format_value, c_type)                              \
  {format_value, static_cast<uint32_t>(sizeof(c_type))},
  static const std::unordered_map<VkFormat, uint32_t> map = {
    FOR_EACH_VULKAN_IMAGE_FORMAT(INIT_MAP_WITH_IMAGE_FORMAT)};
  auto it = map.find(format);
  if (it == map.end())
    return 0;
  return it->second;
#undef INIT_MAP_WITH_IMAGE_FORMAT
}

/// Get size of image
template<typename FormatT>
inline size_t GetSizeOfImage(const VkExtent3D & extent, FormatT format) noexcept
{
  return extent.width * extent.height * extent.depth * GetSizeOfTexel<FormatT>(format);
}

} // namespace RHI::utils
