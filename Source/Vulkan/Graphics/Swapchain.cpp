#include "Swapchain.hpp"

#include <format>

#include <VkBootstrap.h>

#include "../Utils/Builders.hpp"
#include "RenderPass.hpp"

namespace RHI::vulkan
{

Swapchain::Swapchain(const Context & ctx, const vk::SurfaceKHR surface)
  : m_context(ctx)
  , m_surface(surface)
  , m_swapchain(std::make_unique<vkb::Swapchain>())
{
  std::tie(m_presentQueueIndex, m_presentQueue) = ctx.GetQueue(QueueType::Present);
  m_renderPass = std::make_unique<RenderPass>(m_context);
  m_usePresentation = !!surface;
  InvalidateSwapchain();
}

Swapchain::~Swapchain()
{
  DestroyHandles();
}

void Swapchain::Invalidate()
{
  m_context.WaitForIdle();
  InvalidateSwapchain();
}

void Swapchain::InvalidateSwapchain()
{
  if (m_usePresentation)
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
  //else offscreen rendering
}

void Swapchain::DestroyHandles() noexcept
{
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

std::pair<uint32_t, uint32_t> Swapchain::GetExtent() const
{
  return {m_swapchain->extent.width, m_swapchain->extent.height};
}

IRenderTarget * Swapchain::AcquireFrame()
{
  uint32_t imageIndex = InvalidImageIndex;
  auto res = vkAcquireNextImageKHR(m_context.GetDevice(), m_swapchain->swapchain, UINT64_MAX,
                                   m_imageAvailabilitySemaphores[m_activeSemaphore], VK_NULL_HANDLE,
                                   &imageIndex);
  if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
  {
    Invalidate();
    return nullptr;
  }
  else if (res != VK_SUCCESS)
  {
    m_context.Log(LogMessageStatus::LOG_ERROR,
                  std::format("Failed to acquire swap chain image - {}",
                              static_cast<uint32_t>(res)));
    return nullptr;
  }

  m_activeImage = imageIndex;
  auto && activeTarget = m_targets[m_activeImage];

  m_renderPass->BindRenderTarget(&activeTarget);
  m_renderPass->Invalidate();
  activeTarget.BindRenderPass(m_renderPass->GetHandle());
  activeTarget.Invalidate();
  return &activeTarget;
}

void Swapchain::FlushFrame()
{
  VkSemaphore passSemaphore = m_renderPass->Draw(m_imageAvailabilitySemaphores[m_activeSemaphore]);

  const VkSwapchainKHR swapchains[] = {GetHandle()};
  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = &passSemaphore;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapchains;
  presentInfo.pImageIndices = &m_activeImage;
  presentInfo.pResults = nullptr; // Optional
  auto res = vkQueuePresentKHR(m_presentQueue, &presentInfo);
  if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
  {
    Invalidate();
    return;
  }
  else if (res != VK_SUCCESS)
  {
    m_context.Log(LogMessageStatus::LOG_ERROR,
                  std::format("Failed to queue image in present - {}", static_cast<uint32_t>(res)));
  }
  m_activeSemaphore = (m_activeSemaphore + 1) % m_imageAvailabilitySemaphores.size();
}

ISubpass * Swapchain::CreateSubpass()
{
  return m_renderPass->CreateSubpass();
}

vk::SwapchainKHR Swapchain::GetHandle() const noexcept
{
  return m_swapchain->swapchain;
}

size_t Swapchain::GetImagesCount() const noexcept
{
  return m_swapchain->image_count;
}

} // namespace RHI::vulkan
