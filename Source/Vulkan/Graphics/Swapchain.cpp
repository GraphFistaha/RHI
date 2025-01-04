#include "Swapchain.hpp"

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
  InvalidateSwapchain();
}

Swapchain::~Swapchain()
{
  DestroyHandles();
}

void Swapchain::Invalidate()
{
  InvalidateSwapchain();
}

void Swapchain::InvalidateSwapchain()
{
  auto [presentIndex, presentQueue] = m_context.GetQueue(QueueType::Present);
  auto [renderIndex, renderQueue] = m_context.GetQueue(QueueType::Graphics);
  vkb::SwapchainBuilder swapchain_builder(m_context.GetGPU(), m_context.GetDevice(), m_surface,
                                          renderIndex, presentIndex);
  auto swap_ret = swapchain_builder.set_old_swapchain(*m_swapchain).build();
  if (!swap_ret)
    throw std::runtime_error("Failed to create Vulkan swapchain - " + swap_ret.error().message());

  DestroyHandles();

  *m_swapchain = swap_ret.value();
  m_swapchainImages = m_swapchain->get_images().value();
  m_swapchainImageViews = m_swapchain->get_image_views().value();
  m_extent = m_swapchain->extent;

  for (auto && imageView : m_swapchainImageViews)
  {
    m_imageAvailabilitySemaphores.emplace_back(utils::CreateVkSemaphore(m_context.GetDevice()));
    auto && target = m_targets.emplace_back(m_context);
    FramebufferAttachment colorAttachment{imageView, m_swapchain->image_format,
                                          VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_SAMPLE_COUNT_1_BIT};
    target.AddAttachment(colorAttachment);
    target.SetExtent(m_swapchain->extent);
    target.BindRenderPass(m_renderPass->GetHandle());
  }
}

void Swapchain::DestroyHandles() noexcept
{
  m_context.WaitForIdle();
  m_targets.clear();
  if (!m_swapchainImageViews.empty())
    m_swapchain->destroy_image_views(m_swapchainImageViews);
  vkb::destroy_swapchain(*m_swapchain);
  for (auto && semaphore : m_imageAvailabilitySemaphores)
  {
    if (!!semaphore)
      vkDestroySemaphore(m_context.GetDevice(), semaphore, nullptr);
  }
  m_imageAvailabilitySemaphores.clear();
}

uint32_t Swapchain::AcquireImage(VkSemaphore signalSemaphore) noexcept
{
  uint32_t imageIndex = InvalidImageIndex;
  auto res = vkAcquireNextImageKHR(m_context.GetDevice(), m_swapchain->swapchain, UINT64_MAX,
                                   signalSemaphore, VK_NULL_HANDLE, &imageIndex);
  if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
  {
    Invalidate();
    return InvalidImageIndex;
  }
  else if (res != VK_SUCCESS)
  {
    m_context.Log(LogMessageStatus::LOG_ERROR,
                  std::format("Failed to acquire swap chain image - {}",
                              static_cast<uint32_t>(res)));
    return InvalidImageIndex;
  }
  return imageIndex;
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
