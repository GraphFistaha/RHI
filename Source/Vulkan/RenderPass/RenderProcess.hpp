#pragma once
#include <atomic>
#include <functional>
#include <tuple>
#include <variant>
#include <vector>

#include <CommandsExecution/CommandBuffer.hpp>
#include <Memory/ResourceUser.hpp>
#include <Private/OwnedBy.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.h>

namespace RHI::vulkan
{
struct Context;
struct Pipeline;
struct Framebuffer;
struct SubpassGraph;
} // namespace RHI::vulkan

namespace RHI::vulkan
{

/// @brief class to accumulate commands to call them every frame
struct RenderProcess final : public RHI::IRenderPass,
                             public IResourceUser,
                             public OwnedBy<Context>
{
  explicit RenderProcess(Context & ctx, FramebufferPtr framebuffer, PipelinePtr initialPipeline);
  virtual ~RenderProcess() override;
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
  /// @brief return true if process uses vkCmdPipelineBarrier.
  /// used to detect if RenderPass should have self-dependency
  bool RequireSynchronization() const;
  virtual void CollectResources(std::vector<ResourcePtr> & resources) const override;
  virtual void SynchroniseResources(SynchronizationFilter filter,
                                    details::CommandBuffer & commands) const override;

public: // internal public API
  void RecordCommands(details::CommandBuffer & commands);
  void Invalidate();

private:
  std::shared_ptr<Framebuffer> m_framebuffer;
  std::vector<std::shared_ptr<Pipeline>> m_pipelines;
  std::unique_ptr<SubpassGraph> m_renderGraph;
  /// what happends to attachments when renderPass has began
  VkAttachmentLoadOp m_loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  /// what happends to attachments when renderPass has end
  VkAttachmentStoreOp m_storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  //using DrawCommand = std::function<void(details::CommandBuffer &)>;
  //using CommandsVector = std::vector<DrawCommand>;
  //std::vector<CommandsVector> m_commands;
  //std::vector<ResourceUsageInfo> m_resourceSyncInfos;

private:
  void CollectAttachmentsUsageInfo(std::span<VkImageUsageFlags> usage) const;
};

} // namespace RHI::vulkan
