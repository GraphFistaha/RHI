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
  m_renderPass.SetInvalid();
  m_attachmentsChanged = true;
  for (auto && attachment : m_protoAttachments)
    attachment.second.second = true;
}

void SwapchainBase::Invalidate()
{
  if (m_attachmentsChanged)
  {
    std::unordered_map<uint32_t, AttachmentSet> new_sets;
    for (auto && [idx, protoData] : m_protoAttachments)
    {
      AttachmentSet new_set;
      new_set.reserve(m_framesCount);
      for (size_t i = 0; i < m_framesCount; ++i)
      {
        auto image = m_context.AllocImage(protoData.first);
        new_set.emplace_back(std::move(*image));
      }
      new_sets[idx] = std::move(new_set);
    }

    m_attachments = std::move(new_sets);
    m_activeImageIdx = 0;
    m_attachmentsChanged = false;
  }

  while (m_framesCount < m_targets.size())
    m_targets.pop_back();

  while (m_framesCount > m_targets.size())
    m_targets.emplace_back(m_context);

  for (auto && target : m_targets)
  {
    target.SetExtent(m_extent);
    target.BindRenderPass(m_renderPass.GetHandle());
  }
}

size_t SwapchainBase::GetImagesCount() const noexcept
{
  return m_framesCount;
}

void SwapchainBase::SetExtent(VkExtent2D extent) noexcept
{
  m_extent = extent;
}

void SwapchainBase::SetFramesCount(uint32_t framesCount) noexcept
{
  m_framesCount = framesCount;
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
  if (m_extent.width != args.extent[0] || m_extent.height != args.extent[1])
  {
    throw std::invalid_argument("Image's size is not appropriate to swapchain's size");
  }

  //should rebuild framebuffers and renderPass
  m_protoAttachments[binding] = {args, true};
  m_attachmentsChanged = true;
}

} // namespace RHI::vulkan
