#include "SurfacedAttachment.hpp"

#include <format>

#include <VkBootstrap.h>

#include "../Graphics/RenderPass.hpp"
#include "../Images/InternalImageTraits.hpp"
#include "../Utils/CastHelper.hpp"
#include "../VulkanContext.hpp"

namespace RHI::vulkan
{

SurfacedAttachment::SurfacedAttachment(Context & ctx, const VkSurfaceKHR surface,
                                       RHI::SamplesCount samplesCount)
  : OwnedBy<Context>(ctx)
  , m_surface(surface)
  , m_swapchain(std::make_unique<vkb::Swapchain>())
  , m_samplesCount(samplesCount)
{
  std::tie(m_presentQueueIndex, m_presentQueue) = ctx.GetQueue(QueueType::Present);
}

SurfacedAttachment::~SurfacedAttachment()
{
  DestroySwapchain();
}

std::future<DownloadResult> SurfacedAttachment::DownloadImage(HostImageFormat format,
                                                              const ImageRegion & region)
{
  return GetContext().GetTransferer().DownloadImage(*this, format, region);
}

ImageCreateArguments SurfacedAttachment::GetDescription() const noexcept
{
  ImageCreateArguments description{};
  {
    description.samples = m_samplesCount;
    description.mipLevels = 1;
    description.shared = false;
    description.type = RHI::ImageType::Image2D;
    if (m_swapchain)
    {
      description.extent = {m_swapchain->extent.width, m_swapchain->extent.height, 1};
      description.format = RHI::ImageFormat::RGBA8; // TODO: take from m_swapchain
    }
    else
    {
      description.format = RHI::ImageFormat::UNDEFINED;
    }
  }
  return description;
}

size_t SurfacedAttachment::Size() const
{
  return RHI::utils::GetSizeOfImage(GetInternalExtent(), GetInternalFormat());
}

// -------------------- ITexture interface ---------------------

VkImageView SurfacedAttachment::GetImageView() const noexcept
{
  return m_imageViews[m_activeImage];
}

void SurfacedAttachment::TransferLayout(details::CommandBuffer & commandBuffer,
                                        VkImageLayout layout)
{
  m_layouts[m_activeImage].TransferLayout(commandBuffer, layout);
}

VkImageLayout SurfacedAttachment::GetLayout() const noexcept
{
  return m_layouts[m_activeImage].GetLayout();
}

VkImage SurfacedAttachment::GetHandle() const noexcept
{
  return m_images[m_activeImage];
}

VkFormat SurfacedAttachment::GetInternalFormat() const noexcept
{
  return m_swapchain ? m_swapchain->image_format : VK_FORMAT_UNDEFINED;
}

VkExtent3D SurfacedAttachment::GetInternalExtent() const noexcept
{
  VkExtent3D result{0, 0, 0};
  if (m_swapchain)
  {
    VkExtent2D tmp = m_swapchain->extent;
    result = {tmp.width, tmp.height, 1};
  }
  return result;
}

void SurfacedAttachment::BlitTo(ITexture * texture)
{
  //if (auto * ptr = dynamic_cast<RHI::vulkan::IInternalTexture *>(texture))
  //  GetContext().GetTransferer().BlitImageToImage(*ptr, *this);
}

// --------------------- IAttachment interface ----------------------

void SurfacedAttachment::Invalidate()
{
  if (m_invalidSwapchain || !m_swapchain->swapchain)
  {
    auto [renderIndex, renderQueue] = GetContext().GetQueue(QueueType::Graphics);
    vkb::SwapchainBuilder swapchain_builder(GetContext().GetGPU(), GetContext().GetDevice(),
                                            m_surface, renderIndex, m_presentQueueIndex);
    if (m_desiredBuffering != g_InvalidImageIndex)
      swapchain_builder.set_desired_min_image_count(m_desiredBuffering);
    auto swap_ret = swapchain_builder.set_old_swapchain(*m_swapchain).build();
    if (!swap_ret)
      throw std::runtime_error("Failed to create Vulkan swapchain - " + swap_ret.error().message());

    DestroySwapchain();

    *m_swapchain = swap_ret.value();
    m_images = m_swapchain->get_images().value();
    m_imageViews = m_swapchain->get_image_views().value();
    for (auto && view : m_imageViews)
      m_imageAvailabilitySemaphores.push_back(utils::CreateVkSemaphore(GetContext().GetDevice()));

    m_layouts.reserve(m_images.size());
    for (auto image : m_images)
      m_layouts.emplace_back(image);

    // reset invalid flags
    m_invalidSwapchain = false;
    m_desiredBuffering = g_InvalidImageIndex;
  }
}

std::pair<VkImageView, VkSemaphore> SurfacedAttachment::AcquireForRendering()
{
  m_renderingMutex.lock();
  VkSemaphore signalSemaphore = m_imageAvailabilitySemaphores[m_activeSemaphore];
  uint32_t imageIndex = g_InvalidImageIndex;
  auto res = vkAcquireNextImageKHR(GetContext().GetDevice(), m_swapchain->swapchain, UINT64_MAX,
                                   signalSemaphore, VK_NULL_HANDLE, &imageIndex);
  if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
  {
    m_renderingMutex.unlock();
    m_invalidSwapchain = true;
    return {VK_NULL_HANDLE, VK_NULL_HANDLE};
  }
  else if (res != VK_SUCCESS)
  {
    m_renderingMutex.unlock();
    throw std::runtime_error(
      std::format("Failed to acquire swap chain image - {}", static_cast<uint32_t>(res)));
  }
  m_activeImage = imageIndex;
  return {GetImageView(), signalSemaphore};
}

bool SurfacedAttachment::FinalRendering(VkSemaphore waitSemaphore)
{
  const VkSwapchainKHR swapchains[] = {m_swapchain->swapchain};
  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = !!waitSemaphore ? 1 : 0;
  presentInfo.pWaitSemaphores = &waitSemaphore;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapchains;
  presentInfo.pImageIndices = &m_activeImage;
  presentInfo.pResults = nullptr; // Optional
  auto res = vkQueuePresentKHR(m_presentQueue, &presentInfo);
  m_activeSemaphore = (m_activeSemaphore + 1u) % m_imageAvailabilitySemaphores.size();
  if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
  {
    m_renderingMutex.unlock();
    Invalidate();
    return false;
  }
  else if (res != VK_SUCCESS)
  {
    m_renderingMutex.unlock();
    throw std::runtime_error(
      std::format("Failed to queue image in present - {}", static_cast<uint32_t>(res)));
  }
  m_renderingMutex.unlock();
  return true;
}

void SurfacedAttachment::SetBuffering(uint32_t framesCount)
{
  if (!m_swapchain || framesCount != m_swapchain->image_count)
  {
    m_desiredBuffering = framesCount;
    m_invalidSwapchain = true;
  }
}

uint32_t SurfacedAttachment::GetBuffering() const noexcept
{
  return static_cast<uint32_t>(m_images.size());
}

VkAttachmentDescription SurfacedAttachment::BuildDescription() const noexcept
{
  VkAttachmentDescription description{};
  {
    description.format = GetInternalFormat();
    description.samples = utils::CastInterfaceEnum2Vulkan<VkSampleCountFlagBits>(m_samplesCount);
    description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
  }
  return description;
}

void SurfacedAttachment::TransferLayout(VkImageLayout newLayout) noexcept
{
  m_layouts[m_activeImage].TransferLayout(newLayout);
}

void SurfacedAttachment::Resize(const VkExtent2D & new_extent) noexcept
{
  // do nothing because resizing handled in AcquireForRend
  m_invalidSwapchain = true;
}

// ---------------------------- Private -----------------

void SurfacedAttachment::DestroySwapchain() noexcept
{
  GetContext().WaitForIdle();
  if (!m_imageViews.empty())
    m_swapchain->destroy_image_views(m_imageViews);
  vkb::destroy_swapchain(*m_swapchain);
  // we can delete semaphore directly, because we have waited for gpu's idle
  for (auto sem : m_imageAvailabilitySemaphores)
    vkDestroySemaphore(GetContext().GetDevice(), sem, nullptr);
  m_images.clear();
  m_imageViews.clear();
  m_imageAvailabilitySemaphores.clear();
  m_layouts.clear();
}

} // namespace RHI::vulkan
