#pragma once
#include <array>
#include <format>
#include <string>
#include <unordered_map>

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "../Utils/Types.hpp"
#include "ImageFormats.hpp"

namespace RHI::vulkan::utils
{
namespace details
{
template<typename FormatT, FormatT format>
struct InternalTexelTrait
{
  using type = void;
  static constexpr uint32_t size = 1;
  static constexpr const char * readable_name = nullptr;
};
} // namespace details

template<VkFormat internal_format>
using internal_texel_type_t = typename details::InternalTexelTrait<VkFormat, internal_format>::type;

template<VkFormat internal_format>
static constexpr uint32_t internal_texel_size_v =
  details::InternalTexelTrait<VkFormat, internal_format>::size;

template<HostImageFormat format>
using host_texel_type_t = typename details::InternalTexelTrait<HostImageFormat, format>::type;

template<HostImageFormat format>
static constexpr uint32_t host_texel_size_v =
  details::InternalTexelTrait<HostImageFormat, format>::size;

template<typename FormatT, typename = std::enable_if_t<std::is_same_v<FormatT, VkFormat> ||
                                                       std::is_same_v<FormatT, HostImageFormat>>>
constexpr uint32_t GetSizeOfTexel(FormatT format)
{
#define INIT_MAP_WITH_IMAGE_FORMAT(format_type, format_value, c_type)                              \
  {format_value, static_cast<uint32_t>(sizeof(c_type))},

  if constexpr (std::is_same_v<FormatT, VkFormat>)
  {
    static const std::unordered_map<VkFormat, uint32_t> map = {
      FOR_EACH_VULKAN_IMAGE_FORMAT(INIT_MAP_WITH_IMAGE_FORMAT)};
    auto it = map.find(format);
    if (it == map.end())
      throw std::invalid_argument(
        std::format("GetSizeOfTexel: VkFormat({}) is not allowed to use as InternalImageFormat",
                    static_cast<uint32_t>(format)));
    return it->second;
  }
  else if constexpr (std::is_same_v<FormatT, HostImageFormat>)
  {
    static const std::unordered_map<VkFormat, uint32_t> map = {
      FOR_EACH_HOST_IMAGE_FORMAT(INIT_MAP_WITH_IMAGE_FORMAT)};
    auto it = map.find(format);
    if (it == map.end())
      throw std::invalid_argument("HostImageFormat({}) is not implemented in GetSizeOfTexel",
                                  static_cast<uint32_t>(format));
    return it->second;
  }
#undef INIT_MAP_WITH_IMAGE_FORMAT
}

template<typename FormatT>
constexpr size_t GetSizeOfImage(const ImageExtent & extent, FormatT format)
{
  return extent[0] * extent[1] * extent[2] * GetSizeOfTexel<FormatT>(format);
}

template<typename FormatT>
constexpr size_t GetSizeOfImage(const VkExtent3D & extent, FormatT format)
{
  return extent.width * extent.height * extent.depth * GetSizeOfTexel<FormatT>(format);
}

} // namespace RHI::vulkan::utils


namespace RHI::vulkan::utils
{
#define DECLARE_IMAGE_FORMAT(format_type, format_value, c_type)                                    \
  template<>                                                                                       \
  struct details::InternalTexelTrait<format_type, format_value>                                    \
  {                                                                                                \
    using type = c_type;                                                                           \
    static constexpr uint32_t size = sizeof(c_type);                                               \
    static constexpr const char * readable_name = #format_value;                                   \
  };

FOR_EACH_VULKAN_IMAGE_FORMAT(DECLARE_IMAGE_FORMAT)
FOR_EACH_HOST_IMAGE_FORMAT(DECLARE_IMAGE_FORMAT)

#undef DECLARE_TEXEL_TYPE
} // namespace RHI::vulkan::utils
