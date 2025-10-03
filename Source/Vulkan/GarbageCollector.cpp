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
static const std::unordered_map<std::type_index, void *> kDestroyFuncs =
  {{typeid(VkSemaphore), vkDestroySemaphore},
   {typeid(VkFence), vkDestroyFence},
   {typeid(VkPipeline), vkDestroyPipeline},
   {typeid(VkPipelineLayout), vkDestroyPipelineLayout},
   {typeid(VkPipelineCache), vkDestroyPipelineCache},
   {typeid(VkBuffer), nullptr}, // because VkBuffer must be free with MemoryAllocator
   {typeid(VkBufferView), vkDestroyBufferView},
   {typeid(VkImage), nullptr}, // because VkImage must be free with MemoryAllocator
   {typeid(VkImageView), vkDestroyImageView},
   {typeid(VkDescriptorPool), vkDestroyDescriptorPool},
   {typeid(VkDescriptorSetLayout), vkDestroyDescriptorSetLayout},
   {typeid(VkSurfaceKHR), vkDestroySurfaceKHR},
   {typeid(VkRenderPass), vkDestroyRenderPass},
   {typeid(VkCommandPool), vkDestroyCommandPool},
   {typeid(VkFramebuffer), vkDestroyFramebuffer},
   {typeid(VkSampler), vkDestroySampler},
   {typeid(VkCommandPool), vkDestroyCommandPool}

};
}

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
        throw std::runtime_error(std::format(
          "Failed to destroy object {} of type {}, because kDestroyFunc doesn't contain destroy function pointer",
          data.object, data.objectType.name()));

      auto * func = reinterpret_cast<decltype(vkDestroySemaphore) *>(it->second);
      func(device, reinterpret_cast<VkSemaphore>(data.object),
           reinterpret_cast<const VkAllocationCallbacks *>(data.allocator));
    },
    [](memory::MemoryBlock & block)
    {
      //block will be destroyed later
    }};

  vkDeviceWaitIdle(GetContext().GetGpuConnection().GetDevice());
  std::lock_guard lk{m_mutex};
  while (!m_queue.empty())
  {
    DestroyData data = std::move(m_queue.front());
    m_queue.pop();
    std::visit(visitor, data);
  }
}

} // namespace RHI::vulkan::details
