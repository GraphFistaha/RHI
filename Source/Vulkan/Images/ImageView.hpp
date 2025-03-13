#pragma once

#include <OwnedBy.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan
{
struct Context;
struct Image;
} // namespace RHI::vulkan

namespace RHI::vulkan
{

/// @brief reference on image
struct ImageView final
{
  ImageView() = default;
  ImageView(Context & ctx, Image & image, VkImageViewType type);
  explicit ImageView(VkImageView view);
  ~ImageView();
  ImageView(ImageView && rhs) noexcept;
  ImageView & operator=(ImageView && rhs) noexcept;

public:
  VkImageView GetHandle() const noexcept { return m_view; }

private:
  Context * m_context = nullptr; // not null if m_view is owned
  VkImageView m_view = VK_NULL_HANDLE;

private:
  ImageView(const ImageView &) = delete;
  ImageView & operator=(const ImageView &) = delete;
};

} // namespace RHI::vulkan
