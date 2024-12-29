#pragma once
#include <atomic>
#include <mutex>

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "../CommandsExecution/CommandBuffer.hpp"
#include "Pipeline.hpp"

namespace RHI::vulkan
{
struct Context;
}

namespace RHI::vulkan
{
struct RenderPass;
struct Pipeline;

struct Subpass : public ISubpass
{
  explicit Subpass(const Context & ctx, const RenderPass & ownerPass, uint32_t subpassIndex,
                   uint32_t familyIndex);
  virtual ~Subpass() override;

public: // ISubpass Interface
  virtual void BeginPass() override;
  virtual void EndPass() override;
  virtual IPipeline & GetConfiguration() & noexcept override;
  virtual void SetEnabled(bool enabled) noexcept override;
  virtual bool IsEnabled() const noexcept override;
  virtual bool ShouldBeInvalidated() const noexcept override;

public: // Commands
  /// @brief draw vertices command (analog glDrawArrays)
  void DrawVertices(std::uint32_t vertexCount, std::uint32_t instanceCount,
                    std::uint32_t firstVertex = 0, std::uint32_t firstInstance = 0) override;

  /// @brief draw vertices with indieces (analog glDrawElements)
  void DrawIndexedVertices(std::uint32_t indexCount, std::uint32_t instanceCount,
                           std::uint32_t firstIndex = 0, int32_t vertexOffset = 0,
                           std::uint32_t firstInstance = 0) override;

  /// @brief Set viewport command
  void SetViewport(float width, float height) override;

  /// @brief Set scissor command
  void SetScissor(int32_t x, int32_t y, std::uint32_t width, std::uint32_t height) override;

  /// @brief binds buffer as input attribute data
  void BindVertexBuffer(std::uint32_t binding, const IBufferGPU & buffer,
                        std::uint32_t offset = 0) override;

  /// @brief binds buffer as index buffer
  void BindIndexBuffer(const IBufferGPU & buffer, IndexType type,
                       std::uint32_t offset = 0) override;

//public: // IInvalidable Interface
  //virtual void Invalidate() override;

public:
  const details::CommandBuffer & GetCommandBuffer() const & noexcept { return m_executableBuffer; }
  void LockWriting(bool lock) const noexcept;
  void SetDirtyCacheCommands() noexcept;

private:
  const Context & m_context;
  const RenderPass & m_ownerPass;
  VkRenderPass m_cachedRenderPass = VK_NULL_HANDLE;
  details::CommandBuffer m_executableBuffer;
  details::CommandBuffer m_writingBuffer;
  Pipeline m_pipeline;
  mutable std::mutex m_write_lock;
  std::atomic_bool m_enabled = true;
  std::atomic_bool m_shouldBeInvalidated = true;
};

} // namespace RHI::vulkan
