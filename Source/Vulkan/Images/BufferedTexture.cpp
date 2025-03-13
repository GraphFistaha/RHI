#include "BufferedTexture.hpp"

#include "../VulkanContext.hpp"
#include "ImageInfo.hpp"

namespace RHI::vulkan
{
BufferedTexture::BufferedTexture(Context & ctx, const ImageCreateArguments & args)
  : OwnedBy<Context>(ctx)
  , m_description(args)
{
}


std::future<UploadResult> BufferedTexture::UploadImage(const uint8_t * srcPixelData,
                                                       const CopyImageArguments & args)
{
  assert(GetImage());
  return GetImage()->UploadImage(srcPixelData, args);
}


std::future<DownloadResult> BufferedTexture::DownloadImage(HostImageFormat format,
                                                           const ImageRegion & region)
{
  assert(GetImage());
  return GetImage()->DownloadImage(format, region);
}

size_t BufferedTexture::Size() const
{
  assert(GetImage());
  return GetImage()->Size();
}

std::pair<VkImageView, VkSemaphore> BufferedTexture::AcquireForRendering()
{
  m_activeImage = (m_activeImage + 1) % m_images.size();
  return {GetImage(ImageUsage::FramebufferAttachment), VK_NULL_HANDLE};
}

VkImageView BufferedTexture::GetImage(ImageUsage usage) const noexcept
{
  return m_views.at(usage)[m_activeImage];
}

const Image * BufferedTexture::GetImage() const noexcept
{
  return &m_images[m_activeImage];
}

Image * BufferedTexture::GetImage() noexcept
{
  return &m_images[m_activeImage];
}

bool BufferedTexture::FinalRendering(VkSemaphore waitSemaphore)
{
  return true;
}

void BufferedTexture::SetBuffering(uint32_t framesCount)
{
  while (m_images.size() > framesCount)
    m_images.pop_back();

  while (m_images.size() < framesCount)
    m_images.emplace_back(GetContext(), *this);
}

uint32_t BufferedTexture::GetBuffering() const noexcept
{
  return m_images.size();
}

VkAttachmentDescription BufferedTexture::BuildDescription() const noexcept
{
  return BuildAttachmentDescription(m_description);
}

ImageCreateArguments BufferedTexture::GetDescription() const noexcept
{
  return m_description;
}

void BufferedTexture::TransferLayout(details::CommandBuffer & commandBuffer, VkImageLayout layout)
{
    // TODO
}
} // namespace RHI::vulkan
