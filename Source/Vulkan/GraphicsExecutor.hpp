#pragma once
#include <atomic>
#include <list>

#include "VulkanContext.hpp"

namespace RHI::vulkan
{

struct GraphicsExecutor : public IGraphicsCommandsExecutor
{
  explicit GraphicsExecutor(const Context & context, GraphicsExecutor * owner);
  ~GraphicsExecutor() override;

  /// @brief Enable writing mode (only for thread local buffers). Also binds framebuffer and pipeline
  virtual void BeginExecute(const IFramebuffer & framebuffer) override;
  /// @brief disables writing mode
  virtual void EndExecute() override;
  /// @brief Cancel executing
  virtual void CancelExecute() override;
  /// @brief Creates sub executor which can be executed in separate thread
  virtual IGraphicsCommandsExecutor & CreateThreadLocalExecutor() & override;

  virtual void UsePipeline(const IPipeline & pipeline) override;
  /// @brief draw vertices command (analog glDrawArrays)
  virtual void DrawVertices(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex = 0,
                            uint32_t firstInstance = 0) override;
  /// @brief draw vertices with indieces (analog glDrawElements)
  virtual void DrawIndexedVertices(uint32_t indexCount, uint32_t instanceCount,
                                   uint32_t firstIndex = 0, int32_t vertexOffset = 0,
                                   uint32_t firstInstance = 0) override;
  /// @brief Set viewport command
  virtual void SetViewport(float width, float height) override;
  /// @brief Set scissor command
  virtual void SetScissor(int32_t x, int32_t y, uint32_t width, uint32_t height) override;
  /// @brief binds buffer as input attribute data
  virtual void BindVertexBuffer(uint32_t binding, const IBufferGPU & buffer,
                                uint32_t offset = 0) override;
  /// @brief binds buffer as index buffer
  virtual void BindIndexBuffer(const IBufferGPU & buffer, IndexType type,
                               uint32_t offset = 0) override;

private:
  const Context & m_context;
  GraphicsExecutor * m_owner = nullptr;
  vk::CommandPool m_pool; // owns only if m_owner == nullptr
  RHI::CommandBufferType m_buffersType;

  vk::CommandBuffer m_writing_commands_buffer = VK_NULL_HANDLE;
  vk::CommandBuffer m_commited_commands_buffer = VK_NULL_HANDLE;
  vk::CommandBuffer m_child_commands_buffer = VK_NULL_HANDLE;

  std::list<GraphicsExecutor> m_child_executors;
  std::atomic_bool m_child_commands_dirty = false;

private:
  void ExecuteCommandBuffer();
  void SetChildCommandsBufferDirty() { m_child_commands_dirty = true; }
};

} // namespace RHI::vulkan
