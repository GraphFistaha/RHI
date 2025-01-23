#pragma once
#include <mutex>
#include <queue>
#include <typeindex>
#include <variant>

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "Memory/MemoryBlock.hpp"

namespace RHI::vulkan
{
struct Context;
}

namespace RHI::vulkan::details
{
struct VkObjectsGarbageCollector final
{
  explicit VkObjectsGarbageCollector(const Context & ctx);
  ~VkObjectsGarbageCollector();

  struct VkObjectDestroyData
  {
    std::type_index objectType;
    RHI::InternalObjectHandle object;
    RHI::InternalObjectHandle allocator;
  };

  using DestroyData = std::variant<VkObjectDestroyData, memory::MemoryBlock>;

  template<typename ObjT>
  void PushVkObjectToDestroy(ObjT && object, InternalObjectHandle allocator) const noexcept
  {
    if (!object)
      return;
    std::lock_guard lk{m_mutex};
    m_queue.push(VkObjectDestroyData{typeid(ObjT), object, allocator});
  }

  template<>
  void PushVkObjectToDestroy<memory::MemoryBlock>(memory::MemoryBlock && block,
                                                  InternalObjectHandle allocator) const noexcept
  {
    if (!block)
      return;
    std::lock_guard lk{m_mutex};
    m_queue.push(DestroyData{std::move(block)});
  }

  void ClearObjects();

private:
  const Context & m_context;
  mutable std::mutex m_mutex;
  //TODO: make queue lock-free
  mutable std::queue<DestroyData> m_queue;
};

} // namespace RHI::vulkan::details
