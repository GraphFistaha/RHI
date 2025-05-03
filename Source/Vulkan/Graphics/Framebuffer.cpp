#include "Framebuffer.hpp"

#include <format>

#include "../Utils/CastHelper.hpp"
#include "../VulkanContext.hpp"
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
  if (m_attachmentsChanged)
  {
    if (m_attachments.empty())
      throw std::runtime_error("Framebuffer has no attachments");

    std::vector<VkAttachmentDescription> newAttachmentsDescription;
    newAttachmentsDescription.reserve(m_attachments.size());
    for (auto && attachment : m_attachments)
    {
      if (attachment)
      {
        attachment->Invalidate();
        newAttachmentsDescription.push_back(attachment->BuildDescription());
      }
      else
      {
        newAttachmentsDescription.push_back(VkAttachmentDescription{});
      }
    }
    m_attachmentDescriptions = std::move(newAttachmentsDescription);

    // set attachments to render Pass
    m_renderPass.SetAttachments(m_attachmentDescriptions);
    //build render pass
    m_renderPass.Invalidate();

    uint32_t buffersCount = m_attachments[0]->GetBuffering();
    auto extent = m_attachments[0]->GetInternalExtent();
    // all attachments must have equal count of buffers
    assert(std::all_of(m_attachments.begin(), m_attachments.end(),
                       [buffersCount, extent](IInternalAttachment * att) {
                         return buffersCount == att->GetBuffering() &&
                                att->GetInternalExtent() == extent;
                       }));

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
    m_attachmentsChanged = false;
  }
}

void Framebuffer::ForEachAttachment(AttachmentProcessFunc && func)
{
  std::for_each(m_attachments.begin(), m_attachments.end(), std::move(func));
}

IRenderTarget * Framebuffer::BeginFrame()
{
  if (m_attachments.empty())
    return nullptr;

  Invalidate();

  std::vector<VkImageView> renderingImages;
  renderingImages.reserve(m_attachments.size());
  m_imagesAvailabilitySemaphores.reserve(m_attachments.size());

  for (auto && attachment : m_attachments)
  {
    auto [imageView, imgAvailSemaphore] = attachment->AcquireForRendering();
    if (!imageView)
      return nullptr;
    if (imgAvailSemaphore)
      m_imagesAvailabilitySemaphores.push_back(imgAvailSemaphore);
    renderingImages.push_back(imageView);
  }

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
    attachment->FinalRendering(task->GetSemaphore());
  return task;
}

ISubpass * Framebuffer::CreateSubpass()
{
  return m_renderPass.CreateSubpass();
}

void Framebuffer::AddAttachment(uint32_t binding, IAttachment * attachment)
{
  while (m_attachments.size() < binding + 1)
    m_attachments.push_back(nullptr);

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

void Framebuffer::SetFramesCount(uint32_t framesCount)
{
  for (auto * attachment : m_attachments)
    attachment->SetBuffering(framesCount);
  m_attachmentsChanged = true;
}

void Framebuffer::Resize(uint32_t width, uint32_t height)
{
  for (auto * attachment : m_attachments)
    attachment->Resize(VkExtent2D(width, height));
  m_attachmentsChanged = true;
}

} // namespace RHI::vulkan
