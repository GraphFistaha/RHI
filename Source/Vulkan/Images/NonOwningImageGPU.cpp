#include "NonOwningImageGPU.hpp"

namespace RHI::vulkan
{

NonOwningImageGPU::NonOwningImageGPU(const Context & ctx, Transferer * transferer,
                                     const ImageDescription & description, VkImage image,
                                     VkImageLayout layout)
  : ImageBase(ctx, transferer, description)
{
  m_image = image;
  m_layout = layout;
}


} // namespace RHI::vulkan
