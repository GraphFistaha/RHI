#pragma once
#include <atomic>
#include <functional>
#include <tuple>
#include <variant>
#include <vector>

#include <CommandsExecution/CommandBuffer.hpp>
#include <Memory/ResourceUser.hpp>
#include <Private/OwnedBy.hpp>
#include <RenderPass/RenderTarget.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.h>


namespace RHI::vulkan
{
struct Context;
struct Pipeline;
struct Framebuffer;
struct SubpassGraph;
struct SubmitTask;
struct SubpassInfo;
} // namespace RHI::vulkan

namespace RHI::vulkan
{

/// @brief class to accumulate commands to call them every frame
struct RenderPass final : public RHI::IRenderPass,
                          public OwnedBy<Context>
{
  explicit RenderPass(Context & ctx, FramebufferPtr framebuffer, PipelinePtr initialPipeline);
  virtual ~RenderPass() override;
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);

public: // IRenderPass interface
  virtual void Barrier(IBufferGPU & buffer) override;
  virtual void Barrier(ITexture & texture) override;
  virtual void Barrier(IAttachment & attachment) override;
  virtual void NextPipeline(PipelinePtr pipeline) override;
  /// @brief marks that framebuffer attachments must be filled with clear color value in the begining of the pass
  /// put that in the beginning of the pass
  virtual void ClearFramebuffer() override;
  /// @brief marks that framebuffer attachments must be loaded from CPU memory in the begining of the pass
  ///   put that in the beginning of the pass
  virtual void LoadFramebuffer() override;
  /// @brief marks that framebuffer attachments must be stored in CPU memory in the end of the pass
  ///   Put that in the end of pass
  virtual void StoreFramebuffer() override;

public: // Commands
  /// @brief draw vertices command (analog glDrawArrays)
  virtual void DrawVertices(std::uint32_t vertexCount, std::uint32_t instanceCount,
                            std::uint32_t firstVertex = 0,
                            std::uint32_t firstInstance = 0) override;

  /// @brief draw vertices with indieces (analog glDrawElements)
  virtual void DrawIndexedVertices(std::uint32_t indexCount, std::uint32_t instanceCount,
                                   std::uint32_t firstIndex = 0, int32_t vertexOffset = 0,
                                   std::uint32_t firstInstance = 0) override;

  /// @brief Set viewport command
  virtual void SetViewport(float width, float height) override;

  /// @brief Set scissor command
  virtual void SetScissor(int32_t x, int32_t y, std::uint32_t width, std::uint32_t height) override;

  /// @brief binds buffer as input attribute data
  virtual void BindVertexBuffer(std::uint32_t binding, IBufferGPU * buffer,
                                std::uint32_t offset = 0) override;

  /// @brief binds buffer as index buffer
  virtual void BindIndexBuffer(IBufferGPU * buffer, IndexType type,
                               std::uint32_t offset = 0) override;

  virtual void PushConstant(const void * data, size_t size) override;

public: // IResourceUser
  VkRenderPass GetHandle() const noexcept;
  std::shared_ptr<Framebuffer> GetFramebuffer() const noexcept;

public: // internal public API
  void ResetResourcesSynchronization();
  void RecordCommands(details::CommandBuffer & commands, std::vector<VkSemaphore> & waitSemaphores);
  void OnSubmit(SubmitTask & submitTask);
  void Invalidate();

private:
  std::shared_ptr<Framebuffer> m_framebuffer;
  std::vector<RenderTarget> m_targets; //TODO: small_vector
  uint32_t m_activeTarget = -1;
  std::unique_ptr<SubpassGraph> m_renderGraph;
  std::vector<std::unique_ptr<SubpassInfo>> m_subpasses;
  /// what happends to attachments when renderPass has began
  VkAttachmentLoadOp m_loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  /// what happends to attachments when renderPass has end
  VkAttachmentStoreOp m_storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
};

} // namespace RHI::vulkan
