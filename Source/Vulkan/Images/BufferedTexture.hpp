#pragma once
#pragma once
#include <OwnedBy.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "../Graphics/TextureInterface.hpp"
#include "ImageLayoutTransferer.hpp"

namespace RHI::vulkan
{
struct Context;
}

namespace RHI::vulkan
{

struct BufferedTexture : public IAttachment,
                         public OwnedBy<Context>
{
  explicit BufferedTexture(Context & ctx, const ImageCreateArguments & m_description,
                           TextureUsage usage);
  virtual ~BufferedTexture() override;
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);

public: // IImageGPU interface
  virtual std::future<UploadResult> UploadImage(const uint8_t * srcPixelData,
                                                const CopyImageArguments & args) override;
  virtual std::future<DownloadResult> DownloadImage(HostImageFormat format,
                                                    const ImageRegion & region) override;
  virtual ImageCreateArguments GetDescription() const noexcept override;
  /// @brief Get size of image in bytes
  virtual size_t Size() const override;
  virtual bool IsAllowedUsage(TextureUsage usage) const noexcept override;

public: //ITexture interface
  virtual VkImageView GetImageView(TextureUsage usage) const noexcept override;
  virtual void TransferLayout(details::CommandBuffer & commandBuffer,
                              VkImageLayout layout) override;
  virtual VkImageLayout GetLayout() const noexcept override;
  virtual VkImage GetHandle() const noexcept override;
  virtual VkFormat GetInternalFormat() const noexcept override;
  virtual VkExtent3D GetInternalExtent() const noexcept override;

public: // IAttachment interface
  virtual void Invalidate() override;
  virtual std::pair<VkImageView, VkSemaphore> AcquireForRendering() override;
  virtual bool FinalRendering(VkSemaphore waitSemaphore) override;
  virtual void SetBuffering(uint32_t framesCount) override;
  virtual uint32_t GetBuffering() const noexcept override;
  virtual VkAttachmentDescription BuildDescription() const noexcept override;
  virtual void TransferLayout(VkImageLayout layout) noexcept override;

protected:
  static constexpr RHI::TextureUsage s_allowedUsages[] = {RHI::TextureUsage::Sampler,
                                                          RHI::TextureUsage::FramebufferAttachment};

  std::mutex m_renderingMutex;        ///< mutex, because you can't enter in rendering mode twice
  ImageCreateArguments m_description; ///< description of image, all main params for image

  std::vector<memory::MemoryBlock> m_images;    ///< memory for image instances
  std::vector<ImageLayoutTransferer> m_layouts; ///< each image must control its layout
  std::vector<VkImageView> m_samplerViews;
  std::vector<VkImageView> m_attachmentViews;
  uint32_t m_activeImage = 0;

  uint32_t m_desiredInstancesCount = 0;
  bool m_changedImagesCount = false;
  const TextureUsage m_allowedUsage;
};

} // namespace RHI::vulkan
