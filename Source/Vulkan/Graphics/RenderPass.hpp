#pragma once

#include <condition_variable>
#include <list>
#include <shared_mutex>

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "../CommandsExecution/Submitter.hpp"
#include "../Utils/RenderPassBuilder.hpp"
#include "FramebufferAttachment.hpp"
#include "Subpass.hpp"

namespace RHI::vulkan
{
struct Context;
struct RenderTarget;
} // namespace RHI::vulkan

namespace RHI::vulkan
{

struct RenderPass : public IInvalidable
{
  explicit RenderPass(const Context & ctx);
  virtual ~RenderPass() override;

public: // IRenderPass Interface
  ISubpass * CreateSubpass();
  VkSemaphore Draw(VkSemaphore imageAvailiableSemaphore);
  void BindRenderTarget(const RenderTarget * renderTarget) noexcept;
  void ForEachSubpass(std::function<void(Subpass &)> && func);

public: // IInvalidable Interface
  virtual void Invalidate() override;
  virtual void SetInvalid() override;

public:
  VkRenderPass GetHandle() const noexcept { return m_renderPass; }
  void WaitForReadyToRendering() const noexcept;
  void UpdateRenderingReadyFlag() noexcept;

private:
  const Context & m_context;
  const RenderTarget * m_boundRenderTarget = nullptr;

  uint32_t m_graphicsQueueFamily;
  VkQueue m_graphicsQueue;
  std::vector<FramebufferAttachment> m_cachedAttachments;

  /// There is a lot of thread-readers, so it's must be synchronized access
  VkRenderPass m_renderPass = VK_NULL_HANDLE;
  bool m_invalidRenderPass : 1 = false;
  utils::RenderPassBuilder m_builder;

  /// Flag to notify that subpasses can begin pass
  std::atomic_bool m_isReadyForRendering = false;

  details::Submitter m_submitter;
  std::list<Subpass> m_subpasses;
};
} // namespace RHI::vulkan
