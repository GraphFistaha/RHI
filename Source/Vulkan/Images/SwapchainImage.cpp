#include "SwapchainImage.hpp"

namespace RHI::vulkan
{
SwapchainImage::SwapchainImage(Context & ctx)
  : OwnedBy<Context>(ctx)
{
}
} // namespace RHI::vulkan
