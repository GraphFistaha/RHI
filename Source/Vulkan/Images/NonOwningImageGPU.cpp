#include "NonOwningImageGPU.hpp"

#include "../Resources/BufferGPU.hpp"
#include "../Resources/Transferer.hpp"
#include "ImageFormatsConversation.hpp"
#include "ImageTraits.hpp"

namespace RHI::vulkan
{

NonOwningImageGPU::NonOwningImageGPU(const Context & ctx, Transferer & transferer, VkImage image,
                                     VkExtent3D extent, VkFormat internalFormat)
  : m_context(ctx)
  , m_transferer(&transferer)
  , m_image(image)
  , m_extent(extent)
  , m_internalFormat(internalFormat)
{
}

NonOwningImageGPU::NonOwningImageGPU(NonOwningImageGPU && rhs) noexcept
  : m_context(rhs.m_context)
{
  std::swap(m_transferer, rhs.m_transferer);
  std::swap(m_image, rhs.m_image);
  std::swap(m_extent, rhs.m_extent);
  std::swap(m_internalFormat, rhs.m_internalFormat);
  std::swap(m_layout, rhs.m_layout);
}

NonOwningImageGPU & NonOwningImageGPU::operator=(NonOwningImageGPU && rhs) noexcept
{
  if (this != &rhs && &m_context == &rhs.m_context)
  {
    std::swap(m_transferer, rhs.m_transferer);
    std::swap(m_image, rhs.m_image);
    std::swap(m_extent, rhs.m_extent);
    std::swap(m_internalFormat, rhs.m_internalFormat);
    std::swap(m_layout, rhs.m_layout);
  }
  return *this;
}

void NonOwningImageGPU::UploadImage(const uint8_t * data, const CopyImageArguments & args)
{
  if (!m_transferer)
    throw std::runtime_error("Image has no transferer. Async upload is impossible");
  BufferGPU stagingBuffer(m_context, nullptr,
                          utils::GetSizeOfImage(args.dst.extent, m_internalFormat),
                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
  if (auto && mapped_ptr = stagingBuffer.Map())
  {
    utils::CopyImageFromHost(data, args.src.extent, args.src, args.hostFormat,
                             reinterpret_cast<uint8_t *>(mapped_ptr.get()), args.dst.extent,
                             args.dst, m_internalFormat);
    mapped_ptr.reset();
    stagingBuffer.Flush();
    m_transferer->UploadImage(this, std::move(stagingBuffer));
  }
  else
  {
    throw std::runtime_error("Failed to fill staging buffer");
  }
}

size_t NonOwningImageGPU::Size() const noexcept
{
  return utils::GetSizeOfImage(m_extent, m_internalFormat);
}

ImageExtent NonOwningImageGPU::GetExtent() const noexcept
{
  return {m_extent.width, m_extent.height, m_extent.depth};
}

ImageType NonOwningImageGPU::GetImageType() const noexcept
{
  // TOOD: implement
  throw std::runtime_error("unimplemented");
}

ImageFormat NonOwningImageGPU::GetImageFormat() const noexcept
{
  // TOOD: implement
  throw std::runtime_error("unimplemented");
}

VkImage NonOwningImageGPU::GetHandle() const noexcept
{
  return m_image;
}


} // namespace RHI::vulkan
