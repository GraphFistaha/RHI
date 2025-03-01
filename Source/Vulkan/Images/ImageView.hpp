#pragma once

#include <OwnedBy.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "Image.hpp"

namespace RHI::vulkan
{
struct Context;
}

namespace RHI::vulkan
{
/// @brief reference on image
struct ImageView final : public OwnedBy<Context>
{
  explicit ImageView(Context & ctx);
  ~ImageView();
  ImageView(ImageView && rhs) noexcept;
  ImageView & operator=(ImageView && rhs) noexcept;

  void AssignImage(Image * image, VkImageView view);
  void AssignImage(Image * image, VkImageViewType type);
  bool IsImageAssigned() const noexcept;

  VkImageView GetHandle() const noexcept { return m_view; }
  Image * GetImagePtr() const noexcept { return m_imagePtr; }
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);

private:
  Image * m_imagePtr = nullptr;
  bool m_owns = false;
  VkImageView m_view = VK_NULL_HANDLE;

private:
  ImageView(const ImageView &) = delete;
  ImageView & operator=(const ImageView &) = delete;
};

} // namespace RHI::vulkan
