#include "ImageView.hpp"

#include "../Utils/CastHelper.hpp"
#include "../VulkanContext.hpp"

namespace RHI::vulkan
{

namespace utils
{
VkImageView CreateImageView(VkDevice device, const Image & image, VkImageViewType type)
{
  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = image.GetHandle();
  viewInfo.viewType = type;
  viewInfo.format = image.GetVulkanFormat();
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  VkImageView view;
  if (auto res = vkCreateImageView(device, &viewInfo, nullptr, &view); res != VK_SUCCESS)
    throw std::invalid_argument("Failed to create image view!");

  return view;
}
} // namespace utils

ImageView::ImageView(Context & ctx)
  : OwnedBy<Context>(ctx)
{
}

ImageView::~ImageView()
{
  if (m_owns)
    GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_view, nullptr);
}

ImageView::ImageView(ImageView && rhs) noexcept
  : OwnedBy<Context>(std::move(rhs))
{
  std::swap(m_imagePtr, rhs.m_imagePtr);
  std::swap(m_view, rhs.m_view);
  std::swap(m_owns, rhs.m_owns);
}

ImageView & ImageView::operator=(ImageView && rhs) noexcept
{
  if (this != &rhs)
  {
    OwnedBy<Context>::operator=(std::move(rhs));
    std::swap(m_imagePtr, rhs.m_imagePtr);
    std::swap(m_view, rhs.m_view);
    std::swap(m_owns, rhs.m_owns);
  }
  return *this;
}

void ImageView::AssignImage(Image * image, VkImageView view)
{
  if (image)
  {
    if (m_owns)
      GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_view, nullptr);
    m_imagePtr = image;
    m_view = view;
    m_owns = false;
  }
  else
  {
    if (m_owns)
      GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_view, nullptr);
    m_imagePtr = nullptr;
    m_view = VK_NULL_HANDLE;
    m_owns = false;
  }
}

void ImageView::AssignImage(Image * image, VkImageViewType type)
{
  if (image)
  {
    auto new_view = utils::CreateImageView(image->GetContext().GetDevice(), *image, type);
    if (m_owns)
      GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_view, nullptr);
    m_imagePtr = image;
    m_view = new_view;
    m_owns = true;
  }
  else
  {
    if (m_owns)
      GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_view, nullptr);
    m_imagePtr = nullptr;
    m_view = VK_NULL_HANDLE;
    m_owns = false;
  }
}

bool ImageView::IsImageAssigned() const noexcept
{
  return !!m_view;
}
} // namespace RHI::vulkan
