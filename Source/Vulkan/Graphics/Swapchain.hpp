#pragma once

#include <deque>
#include <vector>

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "../Images/ImageGPU.hpp"
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

public: // IInvalidable Interface
  virtual void Invalidate() override;
  virtual void SetInvalid() override;

protected
  : // SwapchainBase interface
    //virtual std::pair<uint32_t, VkSemaphore> AcquireImage() noexcept = 0;
    //virtual bool PresentImage(uint32_t activeImage, VkSemaphore waitRenderingSemaphore) noexcept = 0;
public: // RHI-only API
  size_t GetImagesCount() const noexcept;
  void SetExtent(VkExtent2D extent) noexcept;
  void SetFramesCount(uint32_t framesCount) noexcept;

protected:
  void ForEachRenderTarget(std::function<void(RenderTarget &)> && func);

protected:
  static constexpr uint32_t InvalidImageIndex = -1;
  using AttachmentSet = std::vector<ImageGPU>;

  const Context & m_context;
  size_t m_framesCount = 0;
  VkExtent2D m_extent;

  std::deque<RenderTarget> m_targets;
  RenderPass m_renderPass;
  bool m_attachmentsChanged = false;

  std::unordered_map<uint32_t, std::pair<RHI::ImageCreateArguments, bool>> m_protoAttachments;
  std::unordered_map<uint32_t, AttachmentSet> m_attachments;
  uint32_t m_activeImageIdx = InvalidImageIndex;

private:
  VkSemaphore m_cachedPresentSemaphore = VK_NULL_HANDLE;
  uint32_t m_cachedActiveImage = InvalidImageIndex;
};

} // namespace RHI::vulkan
