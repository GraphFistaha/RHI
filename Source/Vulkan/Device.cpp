
#include "Device.hpp"

#include <cassert>

#include <VkBootstrap.h>
#include <vulkan/vulkan.hpp>
#include <VulkanContext.hpp>

namespace
{
constexpr uint32_t VulkanAPIVersion = VK_API_VERSION_1_3;
constexpr std::pair<uint32_t, uint32_t> VulkanAPIVersionPair = {1, 3};


static VKAPI_ATTR VkBool32 VKAPI_CALL
VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                    VkDebugUtilsMessageTypeFlagsEXT messageType,
                    const VkDebugUtilsMessengerCallbackDataEXT * pCallbackData, void * pUserData)
{
  using SeverityBitFlag = vk::DebugUtilsMessageSeverityFlagBitsEXT;
  using SeverityFlags = VkDebugUtilsMessageSeverityFlagBitsEXT;
  using TypeBitFlag = vk::DebugUtilsMessageTypeFlagBitsEXT;
  using TypeFlags = VkDebugUtilsMessageTypeFlagsEXT;
  RHI::vulkan::Context * ctx = reinterpret_cast<RHI::vulkan::Context *>(pUserData);

  if (ctx)
  {
    if (messageSeverity == static_cast<SeverityFlags>(SeverityBitFlag::eError) ||
        messageType == static_cast<TypeFlags>(TypeBitFlag::eValidation))
      ctx->Log(RHI::LogMessageStatus::LOG_ERROR, pCallbackData->pMessage);
    else if (messageSeverity == static_cast<SeverityFlags>(SeverityBitFlag::eWarning) ||
             messageType == static_cast<TypeFlags>(TypeBitFlag::ePerformance))
      ctx->Log(RHI::LogMessageStatus::LOG_WARNING, pCallbackData->pMessage);
    else
      ctx->Log(RHI::LogMessageStatus::LOG_INFO, pCallbackData->pMessage);
  }
  return VK_FALSE;
}


vkb::Instance CreateInstance(const char * appName, uint32_t apiVersion, void * debugUserData)
{
  vkb::Instance result;
  vkb::InstanceBuilder builder;
  auto inst_ret = builder
                    .set_app_name(appName)
#ifdef ENABLE_VALIDATION_LAYERS
                    .request_validation_layers()
#endif
                    .set_debug_callback(VulkanDebugCallback)
                    .set_debug_callback_user_data_pointer(debugUserData)
                    .set_minimum_instance_version(apiVersion)
                    .build();
  if (!inst_ret || !inst_ret.has_value())
  {
    throw std::runtime_error("Failed to create Vulkan instance");
  }
  else
  {
    result = inst_ret.value();
  }
  return result;
}

vkb::PhysicalDevice SelectPhysicalDevice(vkb::Instance inst, const RHI::GpuTraits & gpuTraits,
                                         const std::pair<uint32_t, uint32_t> & apiVersion)
{
  vkb::PhysicalDeviceSelector selector{inst};
  VkPhysicalDeviceFeatures features{};
  if (gpuTraits.require_geometry_shaders)
    features.geometryShader = VK_TRUE;
  if (gpuTraits.name.has_value())
    selector.set_name(*gpuTraits.name);
  if (gpuTraits.require_presentation)
    selector.defer_surface_initialization();
  auto phys_ret =
    selector
      .set_required_features(features)
      //.set_minimum_version(apiVersion.first, apiVersion.second) // RenderDoc doesn't work with it
      .select();

  if (!phys_ret)
  {
    throw std::runtime_error("Failed to select Vulkan Physical Device");
  }
  return phys_ret.value();
}

struct DeviceInternal final
{
  explicit DeviceInternal(const RHI::GpuTraits & gpuTraits, RHI::vulkan::Context & ctx);
  ~DeviceInternal();

  VkInstance GetInstance() const noexcept { return instance; }
  VkDevice GetDevice() const noexcept { return device; }
  VkPhysicalDevice GetPhysicalDevice() const noexcept { return physicalDevice; }
  const VkPhysicalDeviceProperties & GetGpuProperties() const & noexcept;
  bool GetQueue(vkb::QueueType type, uint32_t & resultFamily, VkQueue & resultQueue) const noexcept;

private:
  vkb::Instance instance;
  vkb::PhysicalDevice physicalDevice;
  vkb::Device device;
  vkb::DispatchTable dispatchTable;
};

DeviceInternal::DeviceInternal(const RHI::GpuTraits & gpuTraits, RHI::vulkan::Context & ctx)
{
  constexpr uint32_t VulkanAPIVersion = VK_API_VERSION_1_3;
  constexpr std::pair<uint32_t, uint32_t> VulkanAPIVersionPair = {1, 3};
  instance = CreateInstance("appName", VulkanAPIVersion, &ctx);
  ctx.Log(RHI::LogMessageStatus::LOG_DEBUG, "VkInstance has been created successfully");
  physicalDevice = SelectPhysicalDevice(instance, gpuTraits, VulkanAPIVersionPair);
  ctx.Log(RHI::LogMessageStatus::LOG_DEBUG,
          "VkPhysicalDevice has been selected successfully - " + physicalDevice.name);
  vkb::DeviceBuilder device_builder{physicalDevice};
  auto dev_ret = device_builder.build();
  if (!dev_ret)
  {
    throw std::runtime_error("Failed to create Vulkan device");
  }
  ctx.Log(RHI::LogMessageStatus::LOG_DEBUG, "VkDevice has been created successfully");
  device = dev_ret.value();
  dispatchTable = device.make_table();
}

DeviceInternal::~DeviceInternal()
{
  vkb::destroy_device(device);
  vkb::destroy_instance(instance);
}

const VkPhysicalDeviceProperties & DeviceInternal::GetGpuProperties() const & noexcept
{
  return physicalDevice.properties;
}

bool DeviceInternal::GetQueue(vkb::QueueType type, uint32_t & resultFamily,
                              VkQueue & resultQueue) const noexcept
{
  auto queue_ret = device.get_queue(type);
  auto familly_index = device.get_queue_index(type);
  if (!queue_ret || !familly_index)
    return false;
  resultFamily = familly_index.value();
  resultQueue = queue_ret.value();
  return true;
}

} // namespace

namespace RHI::vulkan
{

Device::Device(Context & ctx, const GpuTraits & gpuTraits)
  : OwnedBy<Context>(ctx)
{
  static_assert(sizeof(DeviceInternal) <= sizeof(m_privateData));
  auto * privData = new (m_privateData.data()) DeviceInternal(gpuTraits, ctx);

  privData->GetQueue(vkb::QueueType::graphics, m_queues[QueueType::Graphics].first,
                     m_queues[QueueType::Graphics].second);

  if (!privData->GetQueue(vkb::QueueType::transfer, m_queues[QueueType::Transfer].first,
                          m_queues[QueueType::Transfer].second))
    m_queues[QueueType::Transfer] = m_queues[QueueType::Graphics];

  if (!privData->GetQueue(vkb::QueueType::compute, m_queues[QueueType::Compute].first,
                          m_queues[QueueType::Compute].second))
    m_queues[QueueType::Compute] = m_queues[QueueType::Graphics];

  if (!privData->GetQueue(vkb::QueueType::present, m_queues[QueueType::Present].first,
                          m_queues[QueueType::Present].second))
    m_queues[QueueType::Present] = m_queues[QueueType::Graphics];
}

Device::~Device()
{
  auto * privData = reinterpret_cast<const DeviceInternal *>(m_privateData.data());
  if (privData)
    privData->~DeviceInternal();
}

VkDevice Device::GetDevice() const noexcept
{
  auto * privData = reinterpret_cast<const DeviceInternal *>(m_privateData.data());
  return privData ? privData->GetDevice() : VK_NULL_HANDLE;
}

VkInstance Device::GetInstance() const noexcept
{
  auto * privData = reinterpret_cast<const DeviceInternal *>(m_privateData.data());
  return privData ? privData->GetInstance() : VK_NULL_HANDLE;
}

VkPhysicalDevice Device::GetGPU() const noexcept
{
  auto * privData = reinterpret_cast<const DeviceInternal *>(m_privateData.data());
  return privData ? privData->GetPhysicalDevice() : VK_NULL_HANDLE;
}

const VkPhysicalDeviceProperties & Device::GetGpuProperties() const & noexcept
{
  static VkPhysicalDeviceProperties s_nullProperties{};
  auto * privData = reinterpret_cast<const DeviceInternal *>(m_privateData.data());
  return privData ? privData->GetGpuProperties() : s_nullProperties;
}

std::pair<uint32_t, VkQueue> Device::GetQueue(QueueType type) const
{
  return m_queues[type];
}

uint32_t Device::GetVulkanVersion() const noexcept
{
  return VulkanAPIVersion;
}

} // namespace RHI::vulkan
