#include "Framebuffer.hpp"

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

Framebuffer::Framebuffer(Context & ctx)
  : OwnedBy<Context>(ctx)
  , m_renderPass(ctx)
{
}

Framebuffer::~Framebuffer()
{
}

size_t Framebuffer::GetImagesCount() const noexcept
{
  return m_framesCount;
}

std::pair<uint32_t, VkSemaphore> Framebuffer::AcquireImage()
{
  Invalidate();
  return {static_cast<uint32_t>((m_activeImageIdx + 1) % m_targets.size()), VK_NULL_HANDLE};
}

bool Framebuffer::FinishImage(uint32_t activeImage, AsyncTask * task)
{
  return true;
}

void Framebuffer::Invalidate()
{
  if (m_framesCountChanged)
  {
    std::vector<RenderTarget> new_targets;
    new_targets.reserve(m_framesCount);
    for (uint32_t i = 0; i < m_framesCount; ++i)
      new_targets.emplace_back(GetContext());
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

void Framebuffer::InvalidateAttachments()
{
  for (auto && target : m_targets)
  {
    uint32_t binding = 0;
    for (auto && description : m_imageDescriptions)
    {
      if (m_ownedImages[binding])
      {
        auto image = std::make_unique<ImageGPU>(GetContext(), description);
        ImageView view(GetContext());
        view.AssignImage(image.get(),
                         utils::CastInterfaceEnum2Vulkan<VkImageViewType>(description.type));
        target.AddAttachment(binding, std::move(image), std::move(view));
      }
      binding++;
    }
  }
}

void Framebuffer::RequireSwapchainHasAttachmentsCount(uint32_t count)
{
  while (m_imageDescriptions.size() < count)
  {
    m_imageDescriptions.emplace_back();
    m_attachmentDescriptions.emplace_back();
    m_ownedImages.emplace_back();
  }
}

IRenderTarget * Framebuffer::BeginFrame()
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

IAwaitable * Framebuffer::EndFrame()
{
  AsyncTask * task = m_renderPass.Draw(m_targets[m_activeImageIdx], m_imageAvailableSemaphore);
  FinishImage(m_activeImageIdx, task);
  m_imageAvailableSemaphore = VK_NULL_HANDLE;
  return task;
}

ISubpass * Framebuffer::CreateSubpass()
{
  return m_renderPass.CreateSubpass();
}

void Framebuffer::AddImageAttachment(uint32_t binding, const RHI::IImageGPU & image)
{
    //TODO: rewrite
  /*VkAttachmentDescription attachmentDescription = BuildAttachmentDescription(description);

  RequireSwapchainHasAttachmentsCount(binding + 1);

  m_ownedImages[binding] = true;
  m_imageDescriptions[binding] = description;
  m_imageDescriptions[binding].extent = m_extent;
  m_imageDescriptions[binding].samples = m_samplesCount;
  m_attachmentDescriptions[binding] = attachmentDescription;
  m_attachmentsChanged = true;*/
}

void Framebuffer::ClearImageAttachments() noexcept
{
  m_imageDescriptions.clear();
  m_attachmentsChanged = true;
}

void Framebuffer::SetExtent(const ImageExtent & extent) noexcept
{
  if (extent != m_extent)
  {
    m_extent = extent;
    m_extentChanged = true;
  }
}

void Framebuffer::SetMultisampling(RHI::SamplesCount samples) noexcept
{
  if (m_samplesCount != samples)
  {
    m_samplesCount = samples;
    m_samplesCountChanged;
  }
}

void Framebuffer::SetFramesCount(uint32_t framesCount) noexcept
{
  if (m_framesCount != framesCount)
  {
    m_framesCount = framesCount;
    m_framesCountChanged = true;
  }
}

} // namespace RHI::vulkan
