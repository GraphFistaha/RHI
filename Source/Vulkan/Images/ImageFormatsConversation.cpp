#include "ImageFormatsConversation.hpp"

#include "ImageTraits.hpp"

namespace RHI::vulkan::utils
{
template<HostImageFormat srcFormat, VkFormat dstFormat,
         typename srcTexel = host_texel_type_t<srcFormat>,
         typename dstTexel = internal_texel_type_t<dstFormat>>
void CopyImageRowRegionFromHost(const srcTexel * srcRowRegionBegin, dstTexel * dstRowRegionBegin,
                                uint32_t texelsCount);

template<HostImageFormat srcFormat, VkFormat dstFormat,
         typename srcTexel = host_texel_type_t<srcFormat>,
         typename dstTexel = internal_texel_type_t<dstFormat>>
void CopyImageFromHost(const srcTexel * srcPixelData, const ImageExtent & srcExtent,
                       const ImageRegion & srcRegion, dstTexel * dstPixelData,
                       const ImageExtent & dstExtent, const ImageRegion & dstRegion)
{
  if (srcRegion.extent != dstRegion.extent)
    throw std::invalid_argument("Images have different extents. Copying is impossible");

  const uint32_t srcRowSize = srcExtent[0];
  const uint32_t dstRowSize = dstExtent[0];
  const size_t srcLayerSize = srcExtent[0] * srcExtent[1];
  const size_t dstLayerSize = dstExtent[0] * dstExtent[1];

  const srcTexel * src = srcPixelData + srcLayerSize * srcRegion.offset[2] +
                         srcRowSize * srcRegion.offset[1] + srcRegion.offset[0];
  dstTexel * dst = dstPixelData + dstLayerSize * dstRegion.offset[2] +
                   dstRowSize * dstRegion.offset[1] + dstRegion.offset[0];

  for (uint32_t l = 0, lc = srcRegion.extent[2]; l < lc; ++l)
  {
    for (uint32_t h = 0, hc = srcRegion.extent[1]; h < hc; ++h)
    {
      CopyImageRowRegionFromHost<srcFormat, dstFormat>(src, dst, srcRegion.extent[0]);
      src += srcRowSize;
      dst += dstRowSize;
    }
    src += srcLayerSize;
    dst += dstLayerSize;
  }
}

} // namespace RHI::vulkan::utils

namespace RHI::vulkan::utils
{
template<>
void CopyImageRowRegionFromHost<HostImageFormat::RGBA8, VK_FORMAT_R8G8B8A8_SRGB>(
  const uint32_t * srcRowRegionBegin, uint32_t * dstRowRegionBegin, uint32_t texelsCount)
{
  std::memcpy(dstRowRegionBegin, srcRowRegionBegin,
              static_cast<size_t>(texelsCount * host_texel_size_v<HostImageFormat::RGBA8>));
}

template<>
void CopyImageRowRegionFromHost<HostImageFormat::RGB8, VK_FORMAT_R8G8B8A8_SRGB>(
  const uint24_t * srcRowRegionBegin, uint32_t * dstRowRegionBegin, uint32_t texelsCount)
{
  for (uint32_t i = 0; i < texelsCount; ++i)
  {
    uint32_t texel = *reinterpret_cast<const uint32_t *>(srcRowRegionBegin + i);
    texel |= 0xFF000000;
    dstRowRegionBegin[i] = texel;
  }
}

} // namespace RHI::vulkan::utils

namespace RHI::vulkan::utils
{

void CopyImageFromHost(const uint8_t * srcPixelData, const ImageExtent & srcExtent,
                       const ImageRegion & srcRegion, HostImageFormat srcFormat,
                       uint8_t * dstPixelData, const ImageExtent & dstExtent,
                       const ImageRegion & dstRegion, VkFormat dstFormat)
{
  switch (srcFormat)
  {
    case HostImageFormat::R8:
    case HostImageFormat::A8:
    case HostImageFormat::RG8:
    case HostImageFormat::BGR8:
    case HostImageFormat::BGRA8:
      throw std::runtime_error("Image formats are not compatible");
    case HostImageFormat::RGB8:
    {
      if (dstFormat == VK_FORMAT_R8G8B8A8_SRGB)
        CopyImageFromHost<HostImageFormat::RGB8,
                          VK_FORMAT_R8G8B8A8_SRGB>(reinterpret_cast<const uint24_t *>(srcPixelData),
                                                   srcExtent, srcRegion,
                                                   reinterpret_cast<uint32_t *>(dstPixelData),
                                                   dstExtent, dstRegion);
      else
        throw std::runtime_error("Image formats are not compatible");
    }
    break;
    case HostImageFormat::RGBA8:
    {
      if (dstFormat == VK_FORMAT_R8G8B8A8_SRGB)
        CopyImageFromHost<HostImageFormat::RGBA8,
                          VK_FORMAT_R8G8B8A8_SRGB>(reinterpret_cast<const uint32_t *>(srcPixelData),
                                                   srcExtent, srcRegion,
                                                   reinterpret_cast<uint32_t *>(dstPixelData),
                                                   dstExtent, dstRegion);
      else
        throw std::runtime_error("Image formats are not compatible");
    }
    break;
  }
}

} // namespace RHI::vulkan::utils
