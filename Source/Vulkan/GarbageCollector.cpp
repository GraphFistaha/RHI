#include "GarbageCollector.hpp"

#include <format>
#include <unordered_map>

#include <Memory/MemoryBlock.hpp>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>
#include <VulkanContext.hpp>

namespace
{
template<class... Ts>
struct overloads : Ts...
{
  using Ts::operator()...;
};
} // namespace

namespace RHI::vulkan::details
{
class DestroyFuncWrapper final
{
  std::function<void(VkDevice, RHI::InternalObjectHandle, const VkAllocationCallbacks *)> m_func;

public:
  template<typename VkObjectT>
  DestroyFuncWrapper(void (*destroyFunc)(VkDevice, VkObjectT, const VkAllocationCallbacks *))
  {
    if (destroyFunc)
    {
      m_func = [destroyFunc](VkDevice dev, RHI::InternalObjectHandle obj,
                             const VkAllocationCallbacks * callbacks)
      {
        destroyFunc(dev, reinterpret_cast<VkObjectT>(obj), callbacks);
      };
    }
  }

  void operator()(VkDevice dev, RHI::InternalObjectHandle obj,
                  const VkAllocationCallbacks * callbacks) const
  {
    if (m_func)
      m_func(dev, obj, callbacks);
  }
};

static const std::unordered_map<std::type_index, DestroyFuncWrapper> kDestroyFuncs = {
  {typeid(VkSemaphore), DestroyFuncWrapper(vkDestroySemaphore)},
  {typeid(VkFence), DestroyFuncWrapper(vkDestroyFence)},
  {typeid(VkPipeline), DestroyFuncWrapper(vkDestroyPipeline)},
  {typeid(VkPipelineLayout), DestroyFuncWrapper(vkDestroyPipelineLayout)},
  {typeid(VkPipelineCache), DestroyFuncWrapper(vkDestroyPipelineCache)},
  //{typeid(VkBuffer), DestroyFuncWrapper(nullptr)}, // because VkBuffer must be free with MemoryAllocator
  {typeid(VkBufferView), DestroyFuncWrapper(vkDestroyBufferView)},
  //{typeid(VkImage), DestroyFuncWrapper(nullptr)}, // because VkImage must be free with MemoryAllocator
  {typeid(VkImageView), DestroyFuncWrapper(vkDestroyImageView)},
  {typeid(VkDescriptorPool), DestroyFuncWrapper(vkDestroyDescriptorPool)},
  {typeid(VkDescriptorSetLayout), DestroyFuncWrapper(vkDestroyDescriptorSetLayout)},
  //{typeid(VkSurfaceKHR), DestroyFuncWrapper(vkDestroySurfaceKHR)}, //destroyed with vkb::destroy_surface
  {typeid(VkRenderPass), DestroyFuncWrapper(vkDestroyRenderPass)},
  {typeid(VkCommandPool), DestroyFuncWrapper(vkDestroyCommandPool)},
  {typeid(VkFramebuffer), DestroyFuncWrapper(vkDestroyFramebuffer)},
  {typeid(VkSampler), DestroyFuncWrapper(vkDestroySampler)}};
} // namespace RHI::vulkan::details

namespace RHI::vulkan::details
{
VkObjectsGarbageCollector::VkObjectsGarbageCollector(Context & ctx)
  : OwnedBy<Context>(ctx)
{
}

VkObjectsGarbageCollector::~VkObjectsGarbageCollector()
{
  ClearObjects();
}

void VkObjectsGarbageCollector::ClearObjects()
{
  auto && visitor = overloads{
    [device = GetContext().GetGpuConnection().GetDevice()](VkObjectDestroyData & data)
    {
      auto it = kDestroyFuncs.find(data.objectType);
      if (it == kDestroyFuncs.end())
      {
        throw std::runtime_error(std::format(
          "Failed to destroy object {} of type {}, because kDestroyFunc doesn't contain destroy function pointer",
          data.object, data.objectType.name()));
      }

      auto && destroyFunc = it->second;
      destroyFunc(device, data.object,
                  reinterpret_cast<const VkAllocationCallbacks *>(data.allocator));
    },
    [](memory::MemoryBlock & block)
    {
      //block will be destroyed later
    }};

  std::lock_guard lk{m_mutex};
  while (!m_queue.empty())
  {
    DestroyableObject data = std::move(m_queue.front());
    m_queue.pop();
    std::visit(visitor, data);
  }
}

} // namespace RHI::vulkan::details
