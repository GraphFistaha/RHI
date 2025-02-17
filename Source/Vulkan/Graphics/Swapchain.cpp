#include "Swapchain.hpp"

#include <format>

#include "../Images/ImageGPU.hpp"
#include "../Images/ImageInfo.hpp"
#include "../Utils/CastHelper.hpp"
#include "../VulkanContext.hpp"
#include "RenderPass.hpp"

namespace
{
RHI::ShaderImageSlot GetShaderImageSlotByImageDescription(
  const RHI::ImageCreateArguments & desc) noexcept
{
  if (desc.format == RHI::ImageFormat::DEPTH_STENCIL || desc.format == RHI::ImageFormat::DEPTH)
    return RHI::ShaderImageSlot::DepthStencil;

  return RHI::ShaderImageSlot::Color;
}
} // namespace

namespace RHI::vulkan
{

Swapchain::Swapchain(const Context & ctx)
  : m_context(ctx)
  , m_renderPass(ctx)
{
}

Swapchain::~Swapchain()
{
}

size_t Swapchain::GetImagesCount() const noexcept
{
  return m_framesCount;
}

std::pair<uint32_t, VkSemaphore> Swapchain::AcquireImage()
{
  Invalidate();
  return {static_cast<uint32_t>((m_activeImageIdx + 1) % m_targets.size()), VK_NULL_HANDLE};
}

bool Swapchain::FinishImage(uint32_t activeImage, AsyncTask * task)
{
  return true;
}

void Swapchain::Invalidate()
{
  if (m_framesCountChanged)
  {
    std::vector<RenderTarget> new_targets;
    new_targets.reserve(m_framesCount);
    for (uint32_t i = 0; i < m_framesCount; ++i)
      new_targets.emplace_back(m_context);
    m_targets = std::move(new_targets);
    m_framesCountChanged = false;
    m_extentChanged = true;
    m_attachmentsChanged = true;
  }

  // edit image desciptions that they include user settings
  if (m_extentChanged || m_samplesCountChanged)
  {
    for (auto && description : m_imageDescriptions)
    {
      description.extent = m_extent;
      description.samples = m_samplesCount;
    }
    const VkExtent3D extent{m_extent[0], m_extent[1], m_extent[2]};
    for (auto && target : m_targets)
      target.SetExtent(extent);
    m_extentChanged = false;
    m_samplesCountChanged = false;
    m_attachmentsChanged = true;
  }

  if (m_attachmentsChanged)
  {
    InvalidateAttachments();
    // build render pass
    m_renderPass.SetAttachments(m_attachmentDescriptions);
    m_renderPass.ForEachSubpass(
      [this](Subpass & sb)
      {
        uint32_t idx = 0;
        for (auto && description : m_imageDescriptions)
        {
          if (!m_ownedImages[idx])
            sb.GetLayout().SetAttachment(GetShaderImageSlotByImageDescription(description), idx++);
        }
      });
    m_renderPass.Invalidate();
    // final building of render targets
    for (auto && target : m_targets)
    {
      target.BindRenderPass(m_renderPass.GetHandle());
      target.Invalidate();
    }
    m_attachmentsChanged = false;
  }
}

void Swapchain::InvalidateAttachments()
{
  for (auto && target : m_targets)
  {
    uint32_t binding = 0;
    for (auto && description : m_imageDescriptions)
    {
      if (m_ownedImages[binding])
      {
        auto image = std::make_unique<ImageGPU>(const_cast<Context &>(m_context), description);
        ImageView view(*image, utils::CastInterfaceEnum2Vulkan<VkImageViewType>(description.type));
        target.AddAttachment(binding, std::move(image), std::move(view));
      }
      binding++;
    }
  }
}

void Swapchain::RequireSwapchainHasAttachmentsCount(uint32_t count)
{
  while (m_imageDescriptions.size() < count)
  {
    m_imageDescriptions.emplace_back();
    m_attachmentDescriptions.emplace_back();
    m_ownedImages.emplace_back();
  }
}

IRenderTarget * Swapchain::AcquireFrame()
{
  auto [imageIndex, waitSemaphore] = AcquireImage();
  m_activeImageIdx = imageIndex;
  m_imageAvailableSemaphore = waitSemaphore;
  if (imageIndex == InvalidImageIndex)
  {
    return nullptr;
  }
  return &m_targets[m_activeImageIdx];
}

IAwaitable * Swapchain::RenderFrame()
{
  AsyncTask * task = m_renderPass.Draw(m_targets[m_activeImageIdx], m_imageAvailableSemaphore);
  FinishImage(m_activeImageIdx, task);
  m_imageAvailableSemaphore = VK_NULL_HANDLE;
  return task;
}

ISubpass * Swapchain::CreateSubpass()
{
  return m_renderPass.CreateSubpass();
}

void Swapchain::AddImageAttachment(uint32_t binding, const ImageCreateArguments & description)
{
  VkAttachmentDescription attachmentDescription = BuildAttachmentDescription(description);

  RequireSwapchainHasAttachmentsCount(binding + 1);

  m_ownedImages[binding] = true;
  m_imageDescriptions[binding] = description;
  m_imageDescriptions[binding].extent = m_extent;
  m_imageDescriptions[binding].samples = m_samplesCount;
  m_attachmentDescriptions[binding] = attachmentDescription;
  m_attachmentsChanged = true;
}

void Swapchain::ClearImageAttachments() noexcept
{
  m_imageDescriptions.clear();
  m_attachmentsChanged = true;
}

void Swapchain::SetExtent(const ImageExtent & extent) noexcept
{
  if (extent != m_extent)
  {
    m_extent = extent;
    m_extentChanged = true;
  }
}

void Swapchain::SetMultisampling(RHI::SamplesCount samples) noexcept
{
  if (m_samplesCount != samples)
  {
    m_samplesCount = samples;
    m_samplesCountChanged;
  }
}

void Swapchain::SetFramesCount(uint32_t framesCount) noexcept
{
  if (m_framesCount != framesCount)
  {
    m_framesCount = framesCount;
    m_framesCountChanged = true;
  }
}

} // namespace RHI::vulkan
