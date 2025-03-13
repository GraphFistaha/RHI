#include "Image.hpp"

#include "../CommandsExecution/CommandBuffer.hpp"
#include "../Resources/BufferGPU.hpp"
#include "../Resources/Transferer.hpp"
#include "../Utils/CastHelper.hpp"
#include "../VulkanContext.hpp"
#include "BufferedTexture.hpp"
#include "InternalImageTraits.hpp"

namespace
{
constexpr VkAccessFlags LayoutTransfer_MakeAccessFlag(VkImageLayout layout) noexcept
{
  switch (layout)
  {
    case VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL:
      return VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
      return VK_ACCESS_SHADER_READ_BIT;

    case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL:
    case VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL:
      return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

    case VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL:
    case VK_IMAGE_LAYOUT_ATTACHMENT_FEEDBACK_LOOP_OPTIMAL_EXT:
      return VK_ACCESS_SHADER_WRITE_BIT;

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
      return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
      return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: // пишем в картинку
      return VK_ACCESS_TRANSFER_WRITE_BIT;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: // читаем из картинки
      return VK_ACCESS_TRANSFER_READ_BIT;

    case VK_IMAGE_LAYOUT_GENERAL:
    case VK_IMAGE_LAYOUT_PREINITIALIZED:
    case VK_IMAGE_LAYOUT_UNDEFINED:
    default:
      return 0;
  }
}

constexpr VkPipelineStageFlags LayoutTransfer_MakePipelineStage(VkImageLayout layout) noexcept
{
  switch (layout)
  {
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: // shader sampler
      return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: // transfering
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
      return VK_PIPELINE_STAGE_TRANSFER_BIT;

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: // color attachment
      return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL: // depth/stencil attachment
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
    case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
    case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
      return VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

    case VK_IMAGE_LAYOUT_PREINITIALIZED:
    case VK_IMAGE_LAYOUT_UNDEFINED:
      return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    default:
      assert(false);
      return VK_PIPELINE_STAGE_NONE;
  }
}

} // namespace

namespace RHI::vulkan
{
Image::Image(Context & ctx, BufferedTexture & texture)
  : OwnedBy<Context>(ctx)
  , OwnedBy<BufferedTexture>(texture)
{
}

Image::~Image()
{
}


Image::Image(Image && rhs) noexcept
  : OwnedBy<Context>(std::move(rhs))
  , OwnedBy<BufferedTexture>(std::move(rhs))
{
  std::swap(m_image, rhs.m_image);
  std::swap(m_memBlock, rhs.m_memBlock);
}

Image & Image::operator=(Image && rhs) noexcept
{
  if (this != &rhs)
  {
    OwnedBy<Context>::operator=(std::move(rhs));
    OwnedBy<BufferedTexture>::operator=(std::move(rhs));
    std::swap(m_image, rhs.m_image);
    std::swap(m_memBlock, rhs.m_memBlock);
  }
  return *this;
}

std::future<UploadResult> Image::UploadImage(const uint8_t * data, const CopyImageArguments & args)
{
  return GetContext().GetTransferer().UploadImage(*this, data, args);
}

std::future<DownloadResult> Image::DownloadImage(HostImageFormat format, const ImageRegion & region)
{
  return GetContext().GetTransferer().DownloadImage(*this, format, region);
}

void Image::TransferLayout(details::CommandBuffer & commandBuffer, VkImageLayout newLayout) noexcept
{
  if (newLayout == VK_IMAGE_LAYOUT_UNDEFINED || newLayout == VK_IMAGE_LAYOUT_PREINITIALIZED ||
      newLayout == m_layout)
    return;
  VkImageMemoryBarrier barrier{};
  {
    auto && description = GetTextureOwner().GetDescription();
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = m_layout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = description.mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = LayoutTransfer_MakeAccessFlag(m_layout);
    barrier.dstAccessMask = LayoutTransfer_MakeAccessFlag(newLayout);
  }

  VkPipelineStageFlags sourceStage = LayoutTransfer_MakePipelineStage(m_layout);
  VkPipelineStageFlags destinationStage = LayoutTransfer_MakePipelineStage(newLayout);

  std::lock_guard lk{m_layoutLock};
  commandBuffer.PushCommand(vkCmdPipelineBarrier, sourceStage, destinationStage, 0, 0, nullptr, 0,
                            nullptr, 1, &barrier);
  m_layout = newLayout;
}

void Image::SetImageLayoutBeforeRenderPass(VkImageLayout newLayout) noexcept
{
  m_layoutLock.lock();
  if (newLayout != VK_IMAGE_LAYOUT_UNDEFINED && newLayout != VK_IMAGE_LAYOUT_PREINITIALIZED)
    m_layout = newLayout;
}

void Image::SetImageLayoutAfterRenderPass(VkImageLayout newLayout) noexcept
{
  m_layout = newLayout;
  m_layoutLock.unlock();
}

VkImageType Image::GetVulkanImageType() const noexcept
{
  auto && description = GetTextureOwner().GetDescription();
  return utils::CastInterfaceEnum2Vulkan<VkImageType>(description.type);
}

VkExtent3D Image::GetVulkanExtent() const noexcept
{
  auto && description = GetTextureOwner().GetDescription();
  return VkExtent3D{description.extent[0], description.extent[1], description.extent[2]};
}

VkFormat Image::GetVulkanFormat() const noexcept
{
  auto && description = GetTextureOwner().GetDescription();
  return utils::CastInterfaceEnum2Vulkan<VkFormat>(description.format);
}

VkSampleCountFlagBits Image::GetVulkanSamplesCount() const noexcept
{
  auto && description = GetTextureOwner().GetDescription();
  return utils::CastInterfaceEnum2Vulkan<VkSampleCountFlagBits>(description.samples);
}

size_t Image::Size() const
{
  return RHI::utils::GetSizeOfImage(GetVulkanExtent(), GetVulkanFormat());
}

} // namespace RHI::vulkan
