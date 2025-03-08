#include "ImageView.hpp"

#include "../Utils/CastHelper.hpp"
#include "../VulkanContext.hpp"
#include "Image.hpp"

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

ImageView::ImageView(Context & ctx, Image & image, VkImageViewType type)
  : m_context(&ctx)
  , m_view(utils::CreateImageView(ctx.GetDevice(), image, type))
{
  ctx.Log(RHI::LogMessageStatus::LOG_DEBUG, "Created VkImageView");
}

ImageView::ImageView(VkImageView view)
  : m_context(nullptr)
  , m_view(view)
{
}

ImageView::~ImageView()
{
  if (m_context)
  {
    m_context->GetGarbageCollector().PushVkObjectToDestroy(m_view, nullptr);
    m_context->Log(RHI::LogMessageStatus::LOG_DEBUG, "VkImageView destroyed");
  }
}

ImageView::ImageView(ImageView && rhs) noexcept
{
  std::swap(m_view, rhs.m_view);
  std::swap(m_context, rhs.m_context);
}

ImageView & ImageView::operator=(ImageView && rhs) noexcept
{
  if (this != &rhs)
  {
    std::swap(m_view, rhs.m_view);
    std::swap(m_context, rhs.m_context);
  }
  return *this;
}

} // namespace RHI::vulkan
