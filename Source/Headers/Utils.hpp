#pragma once

namespace RHI::utils
{
template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
inline constexpr T bit(int i)
{
  return static_cast<T>(1) << i;
}
} // namespace RHI::utils
