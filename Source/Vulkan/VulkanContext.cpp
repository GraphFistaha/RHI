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
  , m_allocator(*this)
  , m_gc(*this)
{
  // alloc null texture
  RHI::TextureDescription args{};
  {
    args.extent = {1, 1, 1};
    args.format = RHI::ImageFormat::RGBA8;
    args.mipLevels = 1;
    args.type = RHI::ImageType::Image2D;
  }
  m_nullTexture = CreateTexture(args);
}


IAttachment * Context::CreateSurfacedAttachment(const SurfaceConfig & surfaceTraits,
                                                RenderBuffering buffering)
{
  Surface surface(m_device, surfaceTraits);
  return m_attachments.Emplace<SurfacedAttachment>(*this, std::move(surface), buffering);
}

IFramebuffer * Context::CreateFramebuffer()
{
  return m_framebuffers.Emplace<Framebuffer>(*this);
}

void Context::DeleteFramebuffer(IFramebuffer * fbo)
{
  m_framebuffers.Destroy(fbo);
}

IBufferGPU * Context::CreateBuffer(size_t size, BufferGPUUsage usage, bool allowHostAccess)
{
  return m_buffers.Emplace<BufferGPU>(*this, size, usage, allowHostAccess);
}

void Context::DeleteBuffer(IBufferGPU * buffer)
{
  m_buffers.Destroy(buffer);
}

ITexture * Context::CreateTexture(const TextureDescription & args)
{
  return m_textures.Emplace<Texture>(*this, args);
}

void Context::DeleteTexture(ITexture * texture)
{
  m_textures.Destroy(texture);
}

IAttachment * Context::CreateAttachment(RHI::ImageFormat format, const RHI::TextureExtent & extent,
                                        RenderBuffering buffering, RHI::SamplesCount samplesCount)
{
  RHI::TextureDescription args{};
  {
    args.format = format;
    args.extent = extent;
    args.mipLevels = 1;
    args.type = RHI::ImageType::Image2D;
  }
  return m_attachments.Emplace<GenericAttachment>(*this, args, buffering, samplesCount);
}

void Context::DeleteAttachment(IAttachment * attachment)
{
  m_attachments.Destroy(attachment);
}

void Context::ClearResources()
{
  WaitForIdle();
  m_gc.ClearObjects();
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
  return m_allocator;
}

const details::VkObjectsGarbageCollector & Context::GetGarbageCollector() const & noexcept
{
  return m_gc;
}

RHI::ITexture * Context::GetNullTexture() const noexcept
{
  return m_nullTexture;
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
