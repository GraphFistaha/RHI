#pragma once
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan
{

struct MappedGpuTextureView final
{
  uint8_t * pixelData;
  TextureExtent extent;
  VkFormat format;
  uint32_t baseLayerIndex = 0;
  uint32_t layersCount = 1;
};

/// @brief copies host image to staging buffer of gpu's image
/// @param hostTexture - host's texture data
/// @param gpuTexture - gpu's texture data (mapped to RAM)
/// @param copyRegion - copy region (in range of host's image extent)
/// @param dstOffset - offset in gpuTexture's extent where to copy data
void CopyImageFromHost(const HostTextureView & hostTexture, const MappedGpuTextureView & gpuTexture,
                       const TextureRegion & copyRegion, const TexelIndex & dstOffset = {0, 0, 0});

void CopyImageToHost(const MappedGpuTextureView & gpuTexture, const HostTextureView & hostTexture,
                     const TextureRegion & copyRegion, const TexelIndex & dstOffset = {0, 0, 0});

} // namespace RHI::vulkan
