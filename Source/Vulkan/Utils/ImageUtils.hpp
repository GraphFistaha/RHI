#pragma once
#include <set>
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan::utils
{
constexpr size_t GetSizeOfTexel(VkFormat format) noexcept;
constexpr size_t GetSizeForImageRegion(const ImageRegion & region, VkFormat format) noexcept;

constexpr void CopyImageRegion_Host2GPU(const uint8_t * srcPixelData, const ImageRegion & srcRegion,
                              ImageFormat srcFormat, uint8_t * dstPixelData,
                              const ImageRegion & dstRegion, VkFormat internalFormat);

} // namespace RHI::vulkan::utils
