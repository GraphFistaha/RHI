#pragma once
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

inline float Color_Byte2Float(uint8_t channel) noexcept
{
  return static_cast<float>(channel) / 255.0f;
}

inline uint8_t Color_Float2Byte(float channel) noexcept
{
  return static_cast<uint8_t>(channel * 255.0f);
}

namespace RHI::vulkan::utils
{
template<ImageFormat srcFormat, VkFormat dstInternalFormat>
inline void CopyTexel(const uint8_t * srcPixelData, uint8_t * dstPixelData) noexcept;

} // namespace RHI::vulkan::utils

namespace RHI::vulkan::utils
{

template<>
void CopyTexel<ImageFormat::A8, VK_FORMAT_R8_SRGB>(const uint8_t * srcPixelData,
                                                   uint8_t * dstPixelData) noexcept
{
  *dstPixelData = *srcPixelData;
}

template<>
void CopyTexel<ImageFormat::R8, VK_FORMAT_R8_SRGB>(const uint8_t * srcPixelData,
                                                   uint8_t * dstPixelData) noexcept
{
  *dstPixelData = *srcPixelData;
}

template<>
void CopyTexel<ImageFormat::RG8, VK_FORMAT_R8G8B8A8_SRGB>(const uint8_t * srcPixelData,
                                                          uint8_t * dstPixelData) noexcept
{
  std::memcpy(dstPixelData, srcPixelData, 2);
  dstPixelData[2] = 0;
  dstPixelData[3] = 0xFF;
}

template<>
void CopyTexel<ImageFormat::RGB8, VK_FORMAT_R8G8B8A8_SRGB>(const uint8_t * srcPixelData,
                                                           uint8_t * dstPixelData) noexcept
{
  std::memcpy(dstPixelData, srcPixelData, 3);
  dstPixelData[3] = 0xFF;
}

template<>
void CopyTexel<ImageFormat::BGR8, VK_FORMAT_R8G8B8A8_SRGB>(const uint8_t * srcPixelData,
                                                           uint8_t * dstPixelData) noexcept
{
  dstPixelData[0] = srcPixelData[2];
  dstPixelData[1] = srcPixelData[1];
  dstPixelData[2] = srcPixelData[0];
  dstPixelData[3] = 0xFF;
}

template<>
void CopyTexel<ImageFormat::RGBA8, VK_FORMAT_R8G8B8A8_SRGB>(const uint8_t * srcPixelData,
                                                            uint8_t * dstPixelData) noexcept
{
  std::memcpy(dstPixelData, srcPixelData, 4);
}

template<>
void CopyTexel<ImageFormat::DEPTH, VK_FORMAT_D32_SFLOAT>(const uint8_t * srcPixelData,
                                                         uint8_t * dstPixelData) noexcept
{
  std::memcpy(dstPixelData, srcPixelData, 4);
}


} // namespace RHI::vulkan::utils
