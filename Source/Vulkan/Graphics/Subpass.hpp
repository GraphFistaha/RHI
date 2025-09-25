#pragma once
#include <atomic>
#include <mutex>

#include <OwnedBy.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "../CommandsExecution/CommandBuffer.hpp"
#include "SubpassConfiguration.hpp"
#include "SubpassLayout.hpp"

namespace RHI::vulkan
{
struct Context;
struct RenderPass;
} // namespace RHI::vulkan

namespace RHI::vulkan
{
struct Subpass : public ISubpass,
                 public OwnedBy<Context>,
                 public OwnedBy<RenderPass>
{
  using UsedAttachments = std::unordered_map<uint32_t, RHI::ShaderAttachmentSlot>;
  explicit Subpass(Context & ctx, RenderPass & ownerPass, uint32_t subpassIndex,
                   uint32_t familyIndex);
  virtual ~Subpass() override;
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);
  MAKE_ALIAS_FOR_GET_OWNER(RenderPass, GetRenderPass);

public: // ISubpass Interface
  virtual void BeginPass() override;
  virtual void EndPass() override;
  virtual ISubpassConfiguration & GetConfiguration() & noexcept override;
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

  void PushConstant(const void * data, size_t size) override;

public:
  const details::CommandBuffer & GetCommandBufferForExecution() const & noexcept
  {
    return m_executableBuffer;
  }
  details::CommandBuffer & GetCommandBufferForWriting() & noexcept { return m_writingBuffer; }
  const SubpassLayout & GetLayout() const & noexcept;
  SubpassLayout & GetLayout() & noexcept;

  void SetInvalid();
  void Invalidate();

  bool ShouldSwapCommandBuffers() const noexcept;
  void SwapCommandBuffers() noexcept;
  void SetDirtyCacheCommands() noexcept;
  void TransitLayoutForUsedImages(details::CommandBuffer & commandBuffer);

private:
  SubpassConfiguration m_pipeline;
  std::atomic_bool m_invalidPipeline = true;
  std::atomic_bool m_enabled = true;
  VkRenderPass m_cachedRenderPass = VK_NULL_HANDLE;

  details::CommandBuffer m_executableBuffer;
  details::CommandBuffer m_writingBuffer;
  mutable std::mutex m_write_lock;
  std::atomic_bool m_dirtyCommands = true; ///< flag to refill m_writingBuffer
  std::atomic_bool m_shouldSwapBuffer = false;

  SubpassLayout m_layout{VK_PIPELINE_BIND_POINT_GRAPHICS};
};
} // namespace RHI::vulkan
