#pragma once

#include <CommandsExecution/CommandBuffer.hpp>
#include <Memory/ResourceUser.hpp>
#include <Private/OwnedBy.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.h>

namespace RHI::vulkan
{
struct Context;
struct RenderTarget;
struct Framebuffer;
struct Pipeline;
struct PipelineProcess;
struct SubpassGraph;
} // namespace RHI::vulkan

namespace RHI::vulkan
{

struct RenderPass : public IInvalidable,
                    public RHI::IRenderPass,
                    public OwnedBy<Context>
{
  explicit RenderPass(Context & ctx, Framebuffer & framebuffer);
  virtual ~RenderPass() override;
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);

public: // IInvalidable Interface
  virtual void Invalidate() override;
  virtual void SetInvalid() override;

public: // internal public API
  VkRenderPass GetHandle() const noexcept { return m_renderPass; }
  const RenderTarget * GetActiveRenderTarget() const noexcept { return m_activeRenderTarget; }

  void RecordCommands(details::CommandBuffer & commands, RenderTarget & renderTarget);
  void CollectAttachmentsUsageInfo(std::span<VkImageUsageFlags> usage) const;

public: // IResourceUser
  void CollectResources(std::vector<ResourcePtr> & resources) const;

private:
  const RenderTarget * m_activeRenderTarget = nullptr;
  /// There is a lot of thread-readers, so it's must be synchronized access
  VkRenderPass m_renderPass = VK_NULL_HANDLE;
  bool m_invalidRenderPass = false;

  using Subpass = std::pair<std::shared_ptr<Pipeline>, std::shared_ptr<PipelineProcess>>;
  std::vector<Subpass> m_subpasses;
  bool m_dirtyCommands = false;
  std::unique_ptr<Pipeline> m_dummyPipeline; ///< fummy pipeline is used when no subpasses was added

private: // subpass graph data
  std::unique_ptr<SubpassGraph> m_subpassGraph = nullptr;
};


} // namespace RHI::vulkan
