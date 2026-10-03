#pragma once
#include <span>
#include <vector>

namespace RHI::utils
{
template<typename T>
class Table2D
{
public:
  Table2D() = default;
  explicit Table2D(size_t rows, size_t columns, const T & value = T())
    : m_rows(rows)
    , m_columns(columns)
    , m_data(rows * columns, value)
  {
  }

public:
  size_t RowsCount() const noexcept { return m_rows; }
  size_t ColumnsCount() const noexcept { return m_columns; }

public:
  std::span<const T> operator[](size_t i) const
  {
    if (i > m_rows)
      throw std::runtime_error("row is out of range");
    return std::span<const T>(m_data.begin() + i * m_columns, m_columns);
  }
  std::span<T> operator[](size_t i)
  {
    if (i > m_rows)
      throw std::runtime_error("row is out of range");
    return std::span<T>(m_data.begin() + i * m_columns, m_columns);
  }

  void Clear() noexcept
  {
    m_data.clear();
    m_rows = 0;
    m_columns = 0;
  }

private:
  size_t m_rows = 0;
  size_t m_columns = 0;
  std::vector<T> m_data;
};
} // namespace RHI::utils
