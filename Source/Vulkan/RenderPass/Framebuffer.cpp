#include "Framebuffer.hpp"

#include <format>

#include <Attachments/SurfacedAttachment.hpp>
#include <Utils/CastHelper.hpp>
#include <VulkanContext.hpp>

namespace
{

bool operator==(const VkExtent3D & e1, const VkExtent3D & e2)
{
  return std::memcmp(&e1, &e2, sizeof(VkExtent3D)) == 0;
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
  return m_framesCount;
}

void Framebuffer::Invalidate()
{
  bool targetsChanged = false;
  if (m_attachmentsChanged)
  {
    if (m_attachments.empty())
      throw std::runtime_error("Framebuffer has no attachments");

    std::vector<VkAttachmentDescription> newAttachmentsDescription;
    newAttachmentsDescription.reserve(m_attachments.size());
    for (auto * attachment : m_attachments)
    {
      if (attachment)
      {
        attachment->Invalidate();
        newAttachmentsDescription.push_back(attachment->BuildDescription());
      }
    }

    m_attachmentDescriptions = std::move(newAttachmentsDescription);

    uint32_t buffersCount = m_attachments[0]->GetBuffering();
    auto extent = m_attachments[0]->GetInternalExtent();
    // all attachments must have equal count of buffers
    assert(std::all_of(m_attachments.begin(), m_attachments.end(),
                       [buffersCount, extent](IInternalAttachment * att) {
                         return buffersCount == att->GetBuffering() &&
                                att->GetInternalExtent() == extent;
                       }));

    // set attachments to render Pass
    m_renderPass.SetAttachments(m_attachmentDescriptions);
    targetsChanged = true;
  }

  //build render pass
  m_renderPass.Invalidate();

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

void Framebuffer::ForEachAttachment(AttachmentProcessFunc && func)
{
  std::for_each(m_attachments.begin(), m_attachments.end(), func);
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

IRenderTarget * Framebuffer::BeginFrame()
{
  if (m_attachments.empty())
    return nullptr;

  Invalidate();

  std::vector<VkImageView> renderingImages;
  std::vector<VkSemaphore> semaphores;
  renderingImages.reserve(m_attachments.size());
  semaphores.reserve(m_attachments.size());
  bool success = true;

  auto processAttachment =
    [&renderingImages, &semaphores, &success](IInternalAttachment * attachment)
  {
    if (attachment && success)
    {
      auto [imageView, imgAvailSemaphore] = attachment->AcquireForRendering();
      if (!imageView)
        success = false;
      if (imgAvailSemaphore)
        semaphores.push_back(imgAvailSemaphore);
      renderingImages.push_back(imageView);
    }
  };

  std::for_each(m_attachments.begin(), m_attachments.end(), processAttachment);
  if (!success)
  {
    m_attachmentsChanged = true;
    return nullptr;
  }

  m_imagesAvailabilitySemaphores = std::move(semaphores);
  m_activeTarget = (m_activeTarget + 1) % m_targets.size();

  m_targets[m_activeTarget].SetAttachments(std::move(renderingImages));
  m_targets[m_activeTarget].Invalidate(); // rebuilds VkFramebuffer if need it
  return &m_targets[m_activeTarget];
}

IAwaitable * Framebuffer::EndFrame()
{
  AsyncTask * task =
    m_renderPass.Draw(m_targets[m_activeTarget], std::move(m_imagesAvailabilitySemaphores));
  for (auto && attachment : m_attachments)
  {
    if (attachment)
      attachment->FinalRendering(task->GetSemaphore());
  }
  return task;
}

ISubpass * Framebuffer::CreateSubpass()
{
  return m_renderPass.CreateSubpass();
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
  m_renderPass.ForEachSubpass([](Subpass & sp) { sp.SetDirtyCacheCommands(); });
  m_attachmentsChanged = true;
}

RHI::TexelIndex Framebuffer::GetExtent() const
{
  auto internalExtent = m_attachments[0]->GetInternalExtent();
  return {internalExtent.width, internalExtent.height, internalExtent.depth};
}

} // namespace RHI::vulkan
