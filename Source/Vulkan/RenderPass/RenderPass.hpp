#pragma once

#include <condition_variable>
#include <list>
#include <shared_mutex>

#include <CommandsExecution/CommandBuffer.hpp>
#include <Memory/ResourceUser.hpp>
#include <Private/OwnedBy.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan
{
struct Context;
struct RenderTarget;
struct Framebuffer;
struct Pipeline;
struct PipelineProcess;
} // namespace RHI::vulkan

namespace RHI::vulkan
{

struct RenderPass : public IInvalidable,
                    public OwnedBy<Context>,
                    public OwnedBy<Framebuffer>
{
  explicit RenderPass(Context & ctx, Framebuffer & framebuffer);
  virtual ~RenderPass() override;
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);
  MAKE_ALIAS_FOR_GET_OWNER(Framebuffer, GetFramebuffer);

public:
  void SetSubpass(uint32_t index, PipelinePtr pipeline, PipelineProcessPtr process);
  void ClearSubpasses();

  void SetAttachments(uint32_t buffersCount,
                      const std::vector<VkAttachmentDescription> & attachments) noexcept;
  const VkAttachmentDescription & GetAttachmentDescription(uint32_t idx) const & noexcept;

public: // IInvalidable Interface
  virtual void Invalidate() override;
  virtual void SetInvalid() override;

public: // internal public API
  VkRenderPass GetHandle() const noexcept { return m_renderPass; }

  void RecordCommands(details::CommandBuffer & commands, RenderTarget & renderTarget);
  void CollectAttachmentsUsageInfo(std::span<VkImageUsageFlags> usage) const;

public: // IResourceUser
  void CollectResources(std::vector<ResourcePtr> & resources) const;
  void SynchroniseResources(details::CommandBuffer & commands) const;

private:
  using Subpass = std::pair<std::shared_ptr<Pipeline>, std::shared_ptr<PipelineProcess>>;
  std::vector<VkAttachmentDescription> m_cachedAttachments;

  /// There is a lot of thread-readers, so it's must be synchronized access
  VkRenderPass m_renderPass = VK_NULL_HANDLE;
  bool m_invalidRenderPass = false;
  details::CommandBuffer m_writeBuffer;
  details::CommandBuffer m_execBuffer;

  /// Flag to notify that subpasses can begin pass
  std::atomic_bool m_isReadyForRendering = false;

  uint32_t m_buffersCount = 0;
  std::vector<Subpass> m_subpasses;
  bool m_dirtyCommands = false;

  std::unique_ptr<Pipeline> m_dummyPipeline; ///< fummy pipeline is used when no subpasses was added
};


} // namespace RHI::vulkan
