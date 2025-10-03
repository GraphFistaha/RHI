#include "VulkanContext.hpp"

#include <format>

#include <Attachments/GenericAttachment.hpp>
#include <Attachments/SurfacedAttachment.hpp>
#include <CommandsExecution/CommandBuffer.hpp>
#include <RenderPass/Framebuffer.hpp>
#include <RenderPass/RenderPass.hpp>
#include <RenderPass/RenderTarget.hpp>
#include <RenderPass/SubpassConfiguration.hpp>
#include <Resources/BufferGPU.hpp>
#include <Resources/Texture.hpp>
#include <Resources/Transferer.hpp>
#include <RHI.hpp>
#include <Surface.hpp>
#include <Utils/CastHelper.hpp>

// --------------------- Static functions ------------------------------


namespace RHI::vulkan
{
Context::Context(const GpuTraits & gpuTraits, LoggingFunc logFunc)
  : m_logFunc(logFunc)
  , m_device(*this, gpuTraits)
{
  m_allocator = std::make_unique<memory::MemoryAllocator>(*this);
  m_gc = std::make_unique<details::VkObjectsGarbageCollector>(*this);

  // alloc null texture
  RHI::TextureDescription args{};
  {
    args.extent = {1, 1, 1};
    args.format = RHI::ImageFormat::RGBA8;
    args.mipLevels = 1;
    args.type = RHI::ImageType::Image2D;
  }
  AllocImage(args);
}

Context::~Context()
{
}

IAttachment * Context::CreateSurfacedAttachment(const SurfaceConfig & surfaceTraits,
                                                RenderBuffering buffering)
{
  Surface surface(m_device, surfaceTraits);
  auto surfaceTexture = std::make_unique<SurfacedAttachment>(*this, std::move(surface), buffering);
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

ITexture * Context::AllocImage(const TextureDescription & args)
{
  auto && texture = std::make_unique<Texture>(*this, args);
  auto && result = m_textures.emplace_back(std::move(texture));
  return result.get();
}

IAttachment * Context::AllocAttachment(RHI::ImageFormat format, const RHI::TextureExtent & extent,
                                       RenderBuffering buffering, RHI::SamplesCount samplesCount)
{
  RHI::TextureDescription args{};
  {
    args.format = format;
    args.extent = extent;
    args.mipLevels = 1;
    args.type = RHI::ImageType::Image2D;
  }
  auto && attachment = std::make_unique<GenericAttachment>(*this, args, buffering, samplesCount);
  return m_attachments.emplace_back(std::move(attachment)).get();
}

void Context::ClearResources()
{
  m_gc->ClearObjects();
}

void Context::TransferPass()
{
  for (auto && [thread_id, transferer] : m_transferers)
    transferer.DoTransfer();
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
  vkDeviceWaitIdle(GetGpuConnection().GetDevice());
}

const Device & Context::GetGpuConnection() const & noexcept
{
  return m_device;
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

memory::MemoryAllocator & Context::GetBuffersAllocator() & noexcept
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
