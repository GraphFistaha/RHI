#include "NonOwningImageGPU.hpp"

namespace RHI::vulkan
{

NonOwningImageGPU::NonOwningImageGPU(Context & ctx, const ImageCreateArguments & description,
                                     VkImage image, VkImageLayout layout)
  : ImageBase(ctx, description)
{
  m_image = image;
  m_layout = layout;
}


} // namespace RHI::vulkan
