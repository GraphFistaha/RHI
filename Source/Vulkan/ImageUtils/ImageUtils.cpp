#include "ImageUtils.hpp"

namespace RHI::vulkan::utils
{
VkImageView CreateImageView(VkDevice device, VkImage image, VkFormat format, VkImageViewType type,
                            VkImageAspectFlags aspectFlags)
{
  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = image;
  viewInfo.viewType = type;
  viewInfo.format = format;
  viewInfo.subresourceRange.aspectMask = aspectFlags;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

  VkImageView view;
  if (auto res = vkCreateImageView(device, &viewInfo, nullptr, &view); res != VK_SUCCESS)
    throw std::invalid_argument("Failed to create image view!");

  return view;
}

bool AreImageTypesCompatible(RHI::ImageType type, VkImageType vkType) noexcept
{
  switch (type)
  {
    case RHI::ImageType::Image1D:
    case RHI::ImageType::Image1D_Array:
      return vkType == VK_IMAGE_TYPE_1D;
    case RHI::ImageType::Image2D:
    case RHI::ImageType::Image2D_Array:
    case RHI::ImageType::Cubemap:
      return vkType == VK_IMAGE_TYPE_2D;
    case RHI::ImageType::Image3D:
      return vkType == VK_IMAGE_TYPE_3D;
  }
  return false;
}

bool AreImageTypesCompatible(RHI::ImageType type, VkImageViewType vkType) noexcept
{
  switch (type)
  {
    case RHI::ImageType::Image1D:
      return vkType == VK_IMAGE_VIEW_TYPE_1D;
    case RHI::ImageType::Image1D_Array:
      return vkType == VK_IMAGE_VIEW_TYPE_1D_ARRAY;
    case RHI::ImageType::Image2D:
      return vkType == VK_IMAGE_VIEW_TYPE_2D;
    case RHI::ImageType::Image2D_Array:
      return vkType == VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    case RHI::ImageType::Cubemap:
      return vkType == VK_IMAGE_VIEW_TYPE_CUBE;
    case RHI::ImageType::Image3D:
      return vkType == VK_IMAGE_VIEW_TYPE_3D;
  }
  return false;
}


std::pair<VkExtent3D, uint32_t> UnpackExtentAndLayers(const TextureExtent & extent,
                                                      RHI::ImageType type) noexcept
{
  VkExtent3D resultExtent{static_cast<uint32_t>(extent[0]), static_cast<uint32_t>(extent[1]),
                          static_cast<uint32_t>(extent[2])};
  uint32_t layersCount = 1;
  switch (type)
  {
    case RHI::ImageType::Image1D:
    case RHI::ImageType::Image1D_Array:
      layersCount = resultExtent.height;
      resultExtent.height = 1;
      resultExtent.depth = 1;
      break;
    case RHI::ImageType::Image2D:
    case RHI::ImageType::Image2D_Array:
      layersCount = resultExtent.depth;
      resultExtent.depth = 1;
      break;
  }

  return {resultExtent, layersCount};
}

std::pair<VkExtent3D, uint32_t> UnpackExtentAndLayers(const TextureExtent & extent,
                                                      VkImageType type) noexcept
{
  VkExtent3D resultExtent{static_cast<uint32_t>(extent[0]), static_cast<uint32_t>(extent[1]),
                          static_cast<uint32_t>(extent[2])};
  uint32_t layersCount = 1;
  switch (type)
  {
    case VK_IMAGE_TYPE_1D:
      layersCount = resultExtent.height;
      resultExtent.height = 1;
      resultExtent.depth = 1;
      break;
    case VK_IMAGE_TYPE_2D:
      layersCount = resultExtent.depth;
      resultExtent.depth = 1;
      break;
  }

  return {resultExtent, layersCount};
}

std::pair<VkOffset3D, uint32_t> UnpackOffsetAndBaseLayer(const TexelIndex & offset,
                                                         RHI::ImageType type) noexcept
{
  VkOffset3D resultOffset{static_cast<int32_t>(offset[0]), static_cast<int32_t>(offset[1]),
                          static_cast<int32_t>(offset[2])};
  uint32_t baseLayer = 0;
  switch (type)
  {
    case RHI::ImageType::Image1D:
    case RHI::ImageType::Image1D_Array:
      baseLayer = resultOffset.y;
      resultOffset.y = 0;
      resultOffset.z = 0;
      break;
    case RHI::ImageType::Image2D:
    case RHI::ImageType::Image2D_Array:
      baseLayer = resultOffset.z;
      resultOffset.z = 0;
      break;
  }

  return {resultOffset, baseLayer};
}

std::pair<VkOffset3D, uint32_t> UnpackOffsetAndBaseLayer(const TexelIndex & offset,
                                                         VkImageType type) noexcept
{
  VkOffset3D resultOffset{static_cast<int32_t>(offset[0]), static_cast<int32_t>(offset[1]),
                          static_cast<int32_t>(offset[2])};
  uint32_t baseLayer = 0;
  switch (type)
  {
    case VK_IMAGE_TYPE_1D:
      baseLayer = resultOffset.y;
      resultOffset.y = 0;
      resultOffset.z = 0;
      break;
    case VK_IMAGE_TYPE_2D:
      baseLayer = resultOffset.z;
      resultOffset.z = 0;
      break;
  }

  return {resultOffset, baseLayer};
}

} // namespace RHI::vulkan::utils
