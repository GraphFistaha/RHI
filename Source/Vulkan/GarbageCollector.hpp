#pragma once
#include <mutex>
#include <queue>
#include <typeindex>

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

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

  template<typename ObjT>
  void PushVkObjectToDestroy(ObjT object, InternalObjectHandle allocator) const noexcept
  {
    if (!object)
      return;
    std::lock_guard lk{m_mutex};
    m_queue.push(VkObjectDestroyData{typeid(ObjT), object, allocator});
  }

  void ClearObjects();

private:
  const Context & m_context;
  mutable std::mutex m_mutex;
  //TODO: make queue lock-free
  mutable std::queue<VkObjectDestroyData> m_queue;
};

} // namespace RHI::vulkan::details
