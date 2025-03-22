#include "BufferedTexture.hpp"

#include "../Utils/CastHelper.hpp"
#include "../VulkanContext.hpp"
#include "ImageInfo.hpp"
#include "InternalImageTraits.hpp"

namespace
{
VkImageView CreateImageView(VkDevice device, VkImage image, VkFormat format, VkImageViewType type)
{
  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = image;
  viewInfo.viewType = type;
  viewInfo.format = format;
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  VkImageView view;
  if (auto res = vkCreateImageView(device, &viewInfo, nullptr, &view); res != VK_SUCCESS)
    throw std::invalid_argument("Failed to create image view!");

  return view;
}

constexpr VkImageUsageFlags CastTextureUsageToVkImageUsage(RHI::TextureUsage usage) noexcept
{
  VkImageUsageFlags vkUsage = 0;

  if (usage & RHI::TextureUsage::Sampler)
    vkUsage |= VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

  if (usage & RHI::TextureUsage::FramebufferAttachment)
    vkUsage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
               VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;

  if (usage & RHI::TextureUsage::Readback)
    vkUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

  if (usage & RHI::TextureUsage::Compute)
    vkUsage |= VK_IMAGE_USAGE_STORAGE_BIT;

  return vkUsage;
}
} // namespace

namespace RHI::vulkan
{
BufferedTexture::BufferedTexture(Context & ctx, const ImageCreateArguments & args,
                                 TextureUsage usage)
  : OwnedBy<Context>(ctx)
  , m_description(args)
  , m_allowedUsage(usage)
{
  // по умолчанию создаем по 1 экземпл€ру картинки
  SetBuffering(1);
  Invalidate();
}


//--------------------- IImageGPU interface ----------------

std::future<UploadResult> BufferedTexture::UploadImage(const uint8_t * srcPixelData,
                                                       const CopyImageArguments & args)
{
  //TODO: upload to all instances
  return GetContext().GetTransferer().UploadImage(*this, srcPixelData, args);
}


std::future<DownloadResult> BufferedTexture::DownloadImage(HostImageFormat format,
                                                           const ImageRegion & region)
{
  return GetContext().GetTransferer().DownloadImage(*this, format, region);
}

size_t BufferedTexture::Size() const
{
  return m_images.empty() ? 0 : m_images.begin()->Size();
}

bool BufferedTexture::IsAllowedUsage(TextureUsage usage) const noexcept
{
  return m_allowedUsage & usage;
}

ImageCreateArguments BufferedTexture::GetDescription() const noexcept
{
  return m_description;
}

// -------------------- ITexture interface ------------------

VkImageView BufferedTexture::GetImageView(RHI::TextureUsage usage) const noexcept
{
  return m_views.at(usage)[m_activeImage];
}

void BufferedTexture::TransferLayout(details::CommandBuffer & commandBuffer, VkImageLayout layout)
{
  m_layouts[m_activeImage].TransferLayout(commandBuffer, layout);
}

VkImageLayout BufferedTexture::GetLayout() const noexcept
{
  return m_layouts[m_activeImage].GetLayout();
}

VkImage BufferedTexture::GetHandle() const noexcept
{
  return m_images[m_activeImage].GetImage();
}

VkFormat BufferedTexture::GetInternalFormat() const noexcept
{
  return utils::CastInterfaceEnum2Vulkan<VkFormat>(m_description.format);
}

VkExtent3D BufferedTexture::GetInternalExtent() const noexcept
{
  return {m_description.extent[0], m_description.extent[1], m_description.extent[2]};
}

void BufferedTexture::AllowUsage(RHI::TextureUsage usage) noexcept
{
  if (!IsAllowedUsage(usage))
  {
    GetContext().Log(RHI::LogMessageStatus::LOG_WARNING,
                     "Allowed usage has been changed for BufferedTexture"
                     " It can reduce performance because texture will be rebuilt");
    m_allowedUsage = static_cast<RHI::TextureUsage>(m_allowedUsage | usage);
    m_fullInvalidationRequired = true;
  }
}

//-------------------- IAttachment interface --------------------

void BufferedTexture::Invalidate()
{
    //¬месо того, чтобы пресоздавать изобрадение, давай лучше замен€ть VkImage в MemoryBlock
  if (m_justChangeImagesCountRequired)
  {
    while (m_images.size() > m_desiredInstancesCount)
    {
      m_images.pop_back();
      m_layouts.pop_back();
      for (auto && [usage, views] : m_views)
        views.pop_back();
    }

    while (m_images.size() < m_desiredInstancesCount)
    {
      auto memoryBlock =
        GetContext().GetBuffersAllocator().AllocImage(m_description, CastTextureUsageToVkImageUsage(
                                                                       m_allowedUsage));
      m_layouts.emplace_back(memoryBlock.GetImage());
      m_images.push_back(std::move(memoryBlock));
    }
    m_justChangeImagesCountRequired = false;
  }
}

std::pair<VkImageView, VkSemaphore> BufferedTexture::AcquireForRendering()
{
  m_renderingMutex.lock();
  // TODO: Set layout for image
  m_activeImage = (m_activeImage + 1) % m_images.size();
  return {GetImageView(RHI::TextureUsage::FramebufferAttachment), VK_NULL_HANDLE};
}

bool BufferedTexture::FinalRendering(VkSemaphore waitSemaphore)
{
  // TODO: set layout for image
  m_renderingMutex.unlock();
  return true;
}

void BufferedTexture::SetBuffering(uint32_t framesCount)
{
  if (GetBuffering() != framesCount)
  {
    m_desiredInstancesCount = framesCount;
    m_justChangeImagesCountRequired = true;
  }
}

uint32_t BufferedTexture::GetBuffering() const noexcept
{
  return static_cast<uint32_t>(m_images.size());
}

VkAttachmentDescription BufferedTexture::BuildDescription() const noexcept
{
  return BuildAttachmentDescription(m_description);
}

void BufferedTexture::TransferLayout(VkImageLayout layout) noexcept
{
  m_layouts[m_activeImage].TransferLayout(layout);
}

} // namespace RHI::vulkan
