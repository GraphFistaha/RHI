#include "SurfacedAttachment.hpp"

#include <format>

#include <VkBootstrap.h>

#include "../Graphics/RenderPass.hpp"
#include "../Images/InternalImageTraits.hpp"
#include "../Utils/CastHelper.hpp"
#include "../VulkanContext.hpp"

namespace RHI::vulkan
{

static constexpr VkSurfaceFormatKHR g_vkFormat{utils::CastInterfaceEnum2Vulkan<VkFormat>(
                                                 SurfacedAttachment::g_imagesFormat),
                                               VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};

SurfacedAttachment::SurfacedAttachment(Context & ctx, const VkSurfaceKHR surface,
                                       RHI::RenderBuffering buffering)
  : OwnedBy<Context>(ctx)
  , m_surface(surface)
  , m_swapchain(std::make_unique<vkb::Swapchain>())
  , m_desiredBuffering(static_cast<uint32_t>(buffering))
{
  std::tie(m_presentQueueIndex, m_presentQueue) = ctx.GetQueue(QueueType::Graphics);
}

SurfacedAttachment::~SurfacedAttachment()
{
  DestroySwapchain();
}

std::future<DownloadResult> SurfacedAttachment::DownloadImage(HostImageFormat format,
                                                              const TextureRegion & region)
{
  return GetContext().GetTransferer().DownloadImage(*this, format, region);
}

ImageCreateArguments SurfacedAttachment::GetDescription() const noexcept
{
  ImageCreateArguments description{};
  {
    description.mipLevels = 1;
    description.shared = false;
    description.type = RHI::ImageType::Image2D;
    auto extent = GetInternalExtent();
    description.extent = {extent.width, extent.height, extent.depth};
    description.format = g_imagesFormat;
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
  return m_swapchain && m_swapchain->swapchain ? m_swapchain->image_format : g_vkFormat.format;
}

VkExtent3D SurfacedAttachment::GetInternalExtent() const noexcept
{
  VkExtent2D result{0, 0};
  if (m_swapchain && m_swapchain->swapchain)
  {
    result = m_swapchain->extent;
  }
  else
  {
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(GetContext().GetGPU(), m_surface,
                                              &surfaceCapabilities);
    result = surfaceCapabilities.currentExtent;
  }
  return {result.width, result.height, 1};
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
      swapchain_builder.set_required_min_image_count(m_desiredBuffering);
    swapchain_builder.set_desired_format(g_vkFormat);
    swapchain_builder.set_desired_present_mode(VK_PRESENT_MODE_IMMEDIATE_KHR);
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

uint32_t SurfacedAttachment::GetBuffering() const noexcept
{
  return m_desiredBuffering;
}

RHI::SamplesCount SurfacedAttachment::GetSamplesCount() const noexcept
{
  return g_samplesCount;
}

VkAttachmentDescription SurfacedAttachment::BuildDescription() const noexcept
{
  VkAttachmentDescription description{};
  {
    description.format = GetInternalFormat();
    description.samples = utils::CastInterfaceEnum2Vulkan<VkSampleCountFlagBits>(g_samplesCount);
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
