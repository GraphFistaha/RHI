#include "ImageView.hpp"

#include "../Utils/CastHelper.hpp"
#include "../VulkanContext.hpp"

namespace RHI::vulkan
{

namespace utils
{
VkImageView CreateImageView(VkDevice device, const ImageBase & image, VkImageViewType type)
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

ImageView::ImageView(const Context & ctx, const ImageBase & image, VkImageView view)
  : ContextualObject(ctx)
  , m_owns(false)
  , m_view(view)
{
}

ImageView::ImageView(const Context & ctx, const ImageBase & image, VkImageViewType type)
  : ContextualObject(ctx)
  , m_owns(true)
  , m_view(utils::CreateImageView(GetContext().GetDevice(), image, type))
{
}

ImageView::~ImageView()
{
  if (m_owns)
    GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_view, nullptr);
}

ImageView::ImageView(ImageView && rhs) noexcept
  : ContextualObject(std::move(rhs))
{
  if (this != &rhs)
  {
    std::swap(m_view, rhs.m_view);
    std::swap(m_owns, rhs.m_owns);
  }
}

ImageView & ImageView::operator=(ImageView && rhs) noexcept
{
  if (this != &rhs)
  {
    ContextualObject::operator=(std::move(rhs));
    std::swap(m_view, rhs.m_view);
    std::swap(m_owns, rhs.m_owns);
  }
  return *this;
}

void ImageView::AssignImage(const ImageBase & image, VkImageView view)
{
  if (m_owns)
    GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_view, nullptr);
  m_view = view;
  m_owns = false;
}

void ImageView::AssignImage(const ImageBase & image, VkImageViewType type)
{
  auto new_view = utils::CreateImageView(GetContext().GetDevice(), image, type);
  if (m_owns)
    GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_view, nullptr);
  m_view = new_view;
  m_owns = true;
}

bool ImageView::IsImageAssigned() const noexcept
{
  return !!m_view;
}
} // namespace RHI::vulkan
