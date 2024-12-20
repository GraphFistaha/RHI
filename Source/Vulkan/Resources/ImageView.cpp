#include "ImageView.hpp"

#include "../Utils/CastHelper.hpp"
#include "../VulkanContext.hpp"
#include "ImageGPU.hpp"

namespace RHI::vulkan
{

namespace details
{
vk::ImageView CreateImageView(const vk::Device & device, const ImageGPU & image)
{
  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = image.GetHandle();
  viewInfo.viewType = utils::CastInterfaceEnum2Vulkan<VkImageViewType>(image.GetImageType());
  viewInfo.format = utils::CastInterfaceEnum2Vulkan<VkFormat>(image.GetImageFormat());
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  VkImageView view;
  if (auto res = vkCreateImageView(device, &viewInfo, nullptr, &view); res != VK_SUCCESS)
    throw std::invalid_argument("Failed to create image view!");

  return vk::ImageView(view);
}
} // namespace details

ImageGPU_View::ImageGPU_View(const Context & ctx)
  : m_context(ctx)
{
}

ImageGPU_View::~ImageGPU_View()
{
  if (m_view)
    vkDestroyImageView(m_context.GetDevice(), m_view, nullptr);
}

ImageGPU_View::ImageGPU_View(ImageGPU_View && rhs) noexcept
  : m_context(rhs.m_context)
{
  if (this != &rhs)
  {
    std::swap(m_view, rhs.m_view);
  }
}

ImageGPU_View & ImageGPU_View::operator=(ImageGPU_View && rhs) noexcept
{
  if (this != &rhs && &m_context == &rhs.m_context)
  {
    std::swap(m_view, rhs.m_view);
  }
  return *this;
}

void ImageGPU_View::AssignImage(const IImageGPU & image)
{
  auto new_view =
    details::CreateImageView(m_context.GetDevice(), dynamic_cast<const ImageGPU &>(image));
  if (m_view)
    vkDestroyImageView(m_context.GetDevice(), m_view, nullptr);
  m_view = new_view;
}

bool ImageGPU_View::IsImageAssigned() const noexcept
{
  return m_view;
}

InternalObjectHandle ImageGPU_View::GetHandle() const noexcept
{
  return m_view;
}
} // namespace RHI::vulkan
