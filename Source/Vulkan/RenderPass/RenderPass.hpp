#pragma once

#include <condition_variable>
#include <list>
#include <shared_mutex>

#include <CommandsExecution/DoubleBufferedSubmitter.hpp>
#include <Memory/ResourceUser.hpp>
#include <Private/OwnedBy.hpp>
#include <RenderPass/SubpassConfiguration.hpp>
#include <RHI.hpp>
#include <Utils/RenderPassBuilder.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan
{
struct Context;
struct RenderTarget;
struct Framebuffer;
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
  SubpassConfiguration * CreateSubpass();
  void DeleteSubpass(SubpassConfiguration* subpass);

  void SetAttachments(uint32_t buffersCount,
                      const std::vector<VkAttachmentDescription> & attachments) noexcept;
  const VkAttachmentDescription & GetAttachmentDescription(uint32_t idx) const & noexcept;
  void ForEachSubpass(std::function<void(SubpassConfiguration&)> && func);

public: // IInvalidable Interface
  virtual void Invalidate() override;
  virtual void SetInvalid() override;

public:
  VkRenderPass GetHandle() const noexcept { return m_renderPass; }
  void WaitForRenderPassIsValid() const noexcept;
  void UpdateRenderPassValidFlag() noexcept;

  void RecordCommands(details::CommandBuffer & commands, RenderTarget & renderTarget);

public: // IResourceUser
  void CollectResources(std::vector<ResourcePtr> & resources) const;
  void SynchroniseResources(details::CommandBuffer & commands) const;

private:
  std::vector<VkAttachmentDescription> m_cachedAttachments;

  /// There is a lot of thread-readers, so it's must be synchronized access
  VkRenderPass m_renderPass = VK_NULL_HANDLE;
  bool m_invalidRenderPass = false;
  utils::RenderPassBuilder m_builder;

  /// Flag to notify that subpasses can begin pass
  std::atomic_bool m_isReadyForRendering = false;

  uint32_t m_buffersCount = 0;
  std::list<SubpassConfiguration> m_subpasses;
  uint32_t m_createSubpassCallsCounter = 0;
};


} // namespace RHI::vulkan
