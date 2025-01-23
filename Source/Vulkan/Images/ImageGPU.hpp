#pragma once
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "../Memory/MemoryBlock.hpp"
#include "ImageBase.hpp"

namespace RHI::vulkan
{
/// @brief image with owned memory. It used for user's textures, framebuffer attachments
struct ImageGPU : public ImageBase
{
  explicit ImageGPU(const Context & ctx, Transferer * transferer,
                    const ImageDescription & description);
  virtual ~ImageGPU() override;
  ImageGPU(ImageGPU && rhs) noexcept;
  ImageGPU & operator=(ImageGPU && rhs) noexcept;

private:
  memory::MemoryBlock m_memBlock;
};

} // namespace RHI::vulkan
