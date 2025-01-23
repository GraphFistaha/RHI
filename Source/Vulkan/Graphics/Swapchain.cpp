#include "Swapchain.hpp"

#include <format>

#include "../Images/ImageGPU.hpp"
#include "../Utils/CastHelper.hpp"
#include "../VulkanContext.hpp"
#include "RenderPass.hpp"

namespace
{
RHI::ShaderImageSlot GetShaderImageSlotByImageDescription(
  const RHI::ImageDescription & desc) noexcept
{
  if (desc.usage == RHI::ImageUsage::SHADER_INPUT)
    return RHI::ShaderImageSlot::Input;

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

bool Swapchain::FinishImage(uint32_t activeImage, VkSemaphore waitRenderingSemaphore)
{
  if (!waitRenderingSemaphore)
    return true;
  uint64_t waitValue = 1;
  VkSemaphoreWaitInfo info{};
  info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
  info.pSemaphores = &waitRenderingSemaphore;
  info.semaphoreCount = 1;
  info.pValues = &waitValue;
  VkResult result = vkWaitSemaphores(m_context.GetDevice(), &info, UINT64_MAX);
  return result == VK_SUCCESS;
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
        ImageGPU image(m_context, m_context.GetInternalTransferer(), description);
        ImageView view(image, utils::CastInterfaceEnum2Vulkan<VkImageViewType>(description.type));
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
  if (imageIndex == InvalidImageIndex)
  {
    return nullptr;
  }
  m_activeImageIdx = imageIndex;
  m_imageAvailableSemaphore = waitSemaphore;
  return &m_targets[m_activeImageIdx];
}

void Swapchain::FlushFrame()
{
  VkSemaphore passSemaphore =
    m_renderPass.Draw(m_targets[m_activeImageIdx], m_imageAvailableSemaphore);
  FinishImage(m_activeImageIdx, passSemaphore);
  m_imageAvailableSemaphore = VK_NULL_HANDLE;
}

ISubpass * Swapchain::CreateSubpass()
{
  return m_renderPass.CreateSubpass();
}

void Swapchain::AddImageAttachment(uint32_t binding, const ImageDescription & description)
{
  VkAttachmentDescription attachmentDescription{};
  {
    attachmentDescription.format = utils::CastInterfaceEnum2Vulkan<VkFormat>(description.format);
    attachmentDescription.samples =
      utils::CastInterfaceEnum2Vulkan<VkSampleCountFlagBits>(m_samplesCount);
    attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentDescription.finalLayout =
      utils::CastInterfaceEnum2Vulkan<VkImageLayout>(description.format);
    attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
  }

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
