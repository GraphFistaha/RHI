#pragma once
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan
{

void CopyImageFromHost(const uint8_t * srcPixelData, const TexelIndex & srcExtent,
                       const TextureRegion & srcRegion, HostImageFormat srcFormat,
                       uint8_t * dstPixelData, const TexelIndex & dstExtent,
                       const TextureRegion & dstRegion, VkFormat dstFormat);

void CopyImageToHost(const uint8_t * srcPixelData, const TexelIndex & srcExtent,
                     const TextureRegion & srcRegion, VkFormat srcFormat, uint8_t * dstPixelData,
                     const TexelIndex & dstExtent, const TextureRegion & dstRegion,
                     HostImageFormat dstFormat);

} // namespace RHI::vulkan::utils
