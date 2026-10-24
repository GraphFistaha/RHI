#pragma once
#include <memory>
#include <vector>

#include <CommandsExecution/CommandBuffer.hpp>
#include <Memory/ResourceUser.hpp>
#include <Pipeline/Pipeline.hpp>
#include <RenderPass/Framebuffer.hpp>


namespace RHI::vulkan
{
struct RenderPass;
struct SubpassGraph;
enum class SubpassIndex;
} // namespace RHI::vulkan


namespace RHI::vulkan
{

enum class SynchronizationFilter : uint8_t
{
  None = 0,
  BufferOnly = 1,
  ImageOnly = 2,
  BufferAndImage = 3,
  AttachmentOnly = 4,
  BufferAndAttachment = 5,
  ImageAndAttachment = 6,
  All = 7,
};

inline bool FilterSatisfied(SynchronizationFilter f1, SynchronizationFilter f2) noexcept
{
  return (static_cast<uint8_t>(f1) & static_cast<uint8_t>(f2)) != 0;
}

struct SubpassInfo final : public OwnedBy<RenderPass>
{
  /// @brief Creates a subpass owned by its render pass.
  explicit SubpassInfo(RenderPass & renderPass, std::shared_ptr<Pipeline> pipeline);
  virtual ~SubpassInfo() override = default;
  MAKE_ALIAS_FOR_GET_OWNER(RenderPass, GetRenderPass);

public: // API to fill a SubpassGraph
  void AddBarrier(IInternalBuffer & buffer);
  void AddBarrier(IInternalTexture & texture);
  void AddBarrier(IInternalAttachment & attachment);
  void AddDependenciesTo(SubpassGraph & graph, SubpassIndex index) const;
  void CollectBarriers();

public: // draw Commands
  void DrawVertices(std::uint32_t vertexCount, std::uint32_t instanceCount,
                    std::uint32_t firstVertex = 0, std::uint32_t firstInstance = 0);
  void DrawIndexedVertices(std::uint32_t indexCount, std::uint32_t instanceCount,
                           std::uint32_t firstIndex = 0, int32_t vertexOffset = 0,
                           std::uint32_t firstInstance = 0);
  void SetViewport(float width, float height);
  void SetScissor(int32_t x, int32_t y, std::uint32_t width, std::uint32_t height);
  void BindVertexBuffer(std::uint32_t binding, IBufferGPU * buffer, std::uint32_t offset = 0);
  void BindIndexBuffer(IBufferGPU * buffer, IndexType type, std::uint32_t offset = 0);
  void PushConstant(const void * data, size_t size);

public:
  bool IsInlineCommands() const;
  void RecordCommands(details::CommandBuffer & commands);
  /// @brief Resets synchronization for resources and indexed framebuffer attachments.
  void ResetSynchronization() const;
  void SynchroniseResources(SynchronizationFilter filter, details::CommandBuffer & commands) const;

public: // Pipeline getters
  const Pipeline & GetPipeline() const & noexcept;
  Pipeline & GetPipeline() & noexcept;


private:
  std::shared_ptr<Pipeline> m_pipeline;
  std::vector<Barrier> m_barriers; ///< info how each resource is used
  using DrawCommand = std::function<void(details::CommandBuffer &)>;
  using CpuCommandBuffer = std::vector<DrawCommand>;
  CpuCommandBuffer m_commands;
  std::unique_ptr<details::CommandBuffer> m_commandsCache;
};

} // namespace RHI::vulkan
