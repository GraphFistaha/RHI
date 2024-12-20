#pragma once

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "ImageView.hpp"

namespace RHI::vulkan
{
struct Context;
}

namespace RHI::vulkan
{

struct ImageGPU_Sampler final : public IImageGPU_Sampler
{
  explicit ImageGPU_Sampler(const Context & ctx);
  ~ImageGPU_Sampler() override;

  virtual void Invalidate() override;
  virtual InternalObjectHandle GetHandle() const noexcept override;

  virtual IImageGPU_View & GetImageView() & noexcept override { return m_view; }
  virtual const IImageGPU_View & GetImageView() const & noexcept { return m_view; };

private:
  const Context & m_context;
  ImageGPU_View m_view{m_context};
  vk::Sampler m_sampler = VK_NULL_HANDLE;
  bool m_invalidSampler = true;
};

} // namespace RHI::vulkan
