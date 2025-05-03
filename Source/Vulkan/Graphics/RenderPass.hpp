#pragma once

#include <condition_variable>
#include <list>
#include <shared_mutex>

#include <OwnedBy.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "../CommandsExecution/Submitter.hpp"
#include "../Utils/RenderPassBuilder.hpp"
#include "Subpass.hpp"

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

public: // IFramebuffer Interface
  ISubpass * CreateSubpass();
  AsyncTask * Draw(RenderTarget & renderTarget,
                   std::vector<VkSemaphore> && imageAvailiableSemaphore);
  void SetAttachments(const std::vector<VkAttachmentDescription> & attachments) noexcept;
  void ForEachSubpass(std::function<void(Subpass &)> && func);

public: // IInvalidable Interface
  virtual void Invalidate() override;
  virtual void SetInvalid() override;

public:
  VkRenderPass GetHandle() const noexcept { return m_renderPass; }
  void WaitForReadyToRendering() const noexcept;
  void UpdateRenderingReadyFlag() noexcept;

private:
  uint32_t m_graphicsQueueFamily;
  VkQueue m_graphicsQueue;
  std::vector<VkAttachmentDescription> m_cachedAttachments;

  /// There is a lot of thread-readers, so it's must be synchronized access
  VkRenderPass m_renderPass = VK_NULL_HANDLE;
  bool m_invalidRenderPass = false;
  utils::RenderPassBuilder m_builder;

  /// Flag to notify that subpasses can begin pass
  std::atomic_bool m_isReadyForRendering = false;

  details::Submitter m_submitter;
  std::list<Subpass> m_subpasses;
  uint32_t m_createSubpassCallsCounter = 0;
};


} // namespace RHI::vulkan
