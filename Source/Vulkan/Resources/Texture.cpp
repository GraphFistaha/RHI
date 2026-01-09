#include "Texture.hpp"

#include <ImageUtils/ImageUtils.hpp>
#include <Utils/CastHelper.hpp>
#include <VulkanContext.hpp>

namespace RHI::vulkan
{
static constexpr uint32_t g_TextureUsageFlags =
  VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

Texture::Texture(Context & ctx, const TextureDescription & args)
  : OwnedBy<Context>(ctx)
  , m_description(args)
  , m_memBlock(GetContext().GetBuffersAllocator().AllocImage(args, g_TextureUsageFlags,
                                                             VK_SAMPLE_COUNT_1_BIT))
  , m_layout(m_memBlock.GetImage())
{
  m_view =
    utils::CreateImageView(GetContext().GetGpuConnection().GetDevice(), m_memBlock.GetImage(),
                           GetInternalFormat(),
                           utils::CastInterfaceEnum2Vulkan<VkImageViewType>(m_description.type),
                           VK_IMAGE_ASPECT_COLOR_BIT);
}

Texture::~Texture()
{
  GetContext().GetGarbageCollector().PushVkObjectToDestroy(std::move(m_view), nullptr);
  GetContext().GetGarbageCollector().PushVkObjectToDestroy(std::move(m_memBlock), nullptr);
}

std::future<UploadResult> Texture::UploadImage(const UploadImageArgs& args)
{
  return GetContext().GetTransferer().UploadImage(*this, args);
}

std::future<DownloadResult> Texture::DownloadImage(const DownloadImageArgs& args)
{
  return GetContext().GetTransferer().DownloadImage(*this, args);
}

TextureDescription Texture::GetDescription() const noexcept
{
  return m_description;
}

size_t Texture::Size() const
{
  return m_memBlock.Size();
}

void Texture::BlitTo(ITexture * texture)
{
  if (auto * ptr = dynamic_cast<IInternalTexture *>(texture))
    GetContext().GetTransferer().BlitImageToImage(*ptr, *this, RHI::TextureRegion{});
}

VkImageView Texture::GetImageView() const noexcept
{
  return m_view;
}

void Texture::TransferLayout(details::CommandBuffer & commandBuffer, VkImageLayout layout)
{
  m_layout.TransferLayout(commandBuffer, layout);
}

VkImageLayout Texture::GetLayout() const noexcept
{
  return m_layout.GetLayout();
}

VkImage Texture::GetHandle() const noexcept
{
  return m_memBlock.GetImage();
}

VkFormat Texture::GetInternalFormat() const noexcept
{
  return utils::CastInterfaceEnum2Vulkan<VkFormat>(m_description.format);
}

VkExtent3D Texture::GetInternalExtent() const noexcept
{
  return {m_description.extent[0], m_description.extent[1], m_description.extent[2]};
}

} // namespace RHI::vulkan
