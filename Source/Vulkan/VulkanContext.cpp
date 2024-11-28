#include "VulkanContext.hpp"

#include <format>

#include <RHI.hpp>
#include <VkBootstrap.h>

#include "CommandsExecution/CommandBuffer.hpp"
#include "Graphics/Pipeline.hpp"
#include "Graphics/RenderPass.hpp"
#include "Graphics/RenderTarget.hpp"
#include "Graphics/Swapchain.hpp"
#include "Resources/BufferGPU.hpp"
#include "Resources/ImageGPU.hpp"
#include "Resources/Transferer.hpp"

// --------------------- Static functions ------------------------------

namespace
{

static VKAPI_ATTR VkBool32 VKAPI_CALL
VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                    VkDebugUtilsMessageTypeFlagsEXT messageType,
                    const VkDebugUtilsMessengerCallbackDataEXT * pCallbackData, void * pUserData)
{
  using SeverityBitFlag = vk::DebugUtilsMessageSeverityFlagBitsEXT;
  using SeverityFlags = VkDebugUtilsMessageSeverityFlagBitsEXT;
  using TypeBitFlag = vk::DebugUtilsMessageTypeFlagBitsEXT;
  using TypeFlags = VkDebugUtilsMessageTypeFlagsEXT;
  RHI::LoggingFunc logFunc = reinterpret_cast<RHI::LoggingFunc>(pUserData);

  if (logFunc)
  {
    if (messageSeverity == static_cast<SeverityFlags>(SeverityBitFlag::eError) ||
        messageType == static_cast<TypeFlags>(TypeBitFlag::eValidation))
      logFunc(RHI::LogMessageStatus::LOG_ERROR, pCallbackData->pMessage);
    else if (messageSeverity == static_cast<SeverityFlags>(SeverityBitFlag::eWarning) ||
             messageType == static_cast<TypeFlags>(TypeBitFlag::ePerformance))
      logFunc(RHI::LogMessageStatus::LOG_WARNING, pCallbackData->pMessage);
    else
      logFunc(RHI::LogMessageStatus::LOG_INFO, pCallbackData->pMessage);
  }
  return VK_FALSE;
}


vkb::Instance CreateInstance(const char * appName, uint32_t apiVersion, RHI::LoggingFunc logFunc)
{
  vkb::Instance result;
  vkb::InstanceBuilder builder;
  auto inst_ret = builder
                    .set_app_name(appName)
#ifdef ENABLE_VALIDATION_LAYERS
                    .request_validation_layers()
#endif
                    .set_debug_callback(VulkanDebugCallback)
                    .set_debug_callback_user_data_pointer(reinterpret_cast<void *>(logFunc))
                    .set_minimum_instance_version(apiVersion)
                    .build();
  if (!inst_ret || !inst_ret.has_value())
  {
    std::string msg =
      std::format("Failed to create Vulkan instance - {}", inst_ret.error().message());
    throw std::runtime_error(msg);
  }
  else
  {
    result = inst_ret.value();
  }
  return result;
}

vk::SurfaceKHR CreateSurface(vkb::Instance inst, const RHI::SurfaceConfig & config)
{
  VkSurfaceKHR surface = VK_NULL_HANDLE;
  VkResult result;
#ifdef _WIN32
  VkWin32SurfaceCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
  createInfo.hwnd = reinterpret_cast<HWND>(config.hWnd);
  createInfo.hinstance = reinterpret_cast<HINSTANCE>(config.hInstance);
  result = vkCreateWin32SurfaceKHR(inst, &createInfo, nullptr, &surface);
#else
  VkXlibSurfaceCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
  createInfo.window = reinterpret_cast<Window>(config.hWnd);
  createInfo.dpy = reinterpret_cast<Display *>(config.hInstance);
  result = vkCreateXlibSurfaceKHR(inst, &createInfo, nullptr, &surface);
#endif
  if (result != VK_SUCCESS)
    throw std::runtime_error("failed to create window surface!");
  return vk::SurfaceKHR(surface);
}

vkb::PhysicalDevice SelectPhysicalDevice(vkb::Instance inst,
                                         VkSurfaceKHR surface /* = VK_NULL_HANDLE*/,
                                         const std::pair<uint32_t, uint32_t> & apiVersion)
{
  vkb::PhysicalDeviceSelector selector{inst};
  auto phys_ret = selector.set_surface(surface)
                    .require_present(surface != VK_NULL_HANDLE)
                    .set_minimum_version(apiVersion.first, apiVersion.second)
                    .select();

  if (!phys_ret)
  {
    std::string msg =
      std::format("Failed to select Vulkan Physical Device - {}", phys_ret.error().message());
    throw std::runtime_error(msg);
  }
  return phys_ret.value();
}

} // namespace


namespace RHI::vulkan
{
constexpr uint32_t VulkanAPIVersion = VK_API_VERSION_1_3;
constexpr std::pair<uint32_t, uint32_t> VulkanAPIVersionPair = {1, 3};

struct Context::Impl final
{
  explicit Impl(const char * appName, const SurfaceConfig & config, LoggingFunc logFunc)
  {
    m_instance = CreateInstance("AppName", VulkanAPIVersion, logFunc);
    m_surface = CreateSurface(m_instance, config);
    m_gpu = SelectPhysicalDevice(m_instance, m_surface, VulkanAPIVersionPair);
    vkb::DeviceBuilder device_builder{m_gpu};
    auto dev_ret = device_builder.build();
    if (!dev_ret)
    {
      std::string msg =
        std::format("Failed to create Vulkan device - {}", dev_ret.error().message());
      throw std::runtime_error(msg);
    }
    m_device = dev_ret.value();
    m_dispatchTable = m_device.make_table();
  }

  ~Impl()
  {
    vkb::destroy_surface(m_instance, m_surface);
    vkb::destroy_device(m_device);
    vkb::destroy_instance(m_instance);
  }

  VkDevice GetDevice() const noexcept { return m_device; }
  VkInstance GetInstance() const noexcept { return m_instance; }
  VkPhysicalDevice GetGPU() const noexcept { return m_gpu; }
  VkSurfaceKHR GetSurface() const noexcept { return m_surface; }

  std::pair<uint32_t, VkQueue> GetQueue(vkb::QueueType type) const
  {
    auto queue_ret = m_device.get_queue(type);
    auto familly_index = m_device.get_queue_index(type);
    if (!queue_ret)
      throw std::runtime_error("failed to request queue");
    return std::make_pair(familly_index.value(), queue_ret.value());
  }

private:
  vkb::Instance m_instance;
  vkb::PhysicalDevice m_gpu;
  vkb::Device m_device;
  vkb::DispatchTable m_dispatchTable;
  vk::SurfaceKHR m_surface;
};


Context::Context(const SurfaceConfig & config, LoggingFunc logFunc)
  : m_logFunc(logFunc)
{
  m_impl = std::make_unique<Impl>("appName", config, m_logFunc);
  m_allocator = std::make_unique<details::BuffersAllocator>(m_impl->GetInstance(), m_impl->GetGPU(),
                                                            m_impl->GetDevice(),
                                                            GetVulkanVersion());
  m_surfaceSwapchain = std::make_unique<Swapchain>(*this, m_impl->GetSurface());
  m_transferer = std::make_unique<Transferer>(*this);
}

Context::~Context()
{
  WaitForIdle();
}

ISwapchain * Context::GetSurfaceSwapchain()
{
  return m_surfaceSwapchain.get();
}

ITransferer * Context::GetTransferer()
{
  return m_transferer.get();
}

std::unique_ptr<IBufferGPU> Context::AllocBuffer(size_t size, BufferGPUUsage usage,
                                                 bool mapped /* = false*/) const
{
  auto vkUsage = utils::CastInterfaceEnum2Vulkan<VkBufferUsageFlags>(usage);
  return std::make_unique<BufferGPU>(size, vkUsage, *m_allocator, m_transferer.get(), mapped);
}

std::unique_ptr<IImageGPU> Context::AllocImage(const ImageCreateArguments & args) const
{
  return std::make_unique<ImageGPU>(*this, *m_allocator, *m_transferer, args);
}

void Context::WaitForIdle() const noexcept
{
  vkDeviceWaitIdle(GetDevice());
}

const vk::Instance Context::GetInstance() const
{
  return m_impl->GetInstance();
}

const vk::Device Context::GetDevice() const
{
  return m_impl->GetDevice();
}

const vk::PhysicalDevice Context::GetGPU() const
{
  return m_impl->GetGPU();
}

std::pair<uint32_t, VkQueue> Context::GetQueue(QueueType type) const
{
  return m_impl->GetQueue(static_cast<vkb::QueueType>(type));
}

uint32_t Context::GetVulkanVersion() const
{
  return VulkanAPIVersion;
}

void Context::Log(LogMessageStatus status, const std::string & message) const noexcept
{
#ifdef NDEBUG
  if (m_logFunc && status != LogMessageStatus::LOG_DEBUG)
    m_logFunc(status, message);
#else
  if (m_logFunc)
    m_logFunc(status, message);
#endif
}

const details::BuffersAllocator & Context::GetBuffersAllocator() const & noexcept
{
  return *m_allocator;
}

} // namespace RHI::vulkan

namespace RHI
{
std::unique_ptr<IContext> CreateContext(const SurfaceConfig & config,
                                        LoggingFunc loggingFunc /* = nullptr*/)
{
  return std::make_unique<vulkan::Context>(config, loggingFunc);
}
} // namespace RHI


namespace RHI::vulkan::utils
{

vk::Semaphore CreateVkSemaphore(vk::Device device)
{
  VkSemaphore result = VK_NULL_HANDLE;
  VkSemaphoreCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  // Don't use createSemaphore in dispatchTable because it's broken
  if (vkCreateSemaphore(device, &info, nullptr, &result) != VK_SUCCESS)
    throw std::runtime_error("failed to create semaphore");
  return vk::Semaphore(result);
}

vk::Fence CreateFence(vk::Device device, bool locked)
{
  VkFenceCreateInfo info{};
  VkFence result = VK_NULL_HANDLE;
  info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  if (locked)
    info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  // Don't use createFence in dispatchTable because it's broken
  if (vkCreateFence(device, &info, nullptr, &result) != VK_SUCCESS)
    throw std::runtime_error("failed to create fence");
  return vk::Fence(result);
}
} // namespace RHI::vulkan::utils
