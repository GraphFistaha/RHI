#pragma once

#include <deque>
#include <vector>

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "RenderTarget.hpp"

namespace RHI::vulkan
{
struct Context;
}

namespace RHI::vulkan
{

/// @brief vulkan implementation for renderer
struct SwapchainBase : public ISwapchain
{
  explicit SwapchainBase(const Context & ctx);
  virtual ~SwapchainBase() override;

public: // ISwapchain interface
  virtual IRenderTarget * AcquireFrame() override;
  virtual void FlushFrame() override;
  virtual ISubpass * CreateSubpass() override;
  virtual void AddImageAttachment(uint32_t binding, const ImageCreateArguments & args) override;

protected: // SwapchainBase interface
  virtual std::pair<uint32_t, VkSemaphore> AcquireImage() noexcept = 0;
  virtual bool PresentImage(uint32_t activeImage, VkSemaphore waitRenderingSemaphore) noexcept = 0;

public:
  virtual void SetInvalid() override;

public: // RHI-only API
  size_t GetImagesCount() const noexcept;

protected:
  void InitRenderTargets(VkExtent2D extent, size_t frames_count);
  void ForEachRenderTarget(std::function<void(RenderTarget &)> && func);

protected:
  static constexpr uint32_t InvalidImageIndex = -1;
  using ImagesSet = std::vector<VkImage>;
  using ImagesSet = std::vector<VkImage>;

  const Context & m_context;
  std::deque<RenderTarget> m_targets;
  RenderPass m_renderPass;
  bool m_invalidSwapchain = false;

  std::vector<RHI::ImageCreateArguments> m_protoAttachments;

private:
  VkSemaphore m_cachedPresentSemaphore = VK_NULL_HANDLE;
  uint32_t m_cachedActiveImage = InvalidImageIndex;
};

} // namespace RHI::vulkan
