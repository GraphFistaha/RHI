#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <span>

namespace RHI
{
template<typename T, size_t MaxSize>
class SmallVector final
{
public:
  SmallVector() = default;
  ~SmallVector() = default;
  SmallVector(const SmallVector & rhs)
    : m_size(rhs.m_size)
    , m_data(rhs.m_data)
  {
  }
  SmallVector & operator=(const SmallVector & rhs)
  {
    if (this != &rhs)
    {
      m_data = rhs.m_data;
      m_size = rhs.m_size;
    }
    return *this;
  }
  SmallVector(SmallVector && rhs) noexcept
    : m_size(std::move(rhs.m_size))
    , m_data(std::move(rhs.m_data))
  {
  }
  SmallVector & operator=(SmallVector && rhs) noexcept
  {
    if (this != &rhs)
    {
      std::swap(m_data, rhs.m_data);
      std::swap(m_size, rhs.m_size);
    }
    return *this;
  }


  operator std::span<const T>() const noexcept
  {
    return std::span<const T>(m_data.begin(), m_size);
  }
  operator std::span<T>() noexcept { return std::span<T>(m_data.begin(), m_size); }

  size_t size() const noexcept { return m_size; }
  void clear() noexcept { m_size = 0; }

  void push_back(const T & el) { m_data[m_size++] = el; }
  void pop_back() noexcept { m_size--; }

  bool operator==(const SmallVector & rhs) const noexcept
  {
    return std::equal(m_data.begin(), m_data.begin() + m_size, rhs.m_data.begin());
  }

  const T & operator[](size_t i) const noexcept { return m_data[i]; }
  T & operator[](size_t i) noexcept { return m_data[i]; }

private:
  size_t m_size = 0;
  std::array<T, MaxSize> m_data;
};
} // namespace RHI
