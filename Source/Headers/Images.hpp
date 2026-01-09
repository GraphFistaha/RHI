#pragma once
#include <array>
#include <cstdint>

#include "Utils.hpp"

namespace RHI
{

/// @brief Defines image's layout in memory. Also defines if image can have mipmaps or not
///        For example image1d is line of pixels, image2d is a rectangle of pixels
enum class ImageType
{
  Image1D, ///< image with height = 1. has only width.Can have one mipmap for a whole image
  Image2D, ///< generic image with width and height. Can have one mipmap for a whole image
  Image3D, ///< layered image2d (with depth). Can have one mipmap for a whole image
  Image1D_Array, ///< array of 1d images, layered in memory like 2d image, but each row of pixels has its own mipmap
  Image2D_Array, ///< the same as image3d, but each layer should have own mipmap
  Cubemap,       ///< it's image2d_array with length = 6
};

/// @brief Defines samples count per pixel in image
enum class SamplesCount : uint8_t
{
  One = 1,
  Two = 2,
  Four = 4,
  Eight = 8,
};

/// @brief Defines in what format image will be uploaded or downloaded
enum class HostImageFormat : uint8_t
{
  R8,
  A8,
  RG8,
  BGR8,
  RGB8,
  RGBA8,
  BGRA8,
  // compressed types
  //BC1,
  //BC5,
  //BC7
};

/// @brief internal image format
// TODO: Remove
enum class ImageFormat : uint8_t
{
  UNDEFINED,
  // general formats
  R8,
  A8,
  RG8,
  BGR8,
  RGB8,
  RGBA8,
  BGRA8,
  // service formats
  DEPTH,
  DEPTH_STENCIL,
  // compressed types
  //BC1,
  //BC5,
  //BC7
};

/// @brief
enum ShaderAttachmentSlot
{
  Color,            ///< color attachment
  DepthStencil,     ///< depth-stencil attachment
  Input,            ///< read-only shader attachment
  Preserved = 0x80, ///< flag, that image just stored and unused in shader
  TOTAL
};

/// For Image1D used only 0'th index
/// For Image2D used only 0 and 1 index
/// For Image3D used all 3 indices like width, height and layer
/// For Image1D used 0 and 1 index as width and i-th array element
/// For Image2D used all 3 indices as width, height and i-th array element
/// For Cubemap used all 3 indices as width, height and i-th surface of cube
using TexelIndex = std::array<uint32_t, 3>;

/// Gabarit of texture
using TextureExtent = TexelIndex;

struct TextureRegion
{
  TexelIndex offset;
  TextureExtent extent;
};


struct TextureDescription final
{
  TextureExtent extent;
  uint32_t mipLevels = 1;
  uint32_t layersCount = 1;
  ImageType type;
  ImageFormat format;
};

struct HostTextureView final
{
  TextureExtent extent;
  uint8_t * pixelData = nullptr;
  HostImageFormat format;
  uint32_t layersCount = 1;
};

} // namespace RHI
