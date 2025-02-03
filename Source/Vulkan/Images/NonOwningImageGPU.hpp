#pragma once
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "ImageBase.hpp"

namespace RHI::vulkan
{

/// @brief image which can be built from already existing image (f.e from swapchainKHR)
struct NonOwningImageGPU : public ImageBase
{
  explicit NonOwningImageGPU(Context & ctx, const ImageDescription & description, VkImage image,
                             VkImageLayout layout);
  virtual ~NonOwningImageGPU() override = default;
};
} // namespace RHI::vulkan
