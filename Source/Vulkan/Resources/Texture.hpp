#pragma once
#include <OwnedBy.hpp>
#include <RHI.hpp>

#include "../ImageUtils/ImageLayoutTransferer.hpp"
#include "../ImageUtils/TextureInterface.hpp"
#include "../Memory/MemoryBlock.hpp"

namespace RHI::vulkan
{
struct Context;
} // namespace RHI::vulkan

namespace RHI::vulkan
{

struct Texture : public ITexture,
                 public IInternalTexture,
                 public OwnedBy<Context>
{
  Texture(Context & ctx, const ImageCreateArguments & args);
  virtual ~Texture() override;
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);
  RESTRICTED_COPY(Texture);

public: // ITexture interface
  virtual std::future<UploadResult> UploadImage(const uint8_t * srcPixelData,
                                                const TextureExtent & srcExtent,
                                                HostImageFormat hostFormat,
                                                const TextureRegion & srcRegion,
                                                const TextureRegion & dstRegion) override;
  virtual std::future<DownloadResult> DownloadImage(HostImageFormat format,
                                                    const TextureRegion & region) override;
  virtual ImageCreateArguments GetDescription() const noexcept override;
  virtual size_t Size() const override;
  //virtual void SetSwizzle() = 0;
  virtual void BlitTo(ITexture * texture) override;

public: // IInternalTexture interface
  virtual VkImageView GetImageView() const noexcept override;
  virtual void TransferLayout(details::CommandBuffer & commandBuffer,
                              VkImageLayout layout) override;
  virtual VkImageLayout GetLayout() const noexcept override;
  virtual VkImage GetHandle() const noexcept override;
  virtual VkFormat GetInternalFormat() const noexcept override;
  virtual VkExtent3D GetInternalExtent() const noexcept override;

private:
  ImageCreateArguments m_description;
  memory::MemoryBlock m_memBlock;
  ImageLayoutTransferer m_layout;
  VkImageView m_view = VK_NULL_HANDLE;
};
} // namespace RHI::vulkan
