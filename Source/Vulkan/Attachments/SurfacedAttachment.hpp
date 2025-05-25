#pragma once
#include <deque>
#include <vector>

#include "../CommandsExecution/AsyncTask.hpp"
#include "../Images/ImageLayoutTransferer.hpp"
#include "Attachment.hpp"

namespace vkb
{
struct Swapchain;
}

namespace RHI::vulkan
{

/// @brief vulkan implementation for renderer
struct SurfacedAttachment final : public IAttachment,
                                  public IInternalAttachment,
                                  public OwnedBy<Context>
{
  explicit SurfacedAttachment(Context & ctx, const VkSurfaceKHR surface,
                              RHI::SamplesCount samplesCount);
  virtual ~SurfacedAttachment() override;
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);

public: // ITexture interface
  virtual std::future<DownloadResult> DownloadImage(HostImageFormat format,
                                                    const TextureRegion & region) override;
  virtual ImageCreateArguments GetDescription() const noexcept override;
  virtual size_t Size() const override;

public: //IInternalTexture interface
  virtual VkImageView GetImageView() const noexcept override;
  virtual void TransferLayout(details::CommandBuffer & commandBuffer,
                              VkImageLayout layout) override;
  virtual VkImageLayout GetLayout() const noexcept override;
  virtual VkImage GetHandle() const noexcept override;
  virtual VkFormat GetInternalFormat() const noexcept override;
  virtual VkExtent3D GetInternalExtent() const noexcept override;

public: // IAttachment interface
  virtual void BlitTo(ITexture * texture) override;

public: // IInternalAttachment interface
  virtual std::pair<VkImageView, VkSemaphore> AcquireForRendering() override;
  virtual void Invalidate() override;
  virtual bool FinalRendering(VkSemaphore waitSemaphore) override;
  virtual void SetBuffering(uint32_t framesCount) override;
  virtual uint32_t GetBuffering() const noexcept override;
  virtual VkAttachmentDescription BuildDescription() const noexcept override;
  virtual void TransferLayout(VkImageLayout layout) noexcept override;
  virtual void Resize(const VkExtent2D & new_extent) noexcept;

protected:
  void DestroySwapchain() noexcept;

private:
  std::mutex m_renderingMutex;
  VkQueue m_presentQueue = VK_NULL_HANDLE;
  uint32_t m_presentQueueIndex;
  uint32_t m_desiredBuffering = g_InvalidImageIndex;         ///< desired instances of image
  RHI::SamplesCount m_samplesCount = RHI::SamplesCount::One; ///< samples count for MSAA

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
