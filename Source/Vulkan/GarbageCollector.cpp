#include "GarbageCollector.hpp"

#include <format>
#include <unordered_map>

#include <vk_mem_alloc.h>

#include "VulkanContext.hpp"

namespace RHI::vulkan::details
{
static const std::unordered_map<std::type_index, void *> kDestroyFuncs =
  {{typeid(VkSemaphore), vkDestroySemaphore},
   {typeid(VkFence), vkDestroyFence},
   {typeid(VkPipeline), vkDestroyPipeline},
   {typeid(VkPipelineLayout), vkDestroyPipelineLayout},
   {typeid(VkPipelineCache), vkDestroyPipelineCache},
   {typeid(VkBuffer), nullptr}, // because VkBuffer must be free with BuffersAllocator
   {typeid(VkBufferView), vkDestroyBufferView},
   {typeid(VkImage), nullptr}, // because VkImage must be free with BuffersAllocator
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
VkObjectsGarbageCollector::VkObjectsGarbageCollector(const Context & ctx)
  : m_context(ctx)
{
}

VkObjectsGarbageCollector::~VkObjectsGarbageCollector()
{
  ClearObjects();
}

void VkObjectsGarbageCollector::ClearObjects()
{
  vkDeviceWaitIdle(m_context.GetDevice());
  std::lock_guard lk{m_mutex};
  while (!m_queue.empty())
  {
    VkObjectDestroyData data = m_queue.front();
    m_queue.pop();
    if (data.objectType == typeid(VkImage))
    {
      m_context.GetBuffersAllocator().FreeImage(reinterpret_cast<VkImage>(data.object),
                                                data.allocator);
    }
    else if (data.objectType == typeid(VkBuffer))
    {
      m_context.GetBuffersAllocator().FreeBuffer(reinterpret_cast<VkBuffer>(data.object),
                                                 data.allocator);
    }
    else
    {
      auto it = kDestroyFuncs.find(data.objectType);
      if (it == kDestroyFuncs.end())
        throw std::runtime_error(std::format(
          "Failed to destroy object {} of type {}, because kDestroyFunc doesn't contain destroy function pointer",
          data.object, data.objectType.name()));

      auto * func = reinterpret_cast<decltype(vkDestroySemaphore) *>(it->second);
      func(m_context.GetDevice(), reinterpret_cast<VkSemaphore>(data.object),
           reinterpret_cast<const VkAllocationCallbacks *>(data.allocator));
    }
  }
}

} // namespace RHI::vulkan::details
