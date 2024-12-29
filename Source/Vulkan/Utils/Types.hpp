#pragma once

struct char24_t
{
  static constexpr std::size_t NBITS = std::numeric_limits<unsigned char>::digits;
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