#pragma once
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan
{

void CopyImageFromHost(const uint8_t * srcPixelData, const ImageExtent & srcExtent,
                       const ImageRegion & srcRegion, HostImageFormat srcFormat,
                       uint8_t * dstPixelData, const ImageExtent & dstExtent,
                       const ImageRegion & dstRegion, VkFormat dstFormat);

void CopyImageToHost(const uint8_t * srcPixelData, const ImageExtent & srcExtent,
                     const ImageRegion & srcRegion, VkFormat srcFormat, uint8_t * dstPixelData,
                     const ImageExtent & dstExtent, const ImageRegion & dstRegion,
                     HostImageFormat dstFormat);

} // namespace RHI::vulkan::utils
