#include <algorithm>

#include <Images.hpp>

namespace RHI
{

RHI_API uint32_t CalcMaxMipLevels(TextureExtent extent, uint32_t minLength /* = 1*/)
{
  uint32_t mipCounts = 1;
  uint32_t maxLength = std::max({extent[0], extent[1], extent[2]});
  while (maxLength > minLength)
  {
    mipCounts++;
    extent[0] /= 2;
    extent[1] /= 2;
    extent[2] /= 2;
    maxLength = std::max({extent[0], extent[1], extent[2]});
  }
  return mipCounts;
}

} // namespace RHI
