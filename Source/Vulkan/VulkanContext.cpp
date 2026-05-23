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
#include <RHI.hpp>
#include <Surface.hpp>
#include <TransferPass/Transferer.hpp>
#include <Utils/CastHelper.hpp>

// --------------------- Static functions ------------------------------


namespace RHI::vulkan
{
Context::Context(const GpuTraits & gpuTraits, LoggingFunc logFunc)
  : m_logFunc(logFunc)
  , m_device(*this, gpuTraits)
  , m_allocator(*this)
  , m_gc(*this)
  , m_graphicSubmitter(*this, QueueType::Graphics, 2, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT)
  , m_transferSubmitter(*this, QueueType::Transfer, 2, VK_PIPELINE_STAGE_TRANSFER_BIT)
  , m_computeSubmitter(*this, QueueType::Compute, 2, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT)
  , m_graphicTransferer(*this, m_device.GetQueue(QueueType::Graphics).first, 3)
  , m_transferTransferer(*this, m_device.GetQueue(QueueType::Transfer).first, 3)
  , m_computeTransferer(*this, m_device.GetQueue(QueueType::Compute).first, 3)
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

IAwaitable * Context::TransferPass()
{
  AsyncTask * result = nullptr;
  m_transferSubmitter.WaitForSubmitCompleted();
  m_transferTransferer.FlushCommands(m_transferSubmitter.GetWritingBuffer());
  result = m_transferSubmitter.Submit(false, {});
  return result;
}

IAwaitable * Context::RenderPass(IFramebuffer * framebuffer)
{
  auto * fbo = dynamic_cast<Framebuffer *>(framebuffer);
  if (!fbo)
    return nullptr;
  AsyncTask * result = nullptr;
  m_graphicSubmitter.WaitForSubmitCompleted(); //TODO: think about removing this line
  if (RenderTarget * renderTarget = fbo->BeginFrame())
  {
    m_graphicTransferer.FlushCommands(m_graphicSubmitter.GetWritingBuffer());
    fbo->Draw(m_graphicSubmitter.GetWritingBuffer());
    result = m_graphicSubmitter.Submit(false /*waitPrevSubmitOnGPU*/,
                                       renderTarget->GetImageAvailableForRenderSemaphores());
    fbo->EndFrame(result->GetSemaphore());
  }

  return result;
}

void Context::LogImpl(LogMessageStatus status, const std::string & message) const noexcept
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

Transferer & Context::GetTransferer(QueueType queue) &
{
  switch (queue)
  {
    case QueueType::Graphics:
      return m_graphicTransferer;
    case QueueType::Transfer:
      return m_transferTransferer;
    case QueueType::Compute:
      return m_computeTransferer;
    default:
      throw std::runtime_error("Incorrect queue type");
  }
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
RHI_API std::unique_ptr<IContext> CreateContext(const GpuTraits & gpuTraits,
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
