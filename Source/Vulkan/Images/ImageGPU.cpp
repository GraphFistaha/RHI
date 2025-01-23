#include "ImageGPU.hpp"

#include <vk_mem_alloc.h>

#include "../Memory/BuffersAllocator.hpp"
#include "../Utils/CastHelper.hpp"
#include "../VulkanContext.hpp"


namespace RHI::vulkan
{

ImageGPU::ImageGPU(const Context & ctx, Transferer * transferer,
                   const ImageDescription & description)
  : ImageBase(ctx, transferer, description)
{
  VmaAllocationCreateFlags allocation_flags = 0;
  allocation_flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
  m_memBlock =
    ctx.GetBuffersAllocator().AllocImage(description, allocation_flags, VMA_MEMORY_USAGE_AUTO);
  m_image = m_memBlock.GetImage();
}

ImageGPU::~ImageGPU()
{
  m_context.GetGarbageCollector().PushVkObjectToDestroy(std::move(m_memBlock), nullptr);
}

ImageGPU::ImageGPU(ImageGPU && rhs) noexcept
  : ImageBase(std::move(rhs))
{
  std::swap(m_memBlock, rhs.m_memBlock);
}

ImageGPU & ImageGPU::operator=(ImageGPU && rhs) noexcept
{
  if (this != &rhs)
  {
    ImageBase::operator=(std::move(rhs));
    std::swap(m_memBlock, rhs.m_memBlock);
  }
  return *this;
}

} // namespace RHI::vulkan
