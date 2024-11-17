#pragma once

#include <list>

#include "../VulkanContext.hpp"
#include "FramebufferAttachment.hpp"

namespace RHI::vulkan
{
namespace details
{
struct RenderPassBuilder;
struct Submitter;
} // namespace details
struct RenderTarget;
struct Subpass;

struct RenderPass : public IInvalidable
{
  explicit RenderPass(const Context & ctx);
  virtual ~RenderPass() override;

public: // IRenderPass Interface
  ISubpass * CreateSubpass();
  VkSemaphore Draw(VkSemaphore imageAvailiableSemaphore);
  void BindRenderTarget(const RenderTarget * renderTarget) noexcept;

public: // IInvalidable Interface
  virtual void Invalidate() override;

public:
  vk::RenderPass GetHandle() const noexcept { return m_renderPass; }

private:
  const Context & m_context;
  const RenderTarget * m_boundRenderTarget = nullptr;

  uint32_t m_graphicsQueueFamily;
  vk::Queue m_graphicsQueue;
  std::vector<FramebufferAttachment> m_cachedAttachments;

  vk::RenderPass m_renderPass = VK_NULL_HANDLE;
  bool m_invalidRenderPass : 1 = false;
  std::unique_ptr<details::RenderPassBuilder> m_builder;

  std::unique_ptr<details::Submitter> m_submitter;
  std::list<Subpass> m_subpasses;
};
} // namespace RHI::vulkan
