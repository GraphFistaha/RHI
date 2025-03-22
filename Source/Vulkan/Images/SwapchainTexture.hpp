#pragma once
#include <deque>
#include <vector>

#include "../CommandsExecution/AsyncTask.hpp"
#include "../Graphics/TextureInterface.hpp"
#include "ImageLayoutTransferer.hpp"

namespace vkb
{
struct Swapchain;
}

namespace RHI::vulkan
{

/// @brief vulkan implementation for renderer
struct SwapchainTexture final : public IAttachment,
                                public OwnedBy<Context>
{
  explicit SwapchainTexture(Context & ctx, const VkSurfaceKHR surface,
                            RHI::SamplesCount samplesCount);
  virtual ~SwapchainTexture() override;
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);

public: // IImageGPU interface
  virtual std::future<UploadResult> UploadImage(const uint8_t * srcPixelData,
                                                const CopyImageArguments & args) override;
  virtual std::future<DownloadResult> DownloadImage(HostImageFormat format,
                                                    const ImageRegion & region) override;
  virtual ImageCreateArguments GetDescription() const noexcept override;
  /// @brief Get size of image in bytes
  virtual size_t Size() const override;
  virtual bool IsAllowedUsage(RHI::TextureUsage usage) const noexcept override;

public: //ITexture interface
  virtual VkImageView GetImageView(RHI::TextureUsage usage) const noexcept override;
  virtual void TransferLayout(details::CommandBuffer & commandBuffer,
                              VkImageLayout layout) override;
  virtual VkImageLayout GetLayout() const noexcept override;
  virtual VkImage GetHandle() const noexcept override;
  virtual VkFormat GetInternalFormat() const noexcept override;
  virtual VkExtent3D GetInternalExtent() const noexcept override;
  virtual void AllowUsage(RHI::TextureUsage usage) noexcept override;

public: // IAttachment interface
  virtual std::pair<VkImageView, VkSemaphore> AcquireForRendering() override;
  virtual void Invalidate() override;
  virtual bool FinalRendering(VkSemaphore waitSemaphore) override;
  virtual void SetBuffering(uint32_t framesCount) override;
  virtual uint32_t GetBuffering() const noexcept override;
  virtual VkAttachmentDescription BuildDescription() const noexcept override;
  virtual void TransferLayout(VkImageLayout layout) noexcept override;

protected:
  void DestroySwapchain() noexcept;

private:
  std::mutex m_renderingMutex;
  VkQueue m_presentQueue = VK_NULL_HANDLE;
  uint32_t m_presentQueueIndex;
  uint32_t m_desiredBuffering = g_InvalidImageIndex;         ///< desired instances of image
  RHI::SamplesCount m_samplesCount = RHI::SamplesCount::One; ///< samples count for MSAA
  RHI::TextureUsage m_allowedUsage;                          ///< allowed scenario of usage

  std::vector<VkImage> m_images;                          ///< swapchain images
  std::vector<VkImageView> m_imageViews;                  /// attachment image view
  std::vector<VkSemaphore> m_imageAvailabilitySemaphores; /// owns
  std::vector<ImageLayoutTransferer> m_layouts;
  uint32_t m_activeSemaphore = 0;               ///<
  uint32_t m_activeImage = g_InvalidImageIndex; ///< image index in rendering

  /// presentation data
  VkSurfaceKHR m_surface;                      ///< surface
  std::unique_ptr<vkb::Swapchain> m_swapchain; ///< swapchain
  bool m_invalidSwapchain = false;
};

} // namespace RHI::vulkan
