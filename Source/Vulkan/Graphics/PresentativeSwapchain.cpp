#include "PresentativeSwapchain.hpp"

#include <format>

#include <VkBootstrap.h>

#include "../Images/NonOwningImageGPU.hpp"
#include "../Utils/CastHelper.hpp"
#include "../VulkanContext.hpp"
#include "RenderPass.hpp"

namespace RHI::vulkan
{

PresentativeSwapchain::PresentativeSwapchain(Context & ctx, const VkSurfaceKHR surface)
  : Swapchain(ctx)
  , m_surface(surface)
  , m_swapchain(std::make_unique<vkb::Swapchain>())
{
  std::tie(m_presentQueueIndex, m_presentQueue) = ctx.GetQueue(QueueType::Present);
}

PresentativeSwapchain::~PresentativeSwapchain()
{
  DestroySwapchain();
}

void PresentativeSwapchain::Invalidate()
{
  if (m_invalidSwapchain || !m_swapchain->swapchain)
  {
    auto [renderIndex, renderQueue] = GetContext().GetQueue(QueueType::Graphics);
    vkb::SwapchainBuilder swapchain_builder(GetContext().GetGPU(), GetContext().GetDevice(),
                                            m_surface, renderIndex, m_presentQueueIndex);
    auto swap_ret = swapchain_builder.set_old_swapchain(*m_swapchain).build();
    if (!swap_ret)
      throw std::runtime_error("Failed to create Vulkan swapchain - " + swap_ret.error().message());

    DestroySwapchain();

    *m_swapchain = swap_ret.value();
    m_swapchainImages = m_swapchain->get_images().value();
    m_swapchainImageViews = m_swapchain->get_image_views().value();
    for (auto && view : m_swapchainImageViews)
      m_imageAvailabilitySemaphores.push_back(utils::CreateVkSemaphore(GetContext().GetDevice()));
    m_invalidSwapchain = false;

    auto new_extent = m_swapchain->extent;
    SetFramesCount(m_swapchain->image_count);
    SetExtent({new_extent.width, new_extent.height, 1});
  }
  Swapchain::Invalidate();
}

void PresentativeSwapchain::InvalidateAttachments()
{
  constexpr uint32_t binding = 0;

  VkAttachmentDescription attachmentDescription{};
  {
    attachmentDescription.format = m_swapchain->image_format;
    attachmentDescription.samples =
      utils::CastInterfaceEnum2Vulkan<VkSampleCountFlagBits>(m_samplesCount);
    attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
  }

  ImageCreateArguments imageDescription{};
  {
    imageDescription.samples = m_samplesCount;
    imageDescription.extent = {m_swapchain->extent.width, m_swapchain->extent.height, 1};
    imageDescription.mipLevels = 1;
    imageDescription.shared = false;
    imageDescription.type = RHI::ImageType::Image2D;
    imageDescription.format = RHI::ImageFormat::RGBA8;
  }

  RequireSwapchainHasAttachmentsCount(1);
  m_ownedImages[binding] = false;
  m_imageDescriptions[binding] = imageDescription;
  m_attachmentDescriptions[binding] = attachmentDescription;

  // add swapchain images as attachment to RenderTargets
  auto imgs_it = m_swapchainImages.begin();
  auto views_it = m_swapchainImageViews.begin();
  for (auto && target : m_targets)
  {
    auto image = std::make_unique<NonOwningImageGPU>(GetContext(), imageDescription, *imgs_it,
                                                     VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    ImageView view(*image, *views_it);
    target.AddAttachment(binding, std::move(image), std::move(view));
    imgs_it++;
    views_it++;
  }
  m_attachmentsChanged = true;
  Swapchain::InvalidateAttachments();
}


void PresentativeSwapchain::DestroySwapchain() noexcept
{
  GetContext().WaitForIdle();
  if (!m_swapchainImageViews.empty())
    m_swapchain->destroy_image_views(m_swapchainImageViews);
  vkb::destroy_swapchain(*m_swapchain);
  // we can delete semaphore directly, because we have waited for gpu's idle
  for (auto sem : m_imageAvailabilitySemaphores)
    vkDestroySemaphore(GetContext().GetDevice(), sem, nullptr);
  m_imageAvailabilitySemaphores.clear();
}

std::pair<uint32_t, VkSemaphore> PresentativeSwapchain::AcquireImage()
{
  Invalidate();
  VkSemaphore signalSemaphore = m_imageAvailabilitySemaphores[m_activeSemaphore];
  uint32_t imageIndex = InvalidImageIndex;
  auto res = vkAcquireNextImageKHR(GetContext().GetDevice(), m_swapchain->swapchain, UINT64_MAX,
                                   signalSemaphore, VK_NULL_HANDLE, &imageIndex);
  if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
  {
    m_invalidSwapchain = true;
    return {InvalidImageIndex, VK_NULL_HANDLE};
  }
  else if (res != VK_SUCCESS)
  {
    throw std::runtime_error(
      std::format("Failed to acquire swap chain image - {}", static_cast<uint32_t>(res)));
  }
  return {imageIndex, signalSemaphore};
}

bool PresentativeSwapchain::FinishImage(uint32_t activeImage, AsyncTask * task)
{
  const VkSwapchainKHR swapchains[] = {GetHandle()};
  VkSemaphore sem = task->GetSemaphore();
  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = &sem;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapchains;
  presentInfo.pImageIndices = &activeImage;
  presentInfo.pResults = nullptr; // Optional
  auto res = vkQueuePresentKHR(m_presentQueue, &presentInfo);
  m_activeSemaphore = (m_activeSemaphore + 1u) % m_imageAvailabilitySemaphores.size();
  if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
  {
    Invalidate();
    return false;
  }
  else if (res != VK_SUCCESS)
  {
    throw std::runtime_error(
      std::format("Failed to queue image in present - {}", static_cast<uint32_t>(res)));
  }
  return true;
}

VkSwapchainKHR PresentativeSwapchain::GetHandle() const noexcept
{
  return m_swapchain->swapchain;
}

} // namespace RHI::vulkan
