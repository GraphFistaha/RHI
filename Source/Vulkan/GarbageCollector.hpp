#pragma once
#include <mutex>
#include <queue>
#include <typeindex>
#include <variant>

#include <Memory/MemoryBlock.hpp>
#include <Private/OwnedBy.hpp>
#include <RHI.hpp>

namespace RHI::vulkan
{
struct Context;
}

namespace RHI::vulkan::details
{
class VkObjectsGarbageCollector final : public OwnedBy<Context>
{
  struct VkObjectDestroyData
  {
    std::type_index objectType;
    RHI::InternalObjectHandle object;
    RHI::InternalObjectHandle allocator;
  };

  using DestroyableObject = std::variant<VkObjectDestroyData, memory::MemoryBlock>;

public:
  explicit VkObjectsGarbageCollector(Context & ctx);
  virtual ~VkObjectsGarbageCollector() override;
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);

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
    m_queue.push(DestroyableObject{std::move(block)});
  }

  void ClearObjects();

private:
  mutable std::mutex m_mutex;
  //TODO: make queue lock-free
  mutable std::queue<DestroyableObject> m_queue;
};

} // namespace RHI::vulkan::details
