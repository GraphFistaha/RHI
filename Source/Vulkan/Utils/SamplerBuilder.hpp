#pragma once
#include <vector>

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan::utils
{
struct SamplerBuilder final
{
  VkSampler Make(const VkDevice & device) const;
  void Reset();

  void SetFilter(RHI::TextureFilteration minFilter, RHI::TextureFilteration magFilter) noexcept;
  void SetTextureWrapping(RHI::TextureWrapping uWrapping, RHI::TextureWrapping vWrapping, RHI::TextureWrapping wWrapping);

private:
  VkSamplerCreateInfo m_createInfo;
};
} // namespace RHI::vulkan::utils
