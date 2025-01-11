#include "Swapchain.hpp"

#include <format>

#include "../VulkanContext.hpp"
#include "RenderPass.hpp"

namespace RHI::vulkan
{

SwapchainBase::SwapchainBase(const Context & ctx)
  : m_context(ctx)
  , m_renderPass(ctx)
{
}

SwapchainBase::~SwapchainBase()
{
}

void SwapchainBase::SetInvalid()
{
  m_invalidSwapchain = true;
}

size_t SwapchainBase::GetImagesCount() const noexcept
{
  return m_targets.size();
}

void SwapchainBase::InitRenderTargets(VkExtent2D extent, size_t frames_count)
{
  while (frames_count < m_targets.size())
    m_targets.pop_back();

  while (frames_count > m_targets.size())
    m_targets.emplace_back(m_context);

  for (auto && target : m_targets)
  {
    target.SetExtent(extent);
    target.BindRenderPass(m_renderPass.GetHandle());
  }
}

void SwapchainBase::ForEachRenderTarget(std::function<void(RenderTarget &)> && func)
{
  std::for_each(m_targets.begin(), m_targets.end(), std::move(func));
}

IRenderTarget * SwapchainBase::AcquireFrame()
{
  Invalidate();
  auto [imageIndex, waitSemaphore] = AcquireImage();
  if (imageIndex == InvalidImageIndex)
  {
    return nullptr;
  }

  m_cachedActiveImage = imageIndex;
  m_cachedPresentSemaphore = waitSemaphore;
  auto && activeTarget = m_targets[m_cachedActiveImage];

  m_renderPass.BindRenderTarget(&activeTarget);
  m_renderPass.Invalidate();
  activeTarget.BindRenderPass(m_renderPass.GetHandle());
  activeTarget.Invalidate();
  return &activeTarget;
}

void SwapchainBase::FlushFrame()
{
  VkSemaphore passSemaphore = m_renderPass.Draw(m_cachedPresentSemaphore);
  PresentImage(m_cachedActiveImage, passSemaphore);
  m_cachedPresentSemaphore = VK_NULL_HANDLE;
  m_cachedActiveImage = InvalidImageIndex;
}

ISubpass * SwapchainBase::CreateSubpass()
{
  return m_renderPass.CreateSubpass();
}

void SwapchainBase::AddImageAttachment(uint32_t binding, const ImageCreateArguments & args)
{
  if (m_protoAttachments.size() <= binding)
    m_protoAttachments.resize(binding + 1);

  m_protoAttachments[binding] = args;
}

} // namespace RHI::vulkan
