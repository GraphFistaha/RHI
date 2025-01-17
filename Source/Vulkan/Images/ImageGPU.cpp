#include "ImageGPU.hpp"

#include <vk_mem_alloc.h>

#include "../Utils/CastHelper.hpp"
#include "../VulkanContext.hpp"


namespace RHI::vulkan
{

ImageGPU::ImageGPU(const Context & ctx, Transferer & transferer, const ImageCreateArguments & args)
  : BufferBase(ctx, &transferer)
  , ImageBase()
{
  VmaAllocationCreateFlags allocation_flags = 0;
  allocation_flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

  std::tie(m_image, m_memBlock, m_allocInfo) =
    ctx.GetBuffersAllocator().AllocImage(args, allocation_flags, VMA_MEMORY_USAGE_AUTO);
  m_args = args;
  m_flags = allocation_flags;
  m_internalFormat = utils::CastInterfaceEnum2Vulkan<VkFormat>(args.format);
}

ImageGPU::~ImageGPU()
{
  m_context.GetGarbageCollector().PushVkObjectToDestroy(m_image, m_memBlock);
}

ImageGPU::ImageGPU(ImageGPU && rhs) noexcept
  : BufferBase(std::move(rhs))
{
  std::swap(m_args, rhs.m_args);
}

ImageGPU & ImageGPU::operator=(ImageGPU && rhs) noexcept
{
  if (this != &rhs)
  {
    BufferBase::operator=(std::move(rhs));
    std::swap(m_image, rhs.m_image);
    std::swap(m_layout, rhs.m_layout);
    std::swap(m_args, rhs.m_args);
    std::swap(m_internalFormat, rhs.m_internalFormat);
  }
  return *this;
}

void ImageGPU::UploadImage(const uint8_t * data, const CopyImageArguments & args)
{
}

void ImageGPU::SetImageLayout(details::CommandBuffer & commandBuffer,
                              VkImageLayout newLayout) noexcept
{
  ImageBase::SetImageLayout(commandBuffer, newLayout);
}

ImageType ImageGPU::GetImageType() const noexcept
{
  return m_args.type;
}

ImageExtent ImageGPU::GetExtent() const noexcept
{
  return m_args.extent;
}

ImageFormat ImageGPU::GetImageFormat() const noexcept
{
  return m_args.format;
}

} // namespace RHI::vulkan
