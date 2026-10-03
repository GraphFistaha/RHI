#include "Framebuffer.hpp"

#include <format>

#include <Attachments/SurfacedAttachment.hpp>
#include <Private/Constants.hpp>
#include <RenderPass/AttachmentDescriptionBuilder.hpp>
#include <Utils/CastHelper.hpp>
#include <VulkanContext.hpp>


/// @brief Compare operator for VkAttachmentDescription
static bool operator==(const VkAttachmentDescription & lhs, const VkAttachmentDescription & rhs) noexcept
{
  return std::memcmp(&lhs, &rhs, sizeof(VkAttachmentDescription)) == 0;
}

static bool operator==(const VkExtent3D & e1, const VkExtent3D & e2)
{
  return std::memcmp(&e1, &e2, sizeof(VkExtent3D)) == 0;
}

namespace
{
constexpr VkImageLayout MakeAttachmentFinalLayout(VkFormat format, bool isPresent)
{
  // If the attachment is going to be presented, it must end in PRESENT_SRC_KHR.
  if (isPresent)
    return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  // Depth/stencil attachments typically end in DEPTH_STENCIL_ATTACHMENT_OPTIMAL
  // so they can be reused as attachments in a subsequent pass.
  switch (format)
  {
    case VK_FORMAT_D16_UNORM:
    case VK_FORMAT_X8_D24_UNORM_PACK32:
    case VK_FORMAT_D32_SFLOAT:
    case VK_FORMAT_S8_UINT:
    case VK_FORMAT_D16_UNORM_S8_UINT:
    case VK_FORMAT_D24_UNORM_S8_UINT:
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
      return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    default:
      break;
  }

  // Color attachments end in COLOR_ATTACHMENT_OPTIMAL so they're ready
  // to be used as attachments again without an extra barrier.
  return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
}

} // namespace

namespace RHI::vulkan
{

Framebuffer::Framebuffer(Context & ctx)
  : OwnedBy<Context>(ctx)
  , m_renderPass(ctx, *this)
{
}

Framebuffer::~Framebuffer()
{
}

size_t Framebuffer::GetImagesCount() const noexcept
{
  return m_targets.size();
}

void Framebuffer::Invalidate(VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp)
{
  bool targetsChanged = false;
  //rebuild attachments
  if (m_attachmentsChanged)
  {
    if (m_attachments.empty())
      throw std::runtime_error("Framebuffer has no attachments");

    // collect info about how each attachment is used during render pass
    std::vector<VkImageUsageFlags> attachmentsUsage;
    attachmentsUsage.resize(m_attachments.size(), 0);
    m_renderPass.CollectAttachmentsUsageInfo(attachmentsUsage);

    // rebuilt attachment for each usage
    for (size_t i = 0; auto * attachment : m_attachments)
    {
      if (attachment)
      {
        attachment->Invalidate(attachmentsUsage[i]);
      }
      ++i;
    }

    // build description for each attachment
    std::vector<VkAttachmentDescription> newAttachmentsDescription;
    newAttachmentsDescription.reserve(m_attachments.size());
    for (auto * att : m_attachments)
    {
      if (att)
      {
        newAttachmentsDescription.push_back(
          BuildPassAttachmentDescription(*att,
                                         MakeAttachmentFinalLayout(att->GetInternalFormat(),
                                                                   att->IsPresent()),
                                         loadOp, storeOp));
      }
    }

    // if attachments have been changed - rebuild RenderPass
    if (m_attachmentDescriptions != newAttachmentsDescription)
    {
      m_renderPass.SetInvalid();
    }

    m_attachmentDescriptions = std::move(newAttachmentsDescription);

    const uint32_t buffersCount = m_attachments[0]->GetBuffering();
    const VkExtent3D extent = m_attachments[0]->GetInternalExtent();
    // all attachments must have equal count of buffers
    assert(std::all_of(m_attachments.begin(), m_attachments.end(),
                       [buffersCount, extent](IInternalAttachment * att)
                       {
                         return buffersCount == att->GetBuffering() &&
                                att->GetInternalExtent() == extent;
                       }));

    targetsChanged = true;
  }

  //rebuild render pass
  m_renderPass.Invalidate();

  //rebuild RenderTarget(VkFramebuffer)
  if (targetsChanged)
  {
    uint32_t buffersCount = m_attachments[0]->GetBuffering();
    auto extent = m_attachments[0]->GetInternalExtent();
    if (m_targets.size() != buffersCount)
    {
      while (m_targets.size() > buffersCount)
        m_targets.pop_back();

      while (m_targets.size() < buffersCount)
        m_targets.emplace_back(GetContext());
    }

    // build RenderTargets
    for (auto && target : m_targets)
    {
      target.SetExtent(extent);
      target.BindRenderPass(m_renderPass.GetHandle());
    }
    targetsChanged = false;
  }
}

std::span<IInternalAttachment *> Framebuffer::GetAttachments() noexcept
{
  return m_attachments;
}

IInternalAttachment * Framebuffer::GetAttachment(uint32_t idx) const
{
  return m_attachments[idx];
}

RHI::SamplesCount Framebuffer::CalcSamplesCount() const noexcept
{
  // find first not SurfacedAttachment and return it's value
  for (auto * attachment : m_attachments)
  {
    if (dynamic_cast<GenericAttachment *>(attachment) != nullptr)
      return attachment->GetSamplesCount();
  }
  return RHI::SamplesCount::One;
}

std::span<const VkAttachmentDescription> Framebuffer::GetAttachementsDescription() const noexcept
{
  return m_attachmentDescriptions;
}

RenderTarget * Framebuffer::BeginFrame()
{
  if (m_attachments.empty())
    return nullptr;

  MultibufferVector<VkImageView> renderingImages;
  MultibufferVector<VkSemaphore> semaphores;
  MultibufferVector<VkClearValue> clearValues;
  bool success = true;

  for (auto * attachment : m_attachments)
  {
    if (attachment && success)
    {
      auto [imageView, imgAvailSemaphore] = attachment->AcquireForRendering();
      if (!imageView)
      {
        success = false;
        break;
      }
      if (imgAvailSemaphore)
        semaphores.push_back(imgAvailSemaphore);
      renderingImages.push_back(imageView);
      clearValues.push_back(attachment->GetClearValue());
    }
  }

  if (!success)
  {
    m_attachmentsChanged = true;
    return nullptr;
  }

  m_activeTarget = (m_activeTarget + 1) % m_targets.size();
  //AcquireForRendering can return random imageView set, so probably it could rebuild VkFramebuffer for each frame
  m_targets[m_activeTarget].SetAttachments(std::move(renderingImages), std::move(clearValues),
                                           std::move(semaphores));
  m_targets[m_activeTarget].RebuildFramebuffer(); // rebuilds VkFramebuffer if need it
  return &m_targets[m_activeTarget];
}

void Framebuffer::RecordCommands(details::CommandBuffer & commands)
{
  m_renderPass.RecordCommands(commands, m_targets[m_activeTarget]);
}

void Framebuffer::CollectResources(std::vector<ResourcePtr> & resources) const
{
  resources.insert(resources.end(), m_attachments.begin(), m_attachments.end());
  m_renderPass.CollectResources(resources);
}

void Framebuffer::EndFrame(VkSemaphore renderPassSemaphore)
{
  for (auto && attachment : m_attachments)
  {
    if (attachment)
      attachment->FinalRendering(renderPassSemaphore);
  }
}

void Framebuffer::SetSubpass(uint32_t index, PipelinePtr pipeline, PipelineProcessPtr process)
{
  m_renderPass.SetSubpass(index, std::move(pipeline), std::move(process));
}

void Framebuffer::AddAttachment(uint32_t binding, IAttachment * attachment)
{
  while (m_attachments.size() < binding + 1)
  {
    m_attachments.push_back(nullptr);
  }

  if (IInternalAttachment * ptr = dynamic_cast<IInternalAttachment *>(attachment))
  {
    //ptr->SetBuffering(m_framesCount);
    m_attachments[binding] = ptr;
    m_attachmentsChanged = true;
  }
  else
  {
    throw std::runtime_error("Failed to cast ITexture * to IInternalAttachment *");
  }
}

void Framebuffer::ClearAttachments() noexcept
{
  m_attachments.clear();
  m_attachmentsChanged = true;
}

void Framebuffer::Resize(uint32_t width, uint32_t height)
{
  for (auto * attachment : m_attachments)
    if (attachment)
      attachment->Resize(VkExtent2D(width, height));
  //TODO: return
  //m_renderPass.ForEachSubpass([](SubpassLayout & sp) { sp.SetDirtyCommands(); });
  m_attachmentsChanged = true;
}

RHI::TexelIndex Framebuffer::GetExtent() const
{
  auto internalExtent = m_attachments[0]->GetInternalExtent();
  return {static_cast<texel_t>(internalExtent.width), static_cast<texel_t>(internalExtent.height),
          static_cast<texel_t>(internalExtent.depth)};
}

} // namespace RHI::vulkan
