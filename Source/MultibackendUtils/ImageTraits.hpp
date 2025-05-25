#pragma once
#include <cstdint>
#include <unordered_map>

#include <RHI.hpp>

#include "Types.hpp"

// Table with all host image formats and their mapping of C types
#define FOR_EACH_HOST_IMAGE_FORMAT(IMAGE_FORMAT_MACRO)                                             \
  IMAGE_FORMAT_MACRO(RHI::HostImageFormat, RHI::HostImageFormat::RGBA8, uint32_t)                  \
  IMAGE_FORMAT_MACRO(RHI::HostImageFormat, RHI::HostImageFormat::A8, uint8_t)                      \
  IMAGE_FORMAT_MACRO(RHI::HostImageFormat, RHI::HostImageFormat::R8, uint8_t)                      \
  IMAGE_FORMAT_MACRO(RHI::HostImageFormat, RHI::HostImageFormat::RG8, uint16_t)                    \
  IMAGE_FORMAT_MACRO(RHI::HostImageFormat, RHI::HostImageFormat::RGB8, RHI::utils::char24_t)       \
  IMAGE_FORMAT_MACRO(RHI::HostImageFormat, RHI::HostImageFormat::BGR8, RHI::utils::char24_t)       \
  IMAGE_FORMAT_MACRO(RHI::HostImageFormat, RHI::HostImageFormat::BGRA8, uint32_t)

namespace RHI::utils
{
/// Contains static parameters for texel by its image format
template<typename FormatT, FormatT format>
struct TexelTrait
{
  using type =
    void; ///< C type of texel (f.e. uint32_t fot RGBA8, or char for one-channel 8bit image)
  static constexpr uint32_t size = 1;                    ///< sizeof(type)
  static constexpr const char * readable_name = nullptr; ///< c-string for that image format
};

/// Get C type for texel by vulkan image format (f.e. uint32 for RGBA8)
template<typename FormatT, FormatT format>
using texel_type_t = typename TexelTrait<FormatT, format>::type;

/// Get size of texel's C type (for vulkan image format)
template<typename FormatT, FormatT format>
static constexpr uint32_t texel_size_v = TexelTrait<FormatT, format>::size;

/// Get size of texel by it's format
template<typename FormatT>
inline uint32_t GetSizeOfTexel(FormatT format) noexcept;

template<>
inline uint32_t GetSizeOfTexel<HostImageFormat>(HostImageFormat format) noexcept
{
#define INIT_MAP_WITH_IMAGE_FORMAT(format_type, format_value, c_type)                              \
  {format_value, static_cast<uint32_t>(sizeof(c_type))},

  static const std::unordered_map<HostImageFormat, uint32_t> map = {
    FOR_EACH_HOST_IMAGE_FORMAT(INIT_MAP_WITH_IMAGE_FORMAT)};
  auto it = map.find(format);
  if (it == map.end())
    return 0;
  return it->second;
#undef INIT_MAP_WITH_IMAGE_FORMAT
}

/// Get size of image
template<typename FormatT>
inline size_t GetSizeOfImage(const TexelIndex & extent, FormatT format) noexcept
{
  return extent[0] * extent[1] * extent[2] * utils::GetSizeOfTexel<FormatT>(format);
}

} // namespace RHI::utils

#define DECLARE_IMAGE_FORMAT(format_type, format_value, c_type)                                    \
  template<>                                                                                       \
  struct RHI::utils::TexelTrait<format_type, format_value>                                 \
  {                                                                                                \
    using type = c_type;                                                                           \
    static constexpr uint32_t size = sizeof(c_type);                                               \
    static constexpr const char * readable_name = #format_value;                                   \
  };

FOR_EACH_HOST_IMAGE_FORMAT(DECLARE_IMAGE_FORMAT)
