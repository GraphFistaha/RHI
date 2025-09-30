#include "GenericAttachment.hpp"

#include "../ImageUtils/ImageUtils.hpp"
#include "../ImageUtils/InternalImageTraits.hpp"
#include "../Utils/CastHelper.hpp"
#include "../VulkanContext.hpp"

namespace
{

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

constexpr VkImageUsageFlagBits CalcImageUsageByFormat(RHI::ImageFormat format)
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
      return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    case RHI::ImageFormat::DEPTH:
    case RHI::ImageFormat::DEPTH_STENCIL:
      return VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    default:
      return VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM;
  }
}

constexpr VkImageAspectFlags CalcImageAspectByFormat(RHI::ImageFormat format)
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
      return VK_IMAGE_ASPECT_COLOR_BIT;
    case RHI::ImageFormat::DEPTH:
      return VK_IMAGE_ASPECT_DEPTH_BIT;
    case RHI::ImageFormat::DEPTH_STENCIL:
      return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    default:
      return VK_IMAGE_ASPECT_NONE;
  }
}

VkAttachmentDescription BuildAttachmentDescription(const RHI::ImageCreateArguments & description,
                                                   RHI::SamplesCount samplesCount) noexcept
{
  VkAttachmentDescription attachmentDescription{};
  {
    attachmentDescription.format =
      RHI::vulkan::utils::CastInterfaceEnum2Vulkan<VkFormat>(description.format);
    attachmentDescription.samples =
      RHI::vulkan::utils::CastInterfaceEnum2Vulkan<VkSampleCountFlagBits>(samplesCount);
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
GenericAttachment::GenericAttachment(Context & ctx, const ImageCreateArguments & args,
                                     RHI::RenderBuffering buffering, RHI::SamplesCount samplesCount)
  : OwnedBy<Context>(ctx)
  , m_description(args)
  , m_samplesCount(samplesCount)
  , m_instancesCount(static_cast<uint32_t>(buffering))
{
  m_images.reserve(m_instancesCount);
  m_views.reserve(m_instancesCount);
  m_layouts.reserve(m_instancesCount);
}

GenericAttachment::~GenericAttachment()
{
  for (auto && view : m_views)
    GetContext().GetGarbageCollector().PushVkObjectToDestroy(std::move(view), nullptr);
  for (auto && memBlock : m_images)
    GetContext().GetGarbageCollector().PushVkObjectToDestroy(std::move(memBlock), nullptr);
}


//--------------------- IAttachment interface ----------------


std::future<DownloadResult> GenericAttachment::DownloadImage(HostImageFormat format,
                                                             const TextureRegion & region)
{
  return GetContext().GetTransferer().DownloadImage(*this, format, region);
}

size_t GenericAttachment::Size() const
{
  return std::accumulate(m_images.begin(), m_images.end(), static_cast<size_t>(0),
                         [](size_t acc, auto && img) { return acc + img.Size(); });
}

void GenericAttachment::BlitTo(ITexture * texture)
{
  if (auto * ptr = dynamic_cast<IInternalTexture *>(texture))
    GetContext().GetTransferer().BlitImageToImage(*ptr, *this, RHI::TextureRegion{});
}

ImageCreateArguments GenericAttachment::GetDescription() const noexcept
{
  return m_description;
}

// -------------------- ITexture interface ------------------

VkImageView GenericAttachment::GetImageView() const noexcept
{
  return m_views[m_activeImage];
}

void GenericAttachment::TransferLayout(details::CommandBuffer & commandBuffer, VkImageLayout layout)
{
  m_layouts[m_activeImage].TransferLayout(commandBuffer, layout);
}

VkImageLayout GenericAttachment::GetLayout() const noexcept
{
  return m_layouts[m_activeImage].GetLayout();
}

VkImage GenericAttachment::GetHandle() const noexcept
{
  return m_images[m_activeImage].GetImage();
}

VkFormat GenericAttachment::GetInternalFormat() const noexcept
{
  return utils::CastInterfaceEnum2Vulkan<VkFormat>(m_description.format);
}

VkExtent3D GenericAttachment::GetInternalExtent() const noexcept
{
  return {m_description.extent[0], m_description.extent[1], m_description.extent[2]};
}

//-------------------- IAttachment interface --------------------

void GenericAttachment::Invalidate()
{
  if (m_changedSize || m_changedMSAA)
  {
    for (auto && view : m_views)
      GetContext().GetGarbageCollector().PushVkObjectToDestroy(std::move(view), nullptr);
    for (auto && memBlock : m_images)
      GetContext().GetGarbageCollector().PushVkObjectToDestroy(std::move(memBlock), nullptr);
    m_images.clear();
    m_views.clear();

    m_changedSize = false;
    m_changedMSAA = false;
    m_changedImagesCount = true;
  }

  if (m_changedImagesCount)
  {
    while (m_images.size() > m_instancesCount)
    {
      m_images.pop_back();
      m_layouts.pop_back();
      m_views.pop_back();
    }

    auto desiredMSAA = utils::CastInterfaceEnum2Vulkan<VkSampleCountFlagBits>(m_samplesCount);
    while (m_images.size() < m_instancesCount)
    {
      auto memoryBlock =
        GetContext().GetBuffersAllocator().AllocImage(m_description,
                                                      CalcImageUsageByFormat(m_description.format),
                                                      desiredMSAA);
      m_layouts.emplace_back(memoryBlock.GetImage());
      m_views.emplace_back(utils::CreateImageView(GetContext().GetDevice(), memoryBlock.GetImage(),
                                                  GetInternalFormat(), VK_IMAGE_VIEW_TYPE_2D,
                                                  CalcImageAspectByFormat(m_description.format)));
      m_images.push_back(std::move(memoryBlock));
    }
    m_changedImagesCount = false;
  }
}

std::pair<VkImageView, VkSemaphore> GenericAttachment::AcquireForRendering()
{
  m_renderingMutex.lock();
  m_activeImage = (m_activeImage + 1) % m_images.size();

  // TODO: Set layout for image
  return {GetImageView(), VK_NULL_HANDLE};
}

bool GenericAttachment::FinalRendering(VkSemaphore waitSemaphore)
{
  // TODO: set layout for image
  m_renderingMutex.unlock();
  return true;
}

uint32_t GenericAttachment::GetBuffering() const noexcept
{
  return m_instancesCount;
}

RHI::SamplesCount GenericAttachment::GetSamplesCount() const noexcept
{
  return m_samplesCount;
}

VkAttachmentDescription GenericAttachment::BuildDescription() const noexcept
{
  assert(!m_changedMSAA && !m_changedSize && !m_changedImagesCount);
  return BuildAttachmentDescription(m_description, m_samplesCount);
}

void GenericAttachment::TransferLayout(VkImageLayout layout) noexcept
{
  m_layouts[m_activeImage].TransferLayout(layout);
}

void GenericAttachment::Resize(const VkExtent2D & new_extent) noexcept
{
  m_description.extent = {new_extent.width, new_extent.height, 1};
  m_changedSize = true;
}

} // namespace RHI::vulkan
