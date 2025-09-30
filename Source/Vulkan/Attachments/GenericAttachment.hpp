#pragma once
#pragma once
#include <OwnedBy.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "../ImageUtils/ImageLayoutTransferer.hpp"
#include "Attachment.hpp"

namespace RHI::vulkan
{
struct Context;
}

namespace RHI::vulkan
{

struct GenericAttachment : public IAttachment,
                           public IInternalAttachment,
                           public OwnedBy<Context>
{
  explicit GenericAttachment(Context & ctx, const ImageCreateArguments & m_description,
                             RHI::RenderBuffering buffering, RHI::SamplesCount samplesCount);
  virtual ~GenericAttachment() override;
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);

public: // IAttachment interface
  virtual std::future<DownloadResult> DownloadImage(HostImageFormat format,
                                                    const TextureRegion & region) override;
  virtual ImageCreateArguments GetDescription() const noexcept override;
  /// @brief Get size of image in bytes
  virtual size_t Size() const override;
  virtual void BlitTo(ITexture * texture) override;

public: //IInternalTexture interface
  virtual VkImageView GetImageView() const noexcept override;
  virtual void TransferLayout(details::CommandBuffer & commandBuffer,
                              VkImageLayout layout) override;
  virtual VkImageLayout GetLayout() const noexcept override;
  virtual VkImage GetHandle() const noexcept override;
  virtual VkFormat GetInternalFormat() const noexcept override;
  virtual VkExtent3D GetInternalExtent() const noexcept override;

public: // IInternalAttachment interface
  virtual void Invalidate() override;
  virtual std::pair<VkImageView, VkSemaphore> AcquireForRendering() override;
  virtual bool FinalRendering(VkSemaphore waitSemaphore) override;
  virtual uint32_t GetBuffering() const noexcept override;
  virtual RHI::SamplesCount GetSamplesCount() const noexcept override;
  virtual VkAttachmentDescription BuildDescription() const noexcept override;
  virtual void TransferLayout(VkImageLayout layout) noexcept override;
  virtual void Resize(const VkExtent2D & new_extent) noexcept override;

protected:
  std::mutex m_renderingMutex;        ///< mutex, because you can't enter in rendering mode twice
  ImageCreateArguments m_description; ///< description of image, all main params for image

  std::vector<memory::MemoryBlock> m_images;    ///< memory for image instances
  std::vector<ImageLayoutTransferer> m_layouts; ///< each image must control its layout
  std::vector<VkImageView> m_views;
  uint32_t m_activeImage = 0;

  uint32_t m_instancesCount = 0;
  const RHI::SamplesCount m_samplesCount = RHI::SamplesCount::One;
  bool m_changedImagesCount = true;
  bool m_changedSize = false;
  bool m_changedMSAA = false;
};

} // namespace RHI::vulkan
