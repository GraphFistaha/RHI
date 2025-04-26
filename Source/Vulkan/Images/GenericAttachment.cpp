#include "BufferedTexture.hpp"

#include "../Utils/CastHelper.hpp"
#include "../VulkanContext.hpp"
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
  VkImageUsageFlags vkUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

  if (usage & RHI::TextureUsage::Sampler)
    vkUsage |= VK_IMAGE_USAGE_SAMPLED_BIT;

  if (usage & RHI::TextureUsage::FramebufferAttachment)
    vkUsage |=
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT /* | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
               VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT*/
      ;

  if (usage & RHI::TextureUsage::Compute)
    vkUsage |= VK_IMAGE_USAGE_STORAGE_BIT;

  return vkUsage;
}

constexpr VkImageLayout MakeAttachmentInitialLayout(RHI::ImageFormat format)
{
  return VK_IMAGE_LAYOUT_UNDEFINED;
}

constexpr VkImageLayout MakeAttachmentFinalLayout(RHI::ImageFormat format)
{
  switch (format)
  {
    case RHI::ImageFormat::A8:
    case RHI::ImageFormat::R8:
    case RHI::ImageFormat::RG8:
    case RHI::ImageFormat::RGB8:
    case RHI::ImageFormat::RGBA8:
    case RHI::ImageFormat::BGR8:
    case RHI::ImageFormat::BGRA8:
      return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    case RHI::ImageFormat::DEPTH:
      return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    case RHI::ImageFormat::DEPTH_STENCIL:
      return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    default:
      return VK_IMAGE_LAYOUT_UNDEFINED;
  }
}

VkAttachmentDescription BuildAttachmentDescription(
  const RHI::ImageCreateArguments & description) noexcept
{
  VkAttachmentDescription attachmentDescription{};
  {
    attachmentDescription.format =
      RHI::vulkan::utils::CastInterfaceEnum2Vulkan<VkFormat>(description.format);
    attachmentDescription.samples =
      RHI::vulkan::utils::CastInterfaceEnum2Vulkan<VkSampleCountFlagBits>(description.samples);
    attachmentDescription.initialLayout = MakeAttachmentInitialLayout(description.format);
    attachmentDescription.finalLayout = MakeAttachmentFinalLayout(description.format);
    attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
  }
  return attachmentDescription;
}
} // namespace

namespace RHI::vulkan
{
BufferedTexture::BufferedTexture(Context & ctx, const ImageCreateArguments & args)
  : OwnedBy<Context>(ctx)
  , m_description(args)
{
  // по умолчанию создаем по 1 экземпл€ру картинки
  SetBuffering(1);
  Invalidate();
}

BufferedTexture::~BufferedTexture()
{
  for (auto && view : m_samplerViews)
    GetContext().GetGarbageCollector().PushVkObjectToDestroy(view, nullptr);
  for (auto && view : m_attachmentViews)
    GetContext().GetGarbageCollector().PushVkObjectToDestroy(view, nullptr);
}


//--------------------- ITexture interface ----------------

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

ImageCreateArguments BufferedTexture::GetDescription() const noexcept
{
  return m_description;
}

// -------------------- ITexture interface ------------------

VkImageView BufferedTexture::GetImageView() const noexcept
{
  switch (usage)
  {
    case RHI::TextureUsage::Sampler:
      return m_samplerViews[m_activeImage];
    case RHI::TextureUsage::FramebufferAttachment:
      return m_attachmentViews[m_activeImage];
    default:
      assert(false);
      return VK_NULL_HANDLE;
  }
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

//-------------------- IAttachment interface --------------------

void BufferedTexture::Invalidate()
{
  if (m_changedImagesCount)
  {
    while (m_images.size() > m_desiredInstancesCount)
    {
      m_images.pop_back();
      m_layouts.pop_back();
      if (m_allowedUsage & RHI::TextureUsage::Sampler)
        m_samplerViews.pop_back();
      if (m_allowedUsage & RHI::TextureUsage::FramebufferAttachment)
        m_samplerViews.pop_back();
    }

    while (m_images.size() < m_desiredInstancesCount)
    {
      auto memoryBlock =
        GetContext().GetBuffersAllocator().AllocImage(m_description, CastTextureUsageToVkImageUsage(
                                                                       m_allowedUsage));
      m_layouts.emplace_back(memoryBlock.GetImage());
      if (m_allowedUsage & RHI::TextureUsage::Sampler)
        m_samplerViews.emplace_back(::CreateImageView(GetContext().GetDevice(),
                                                      memoryBlock.GetImage(), GetInternalFormat(),
                                                      VK_IMAGE_VIEW_TYPE_2D));

      if (m_allowedUsage & RHI::TextureUsage::FramebufferAttachment)
        m_attachmentViews.emplace_back(
          ::CreateImageView(GetContext().GetDevice(), memoryBlock.GetImage(), GetInternalFormat(),
                            VK_IMAGE_VIEW_TYPE_2D));
      m_images.push_back(std::move(memoryBlock));
    }
    m_changedImagesCount = false;
  }
}

std::pair<VkImageView, VkSemaphore> BufferedTexture::AcquireForRendering()
{
  m_renderingMutex.lock();
  m_activeImage = (m_activeImage + 1) % m_images.size();

  // TODO: Set layout for image
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
    m_changedImagesCount = true;
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
