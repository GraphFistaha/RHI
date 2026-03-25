#pragma once
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan::utils
{
VkImageView CreateImageView(VkDevice device, VkImage image, VkFormat format, VkImageViewType type,
                            VkImageAspectFlags aspectFlags);

bool AreImageTypesCompatible(RHI::ImageType type, VkImageType vkType) noexcept;
bool AreImageTypesCompatible(RHI::ImageType type, VkImageViewType VkType) noexcept;

std::pair<VkExtent3D, uint32_t> UnpackExtentAndLayers(const TextureExtent & extent,
                                                      RHI::ImageType type) noexcept;
std::pair<VkExtent3D, uint32_t> UnpackExtentAndLayers(const TextureExtent & extent,
                                                      VkImageType type) noexcept;

std::pair<VkOffset3D, uint32_t> UnpackOffsetAndBaseLayer(const TexelIndex & offset,
                                                         RHI::ImageType type) noexcept;
std::pair<VkOffset3D, uint32_t> UnpackOffsetAndBaseLayer(const TexelIndex & offset,
                                                         VkImageType type) noexcept;
} // namespace RHI::vulkan::utils
