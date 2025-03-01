#pragma once
#include <OwnedBy.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "Image.hpp"

namespace RHI::vulkan
{
struct Context;
}

namespace RHI::vulkan
{

struct SwapchainImage : /* public IImageGPU,*/
                        public OwnedBy<Context>
{
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);
  SwapchainImage(Context & ctx);

protected:
  static constexpr uint32_t InvalidImageIndex = -1;
  std::vector<Image> m_images;
  uint32_t m_activeImage = 0;
};

} // namespace RHI::vulkan
