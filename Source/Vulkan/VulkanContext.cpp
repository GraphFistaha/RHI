#include "VulkanContext.hpp"

#include <format>

#include <RHI.hpp>
#include <VkBootstrap.h>

#include "Attachments/GenericAttachment.hpp"
#include "Attachments/SurfacedAttachment.hpp"
#include "CommandsExecution/CommandBuffer.hpp"
#include "Graphics/Framebuffer.hpp"
#include "Graphics/RenderPass.hpp"
#include "Graphics/RenderTarget.hpp"
#include "Graphics/SubpassConfiguration.hpp"
#include "Resources/BufferGPU.hpp"
#include "Resources/Texture.hpp"
#include "Resources/Transferer.hpp"
#include "Utils/CastHelper.hpp"

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

VkSurfaceKHR CreateSurface(vkb::Instance inst, const RHI::SurfaceConfig & config)
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
  return VkSurfaceKHR(surface);
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
  explicit Impl(const char * appName, const GpuTraits & gpuTraits, LoggingFunc logFunc)
    : m_logFunc(logFunc)
  {
    m_instance = CreateInstance("AppName", VulkanAPIVersion, logFunc);
    m_logFunc(LogMessageStatus::LOG_DEBUG, "VkInstance has been created successfully");
    m_gpu = SelectPhysicalDevice(m_instance, gpuTraits, VulkanAPIVersionPair);
    m_logFunc(LogMessageStatus::LOG_DEBUG,
              "VkPhysicalDevice has been selected successfully - " + m_gpu.name);
    vkb::DeviceBuilder device_builder{m_gpu};
    auto dev_ret = device_builder.build();
    m_logFunc(LogMessageStatus::LOG_DEBUG, "VkDevice has been created successfully");
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
    for (auto && surface : m_surfaces)
      vkb::destroy_surface(m_instance, surface);
    m_surfaces.clear();
    vkb::destroy_device(m_device);
    vkb::destroy_instance(m_instance);
  }

  VkDevice GetDevice() const noexcept { return m_device; }
  VkInstance GetInstance() const noexcept { return m_instance; }
  VkPhysicalDevice GetGPU() const noexcept { return m_gpu; }
  const VkPhysicalDeviceProperties & GetGpuProperties() const & noexcept
  {
    return m_gpu.properties;
  }

  std::pair<uint32_t, VkQueue> GetQueue(vkb::QueueType type) const
  {
    auto queue_ret = m_device.get_queue(type);
    // if device doesn't have compute or transfer queue, try graphics for that
    if ((type == vkb::QueueType::compute || type == vkb::QueueType::transfer) && !queue_ret)
    {
      type = vkb::QueueType::graphics;
      queue_ret = m_device.get_queue(type);
    }
    if (!queue_ret)
      return {-1, VK_NULL_HANDLE};
    auto familly_index = m_device.get_queue_index(type);
    return std::make_pair(familly_index.value(), queue_ret.value());
  }

  VkSurfaceKHR AllocSurface(const SurfaceConfig & surfaceTraits)
  {
    VkSurfaceKHR result = CreateSurface(m_instance, surfaceTraits);
    m_logFunc(LogMessageStatus::LOG_DEBUG, "VkSurfaceKHR has been created successfully");
    m_surfaces.push_back(result);
    return result;
  }

private:
  LoggingFunc m_logFunc;
  vkb::Instance m_instance;
  vkb::PhysicalDevice m_gpu;
  vkb::Device m_device;
  vkb::DispatchTable m_dispatchTable;
  std::vector<VkSurfaceKHR> m_surfaces;
};


Context::Context(const GpuTraits & gpuTraits, LoggingFunc logFunc)
  : m_logFunc(logFunc)
{
  m_impl = std::make_unique<Impl>("appName", gpuTraits, m_logFunc);
  m_allocator = std::make_unique<memory::MemoryAllocator>(m_impl->GetInstance(), m_impl->GetGPU(),
                                                          m_impl->GetDevice(), GetVulkanVersion());
  m_gc = std::make_unique<details::VkObjectsGarbageCollector>(*this);

  // alloc null texture
  RHI::ImageCreateArguments args{};
  {
    args.extent = {1, 1, 1};
    args.format = RHI::ImageFormat::RGBA8;
    args.mipLevels = 1;
    args.type = RHI::ImageType::Image2D;
    args.shared = false;
  }
  AllocImage(args);
}

Context::~Context()
{
}

IAttachment * Context::CreateSurfacedAttachment(const SurfaceConfig & surfaceTraits,
                                                RenderBuffering buffering)
{
  auto surface = m_impl->AllocSurface(surfaceTraits);
  auto surfaceTexture = std::make_unique<SurfacedAttachment>(*this, surface, buffering);
  auto && result = m_attachments.emplace_back(std::move(surfaceTexture));
  return result.get();
}

IFramebuffer * Context::CreateFramebuffer()
{
  auto & result = m_framebuffers.emplace_back(*this);
  return &result;
}

IBufferGPU * Context::AllocBuffer(size_t size, BufferGPUUsage usage, bool allowHostAccess)
{
  auto && result = m_buffers.emplace_back(*this, size, usage, allowHostAccess);
  return &result;
}

ITexture * Context::AllocImage(const ImageCreateArguments & args)
{
  auto && texture = std::make_unique<Texture>(*this, args);
  auto && result = m_textures.emplace_back(std::move(texture));
  return result.get();
}

IAttachment * Context::AllocAttachment(const ImageCreateArguments & args, RenderBuffering buffering,
                                       RHI::SamplesCount samplesCount)
{
  auto && attachment = std::make_unique<GenericAttachment>(*this, args, buffering, samplesCount);
  return m_attachments.emplace_back(std::move(attachment)).get();
}

void Context::ClearResources()
{
  m_gc->ClearObjects();
}

void Context::Flush()
{
  for (auto && [thread_id, transferer] : m_transferers)
    transferer.DoTransfer();
}

VkInstance Context::GetInstance() const noexcept
{
  return m_impl->GetInstance();
}

VkDevice Context::GetDevice() const noexcept
{
  return m_impl->GetDevice();
}

VkPhysicalDevice Context::GetGPU() const noexcept
{
  return m_impl->GetGPU();
}

const VkPhysicalDeviceProperties & Context::GetGpuProperties() const & noexcept
{
  return m_impl->GetGpuProperties();
}

std::pair<uint32_t, VkQueue> Context::GetQueue(QueueType type) const
{
  return m_impl->GetQueue(static_cast<vkb::QueueType>(type));
}

uint32_t Context::GetVulkanVersion() const noexcept
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

void Context::WaitForIdle() const noexcept
{
  vkDeviceWaitIdle(GetDevice());
}

Transferer & Context::GetTransferer() & noexcept
{
  auto id = std::this_thread::get_id();
  auto it = m_transferers.find(id);
  if (it == m_transferers.end())
  {
    bool inserted = false;
    std::tie(it, inserted) = m_transferers.insert({id, Transferer(*this)});
    assert(inserted);
  }
  return it->second;
}

const memory::MemoryAllocator & Context::GetBuffersAllocator() const & noexcept
{
  return *m_allocator;
}

const details::VkObjectsGarbageCollector & Context::GetGarbageCollector() const & noexcept
{
  return *m_gc;
}

RHI::ITexture * Context::GetNullTexture() const noexcept
{
  return m_textures[0].get();
}

} // namespace RHI::vulkan

namespace RHI
{
std::unique_ptr<IContext> CreateContext(const GpuTraits & gpuTraits,
                                        LoggingFunc loggingFunc /* = nullptr*/)
{
  try
  {
    return std::make_unique<vulkan::Context>(gpuTraits, loggingFunc);
  }
  catch (const std::exception & e)
  {
    loggingFunc(LogMessageStatus::LOG_ERROR, e.what());
    return nullptr;
  }
  catch (...)
  {
    loggingFunc(LogMessageStatus::LOG_ERROR, "Unknown error happend while creating VulkanContext");
    return nullptr;
  }
}
} // namespace RHI


namespace RHI::vulkan::utils
{

VkSemaphore CreateVkSemaphore(VkDevice device)
{
  VkSemaphore result = VK_NULL_HANDLE;
  VkSemaphoreCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  // Don't use createSemaphore in dispatchTable because it's broken
  if (vkCreateSemaphore(device, &info, nullptr, &result) != VK_SUCCESS)
    throw std::runtime_error("failed to create semaphore");
  return VkSemaphore(result);
}

VkFence CreateFence(VkDevice device, bool locked)
{
  VkFenceCreateInfo info{};
  VkFence result = VK_NULL_HANDLE;
  info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  if (locked)
    info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  // Don't use createFence in dispatchTable because it's broken
  if (vkCreateFence(device, &info, nullptr, &result) != VK_SUCCESS)
    throw std::runtime_error("failed to create fence");
  return VkFence(result);
}
} // namespace RHI::vulkan::utils
