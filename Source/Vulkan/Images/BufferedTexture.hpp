#pragma once
#include <OwnedBy.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "../Graphics/TextureInterface.hpp"
#include "Image.hpp"

namespace RHI::vulkan
{
struct Context;
}

namespace RHI::vulkan
{

struct BufferedTexture : public IAttachment,
                         public OwnedBy<Context>
{
  explicit BufferedTexture(Context & ctx, const ImageCreateArguments & m_description);
  virtual ~BufferedTexture() override = default;
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);

public: // IImageGPU interface
  virtual std::future<UploadResult> UploadImage(const uint8_t * srcPixelData,
                                                const CopyImageArguments & args) override;
  virtual std::future<DownloadResult> DownloadImage(HostImageFormat format,
                                                    const ImageRegion & region) override;
  virtual ImageCreateArguments GetDescription() const noexcept override;
  /// @brief Get size of image in bytes
  virtual size_t Size() const override;

public: //ITexture interface
  virtual VkImageView GetImageView(ImageUsage usage) const noexcept override;
  virtual void TransferLayout(details::CommandBuffer & commandBuffer,
                              VkImageLayout layout) override;
  virtual VkImageLayout GetLayout() const noexcept override;
  virtual VkImage GetHandle() const noexcept override;
  virtual VkFormat GetInternalFormat() const noexcept override;
  virtual VkExtent3D GetInternalExtent() const noexcept override;

public: // IAttachment interface
  virtual void Invalidate() override {}
  virtual std::pair<VkImageView, VkSemaphore> AcquireForRendering() override;
  virtual bool FinalRendering(VkSemaphore waitSemaphore) override;
  virtual void SetBuffering(uint32_t framesCount) override;
  virtual uint32_t GetBuffering() const noexcept override;
  virtual VkAttachmentDescription BuildDescription() const noexcept override;
  virtual void TransferLayout(VkImageLayout layout) noexcept override;

private:
  Image * GetImage() noexcept;
  const Image * GetImage() const noexcept;

protected:
  using ImageViews = std::vector<VkImageView>;

  ImageCreateArguments m_description;
  std::vector<Image> m_images;
  std::unordered_map<ImageUsage, ImageViews> m_views;
  uint32_t m_activeImage = g_InvalidImageIndex;
};

} // namespace RHI::vulkan
