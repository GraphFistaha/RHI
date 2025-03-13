#include "ImageInfo.hpp"

#include "../Utils/CastHelper.hpp"

namespace
{
constexpr VkImageLayout MakeImageInitialLayout(RHI::ImageFormat format) noexcept
{
  return VK_IMAGE_LAYOUT_UNDEFINED;
}

constexpr VkImageUsageFlags MakeImageUsage(RHI::ImageFormat format)
{
  //Здесь перечисляются все возможные использования изображения, в которые может попасть изображение
  // однако стоит помнить, что в один момент времени
  switch (format)
  {
    case RHI::ImageFormat::A8:
    case RHI::ImageFormat::R8:
    case RHI::ImageFormat::RG8:
    case RHI::ImageFormat::RGB8:
    case RHI::ImageFormat::RGBA8:
    case RHI::ImageFormat::BGR8:
    case RHI::ImageFormat::BGRA8:
      return VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
             VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
             VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
    case RHI::ImageFormat::DEPTH:
    case RHI::ImageFormat::DEPTH_STENCIL:
      return VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
             VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
    default:
      return VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM;
  }
}

constexpr VkImageLayout MakeAttachmentInitialLayout(RHI::ImageFormat format)
{
  return VK_IMAGE_LAYOUT_UNDEFINED;
}

constexpr VkImageLayout MakeAttachmentFinalLayout(RHI::ImageFormat format)
{
  switch (format)
  {
    case RHI::ImageFormat::A8:
    case RHI::ImageFormat::R8:
    case RHI::ImageFormat::RG8:
    case RHI::ImageFormat::RGB8:
    case RHI::ImageFormat::RGBA8:
    case RHI::ImageFormat::BGR8:
    case RHI::ImageFormat::BGRA8:
      return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    case RHI::ImageFormat::DEPTH:
      return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    case RHI::ImageFormat::DEPTH_STENCIL:
      return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    default:
      return VK_IMAGE_LAYOUT_UNDEFINED;
  }
}

} // namespace

namespace RHI::vulkan
{


VkAttachmentDescription BuildAttachmentDescription(
  const ImageCreateArguments & description) noexcept
{
  VkAttachmentDescription attachmentDescription{};
  {
    attachmentDescription.format = utils::CastInterfaceEnum2Vulkan<VkFormat>(description.format);
    attachmentDescription.samples =
      utils::CastInterfaceEnum2Vulkan<VkSampleCountFlagBits>(description.samples);
    attachmentDescription.initialLayout = MakeAttachmentInitialLayout(description.format);
    attachmentDescription.finalLayout = MakeAttachmentFinalLayout(description.format);
    attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
  }
  return attachmentDescription;
}

VkImageCreateInfo BuildImageCreateInfo(const ImageCreateArguments & description) noexcept
{
  VkImageCreateInfo imageInfo{};
  {
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = utils::CastInterfaceEnum2Vulkan<VkImageType>(description.type);
    imageInfo.extent.width = description.extent[0];
    imageInfo.extent.height = description.extent[1];
    imageInfo.extent.depth = description.extent[2];
    imageInfo.mipLevels = description.mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = utils::CastInterfaceEnum2Vulkan<VkFormat>(description.format);
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = MakeImageInitialLayout(description.format);
    imageInfo.usage = MakeImageUsage(description.format);
    imageInfo.samples = utils::CastInterfaceEnum2Vulkan<VkSampleCountFlagBits>(description.samples);
    imageInfo.sharingMode = description.shared ? VK_SHARING_MODE_CONCURRENT
                                               : VK_SHARING_MODE_EXCLUSIVE;
  }

  return imageInfo;
}
void TransferImageLayout(details::CommandBuffer & commandBuffer, VkImageLayout prevImageLayout,
                         VkImageLayout newLayout, VkImage image)
{

}
} // namespace RHI::vulkan
