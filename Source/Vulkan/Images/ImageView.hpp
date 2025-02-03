#pragma once

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "../ContextualObject.hpp"
#include "ImageBase.hpp"

namespace RHI::vulkan
{
struct Context;
}

namespace RHI::vulkan
{
/// @brief reference on image
struct ImageView final
{
  ImageView() = default;
  explicit ImageView(const ImageBase & image, VkImageView view);
  explicit ImageView(const ImageBase & image, VkImageViewType type);

  ~ImageView();
  ImageView(ImageView && rhs) noexcept;
  ImageView & operator=(ImageView && rhs) noexcept;

  void AssignImage(const ImageBase & image, VkImageView view);
  void AssignImage(const ImageBase & image, VkImageViewType type);
  bool IsImageAssigned() const noexcept;

  VkImageView GetImageView() const noexcept { return m_view; }

private:
  const ImageBase * m_imagePtr = nullptr;
  bool m_owns = false;
  VkImageView m_view = VK_NULL_HANDLE;

private:
  ImageView(const ImageView &) = delete;
  ImageView & operator=(const ImageView &) = delete;
};

} // namespace RHI::vulkan
