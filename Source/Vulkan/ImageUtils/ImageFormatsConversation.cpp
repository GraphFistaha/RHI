#include "ImageFormatsConversation.hpp"

#include "InternalImageTraits.hpp"

namespace RHI::vulkan::utils
{
template<typename SrcFormatT, typename DstFormatT, SrcFormatT srcFormat, DstFormatT dstFormat,
         typename srcTexel = RHI::utils::texel_type_t<SrcFormatT, srcFormat>,
         typename dstTexel = RHI::utils::texel_type_t<DstFormatT, dstFormat>>
void CopyTexelsArray(const srcTexel * src, dstTexel * dst, uint32_t texelsCount) noexcept = delete;

template<typename SrcFormatT, typename DstFormatT, SrcFormatT srcFormat, DstFormatT dstFormat,
         typename srcTexel = RHI::utils::texel_type_t<SrcFormatT, srcFormat>,
         typename dstTexel = RHI::utils::texel_type_t<DstFormatT, dstFormat>>
void CopyImage(const srcTexel * srcPixelData, const TexelIndex & srcExtent,
               const TextureRegion & copyRegion, dstTexel * dstPixelData,
               const TexelIndex & dstExtent, const TexelIndex & dstOffset)
{
  const uint32_t srcRowSize = srcExtent[0];
  const uint32_t dstRowSize = dstExtent[0];
  const size_t srcLayerSize = srcExtent[0] * srcExtent[1];
  const size_t dstLayerSize = dstExtent[0] * dstExtent[1];

  const srcTexel * src = srcPixelData + srcLayerSize * copyRegion.offset[2] +
                         srcRowSize * copyRegion.offset[1] + copyRegion.offset[0];
  dstTexel * dst =
    dstPixelData + dstLayerSize * dstOffset[2] + dstRowSize * dstOffset[1] + dstOffset[0];

  for (uint32_t l = 0, lc = copyRegion.extent[2]; l < lc; ++l)
  {
    for (uint32_t h = 0, hc = copyRegion.extent[1]; h < hc; ++h)
    {
      CopyTexelsArray<SrcFormatT, DstFormatT, srcFormat, dstFormat>(src, dst, copyRegion.extent[0]);
      src += srcRowSize;
      dst += dstRowSize;
    }
    src += srcLayerSize;
    dst += dstLayerSize;
  }
}

} // namespace RHI::vulkan::utils

namespace RHI::vulkan
{

void CopyImageFromHost(const HostTextureView & hostTexture, const MappedGpuTextureView & gpuTexture,
                       const TextureRegion & copyRegion,
                       const TexelIndex & dstOffset /* = { 0, 0, 0 }*/)
{
  switch (hostTexture.format)
  {
    case HostImageFormat::R8:
    case HostImageFormat::A8:
    case HostImageFormat::RG8:
    case HostImageFormat::BGR8:
    case HostImageFormat::BGRA8:
      throw std::runtime_error("Image formats are not compatible");
    case HostImageFormat::RGB8:
    {
      if (gpuTexture.format == VK_FORMAT_R8G8B8A8_SRGB)
        utils::CopyImage<HostImageFormat, VkFormat, HostImageFormat::RGB8,
                         VK_FORMAT_R8G8B8A8_SRGB>(reinterpret_cast<const RHI::utils::uint24_t *>(
                                                    hostTexture.pixelData),
                                                  hostTexture.extent, copyRegion,
                                                  reinterpret_cast<uint32_t *>(
                                                    gpuTexture.pixelData),
                                                  gpuTexture.extent, dstOffset);
      else
        throw std::runtime_error("Image formats are not compatible");
    }
    break;
    case HostImageFormat::RGBA8:
    {
      if (gpuTexture.format == VK_FORMAT_R8G8B8A8_SRGB)
        utils::CopyImage<HostImageFormat, VkFormat, HostImageFormat::RGBA8,
                         VK_FORMAT_R8G8B8A8_SRGB>(reinterpret_cast<const uint32_t *>(
                                                    hostTexture.pixelData),
                                                  hostTexture.extent, copyRegion,
                                                  reinterpret_cast<uint32_t *>(
                                                    gpuTexture.pixelData),
                                                  gpuTexture.extent, dstOffset);
      else
        throw std::runtime_error("Image formats are not compatible");
    }
    break;
  }
}

void CopyImageToHost(const MappedGpuTextureView & gpuTexture, const HostTextureView & hostTexture,
                     const TextureRegion & copyRegion, const TexelIndex & dstOffset)
{
  switch (hostTexture.format)
  {
    case HostImageFormat::R8:
    case HostImageFormat::A8:
    case HostImageFormat::RG8:
    case HostImageFormat::BGR8:
    case HostImageFormat::BGRA8:
      throw std::runtime_error("Image formats are not compatible");
    case HostImageFormat::RGB8:
    {
      if (gpuTexture.format == VK_FORMAT_R8G8B8A8_SRGB)
      {
        utils::CopyImage<VkFormat, HostImageFormat, VK_FORMAT_R8G8B8A8_SRGB,
                         HostImageFormat::RGB8>(reinterpret_cast<const uint32_t *>(
                                                  gpuTexture.pixelData),
                                                gpuTexture.extent, copyRegion,
                                                reinterpret_cast<RHI::utils::uint24_t *>(
                                                  hostTexture.pixelData),
                                                hostTexture.extent, dstOffset);
      }
      else
      {
        throw std::runtime_error("Image formats are not compatible");
      }
    }
    break;
    case HostImageFormat::RGBA8:
    {
      if (gpuTexture.format == VK_FORMAT_R8G8B8A8_SRGB)
      {
        utils::CopyImage<VkFormat, HostImageFormat, VK_FORMAT_R8G8B8A8_SRGB,
                         HostImageFormat::RGBA8>(reinterpret_cast<const uint32_t *>(
                                                   gpuTexture.pixelData),
                                                 gpuTexture.extent, copyRegion,
                                                 reinterpret_cast<uint32_t *>(
                                                   hostTexture.pixelData),
                                                 hostTexture.extent, dstOffset);
      }
      else
      {
        throw std::runtime_error("Image formats are not compatible");
      }
    }
    break;
  }
}

} // namespace RHI::vulkan

// ============================= Specializations ===================================


#define COPY_TEXELS_ARRAY(SrcFormatType, DstFormatType, SrcFormatValue, DstFormatValue,            \
                          SrcArrVarName, DstArrVarName, TexelsCountVarName)                        \
  template<>                                                                                       \
  void RHI::vulkan::utils::CopyTexelsArray<                                                        \
    SrcFormatType, DstFormatType, SrcFormatValue,                                                  \
    DstFormatValue>(const RHI::utils::texel_type_t<SrcFormatType, SrcFormatValue> * SrcArrVarName, \
                    RHI::utils::texel_type_t<DstFormatType, DstFormatValue> * DstArrVarName,       \
                    uint32_t TexelsCountVarName) noexcept

COPY_TEXELS_ARRAY(RHI::HostImageFormat, VkFormat, RHI::HostImageFormat::RGBA8,
                  VK_FORMAT_R8G8B8A8_SRGB, src, dst, texelsCount)
{
  std::memcpy(dst, src,
              static_cast<size_t>(
                texelsCount * RHI::utils::texel_size_v<HostImageFormat, HostImageFormat::RGBA8>));
}

COPY_TEXELS_ARRAY(RHI::HostImageFormat, VkFormat, RHI::HostImageFormat::RGB8,
                  VK_FORMAT_R8G8B8A8_SRGB, src, dst, texelsCount)
{
  for (uint32_t i = 0; i < texelsCount; ++i)
  {
    uint32_t texel = *reinterpret_cast<const uint32_t *>(src + i);
    texel |= 0xFF000000;
    dst[i] = texel;
  }
}

COPY_TEXELS_ARRAY(VkFormat, RHI::HostImageFormat, VK_FORMAT_R8G8B8A8_SRGB,
                  RHI::HostImageFormat::RGBA8, src, dst, texelsCount)
{
  std::memcpy(dst, src,
              static_cast<size_t>(texelsCount *
                                  RHI::utils::texel_size_v<VkFormat, VK_FORMAT_R8G8B8A8_SRGB>));
}

COPY_TEXELS_ARRAY(VkFormat, RHI::HostImageFormat, VK_FORMAT_R8G8B8A8_SRGB,
                  RHI::HostImageFormat::RGB8, src, dst, texelsCount)
{
  for (uint32_t i = 0; i < texelsCount; ++i)
    dst[i] = src[i];
}

#undef COPY_TEXELS_ARRAY
