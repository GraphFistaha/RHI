#pragma once
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan::utils
{
void CopyImageFromHost(const uint8_t * srcPixelData, const ImageExtent & srcExtent,
                       const ImageRegion & srcRegion, HostImageFormat srcFormat,
                       uint8_t * dstPixelData, const ImageExtent & dstExtent,
                       const ImageRegion & dstRegion, VkFormat dstFormat);
}
