#pragma once
#include <cstdint>
#include <limits>

namespace RHI::utils
{
struct char24_t
{
  static constexpr size_t NBITS = std::numeric_limits<unsigned char>::digits;
  static_assert(NBITS == 8, "byte must be an octet");

  constexpr char24_t() = default;
  constexpr char24_t(char32_t value)
  {
    // assert value within range of char24_t
    lsb = value & 0xff;
    value >>= NBITS;
    midb = value & 0xff;
    msb = value >> NBITS;
  }

  constexpr operator char32_t() const noexcept
  {
    return (msb << (NBITS * 2)) + (midb << NBITS) + lsb;
  }

  uint8_t msb = 0;
  uint8_t midb = 0;
  uint8_t lsb = 0;
};

using uint24_t = char24_t;

} // namespace RHI::utils



namespace RHI::utils
{

//https://stackoverflow.com/questions/31171682/type-trait-for-copying-cv-reference-qualifiers
template<typename T, typename U>
struct copy_cv_reference final
{
private:
  using R = std::remove_reference_t<T>;
  using U1 = std::conditional_t<std::is_const<R>::value, std::add_const_t<U>, U>;
  using U2 = std::conditional_t<std::is_volatile<R>::value, std::add_volatile_t<U1>, U1>;
  using U3 =
    std::conditional_t<std::is_lvalue_reference<T>::value, std::add_lvalue_reference_t<U2>, U2>;
  using U4 =
    std::conditional_t<std::is_rvalue_reference<T>::value, std::add_rvalue_reference_t<U3>, U3>;

public:
  using type = U4;
};

template<typename T, typename U>
using copy_cv_reference_t = typename copy_cv_reference<T, U>::type;
} // namespace RHI::utils
