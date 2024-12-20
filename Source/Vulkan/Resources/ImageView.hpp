#pragma once

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan
{
struct Context;
}

namespace RHI::vulkan
{
struct ImageGPU_View final : public IImageGPU_View
{
  explicit ImageGPU_View(const Context & ctx);
  virtual ~ImageGPU_View() override;
  ImageGPU_View(ImageGPU_View && rhs) noexcept;
  ImageGPU_View & operator=(ImageGPU_View && rhs) noexcept;

  virtual void AssignImage(const IImageGPU & image) override;
  virtual bool IsImageAssigned() const noexcept override;

  virtual InternalObjectHandle GetHandle() const noexcept override;

private:
  const Context & m_context;
  vk::ImageView m_view = VK_NULL_HANDLE;

private:
  ImageGPU_View(const ImageGPU_View &) = delete;
  ImageGPU_View & operator=(const ImageGPU_View &) = delete;
};
} // namespace RHI::vulkan
