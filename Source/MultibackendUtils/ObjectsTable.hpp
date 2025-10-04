#pragma once
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

namespace RHI::utils
{

template<typename InterfaceT>
struct ObjectsTable final
{
  ObjectsTable() = default;
  ~ObjectsTable() = default;

  template<typename ObjectT, typename... Args>
    requires(std::is_base_of_v<InterfaceT, ObjectT>)
  ObjectT * Emplace(Args &&... args);
  void Destroy(InterfaceT * object);
  InterfaceT && Take(InterfaceT * object);

private:
  struct FreedObject;
  using StoredData = std::variant<FreedObject, InterfaceT *>;
  struct FreedObject
  {
    StoredData * next = nullptr;
  };
  std::vector<StoredData> m_data;
  StoredData * m_firstFreedObject = nullptr;
};

} // namespace RHI::utils
