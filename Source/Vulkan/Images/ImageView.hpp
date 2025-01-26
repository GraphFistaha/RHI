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
struct ImageView final : public ContextualObject
{
  ImageView() = default;
  explicit ImageView(const Context & ctx, const ImageBase & image, VkImageView view);
  explicit ImageView(const Context & ctx, const ImageBase & image, VkImageViewType type);

  virtual ~ImageView() override;
  ImageView(ImageView && rhs) noexcept;
  ImageView & operator=(ImageView && rhs) noexcept;

  void AssignImage(const ImageBase & image, VkImageView view);
  void AssignImage(const ImageBase & image, VkImageViewType type);
  bool IsImageAssigned() const noexcept;

  VkImageView GetImageView() const noexcept { return m_view; }

private:
  bool m_owns = false;
  VkImageView m_view = VK_NULL_HANDLE;

private:
  ImageView(const ImageView &) = delete;
  ImageView & operator=(const ImageView &) = delete;
};

} // namespace RHI::vulkan
