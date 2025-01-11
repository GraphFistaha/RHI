#include "PresentativeSwapchain.hpp"

#include <format>

#include <VkBootstrap.h>

#include "../VulkanContext.hpp"
#include "RenderPass.hpp"

namespace RHI::vulkan
{

Swapchain::Swapchain(const Context & ctx, const VkSurfaceKHR surface)
  : SwapchainBase(ctx)
  , m_surface(surface)
  , m_swapchain(std::make_unique<vkb::Swapchain>())
{
  std::tie(m_presentQueueIndex, m_presentQueue) = ctx.GetQueue(QueueType::Present);
}

Swapchain::~Swapchain()
{
  DestroySwapchain();
}

void Swapchain::Invalidate()
{
  if (m_invalidSwapchain || !m_swapchain->swapchain)
  {
    auto [presentIndex, presentQueue] = m_context.GetQueue(QueueType::Present);
    auto [renderIndex, renderQueue] = m_context.GetQueue(QueueType::Graphics);
    vkb::SwapchainBuilder swapchain_builder(m_context.GetGPU(), m_context.GetDevice(), m_surface,
                                            renderIndex, presentIndex);
    auto swap_ret = swapchain_builder.set_old_swapchain(*m_swapchain).build();
    if (!swap_ret)
      throw std::runtime_error("Failed to create Vulkan swapchain - " + swap_ret.error().message());

    DestroySwapchain();
    m_renderPass.SetInvalid();

    *m_swapchain = swap_ret.value();
    m_swapchainImages = m_swapchain->get_images().value();
    m_swapchainImageViews = m_swapchain->get_image_views().value();
    for (auto && view : m_swapchainImageViews)
      m_imageAvailabilitySemaphores.push_back(utils::CreateVkSemaphore(m_context.GetDevice()));

    InitRenderTargets(m_swapchain->extent, m_swapchain->image_count);

    // add swapchain images as attachment to RenderTargets
    ForEachRenderTarget(
      [it = m_swapchainImageViews.begin(),
       format = m_swapchain->image_format](RenderTarget & target) mutable
      {
        target.BindAttachment(0, FramebufferAttachment{*it, format, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                                       VK_SAMPLE_COUNT_1_BIT});
        it++;
      });

    m_renderPass.ForEachSubpass(
      [](Subpass & sb)
      {
        sb.GetLayout().ResetAttachments();
        sb.SetImageAttachmentUsage(0, RHI::ShaderImageSlot::Color);
      });
    m_invalidSwapchain = false;
  }
}

void Swapchain::DestroySwapchain() noexcept
{
  m_context.WaitForIdle();
  if (!m_swapchainImageViews.empty())
    m_swapchain->destroy_image_views(m_swapchainImageViews);
  vkb::destroy_swapchain(*m_swapchain);
  // we can delete semaphore directly, because we have waited for gpu's idle
  for (auto sem : m_imageAvailabilitySemaphores)
    vkDestroySemaphore(m_context.GetDevice(), sem, nullptr);
  m_imageAvailabilitySemaphores.clear();
}

std::pair<uint32_t, VkSemaphore> Swapchain::AcquireImage() noexcept
{
  VkSemaphore signalSemaphore = m_imageAvailabilitySemaphores[m_activeSemaphore];
  uint32_t imageIndex = InvalidImageIndex;
  auto res = vkAcquireNextImageKHR(m_context.GetDevice(), m_swapchain->swapchain, UINT64_MAX,
                                   signalSemaphore, VK_NULL_HANDLE, &imageIndex);
  if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
  {
    Invalidate();
    return {InvalidImageIndex, VK_NULL_HANDLE};
  }
  else if (res != VK_SUCCESS)
  {
    m_context.Log(LogMessageStatus::LOG_ERROR,
                  std::format("Failed to acquire swap chain image - {}",
                              static_cast<uint32_t>(res)));
    return {InvalidImageIndex, VK_NULL_HANDLE};
  }
  return {imageIndex, signalSemaphore};
}

bool Swapchain::PresentImage(uint32_t activeImage, VkSemaphore waitRenderingSemaphore) noexcept
{
  const VkSwapchainKHR swapchains[] = {GetHandle()};
  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = &waitRenderingSemaphore;
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
    m_context.Log(LogMessageStatus::LOG_ERROR,
                  std::format("Failed to queue image in present - {}", static_cast<uint32_t>(res)));
  }
  return true;
}

VkSwapchainKHR Swapchain::GetHandle() const noexcept
{
  return m_swapchain->swapchain;
}

} // namespace RHI::vulkan
