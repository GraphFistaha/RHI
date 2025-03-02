#include "Framebuffer.hpp"

#include <format>

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

void Framebuffer::Invalidate()
{
  if (m_attachmentsChanged)
  {
    std::vector<VkAttachmentDescription> newAttachmentsDescription;
    for (auto && attachment : m_attachments)
    {
      newAttachmentsDescription.push_back(attachment ? attachment->BuildDescription()
                                                     : VkAttachmentDescription{});
    }
    m_attachmentDescriptions = std::move(newAttachmentsDescription);

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

//void Framebuffer::InvalidateAttachments()
//{
//TODO: Rewrite
/*for (auto&& target : m_targets)
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
  }*/
//}

IRenderTarget * Framebuffer::BeginFrame()
{
  for (auto && attachment : m_attachments)
  {
    auto [imageIndex, imgAvailSemaphore] = attachment->AcquireNextImage();
    m_selectedImageIndices.push_back(imageIndex);
    m_imagesAvailabilitySemaphores.push_back(imgAvailSemaphore);
    attachment->GetImage(imageIndex, ImageUsage::FramebufferAttachment);
  }
  // How to build std::vector<ImageView>? Who is owner of ImageView
  // how to find appropriate RenderTarget by these attachments? to reuse its recreation
  m_targets[m_activeTarget].SetAttachments();
  return &m_targets[m_activeTarget];
}

IAwaitable * Framebuffer::EndFrame()
{
  AsyncTask * task =
    m_renderPass.Draw(m_targets[m_activeTarget], std::move(m_imagesAvailabilitySemaphores));
  for (auto && attachment : m_attachments)
    attachment->FinishImage(task->GetSemaphore());

  m_activeTarget = (m_activeTarget + 1) % m_targets.size();
  m_imagesAvailabilitySemaphores.clear();
  m_selectedImageIndices.clear();
  return task;
}

ISubpass * Framebuffer::CreateSubpass()
{
  return m_renderPass.CreateSubpass();
}

void Framebuffer::AddImageAttachment(uint32_t binding, std::shared_ptr<IImageGPU> image)
{
  assert(dynamic_cast<IAttachment *>(image.get()) != nullptr);
  if (std::shared_ptr<IAttachment> ptr = std::dynamic_pointer_cast<IAttachment>(std::move(image)))
  {
    ptr->SetFramesCount(m_framesCount);
    m_attachments[binding] = std::move(ptr);
    m_attachmentsChanged = true;
  }
}

void Framebuffer::ClearImageAttachments() noexcept
{
  m_attachments.clear();
  m_attachmentsChanged = true;
}

void Framebuffer::SetFramesCount(uint32_t framesCount)
{
  for (auto && attachment : m_attachments)
    attachment->SetFramesCount(framesCount);
}

} // namespace RHI::vulkan
