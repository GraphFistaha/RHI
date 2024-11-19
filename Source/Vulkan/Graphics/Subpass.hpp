#pragma once
#include <mutex>

#include "../VulkanContext.hpp"
#include "Commands.hpp"

namespace RHI::vulkan
{
namespace details
{
struct CommandBuffer;
}
struct RenderPass;
struct Pipeline;

struct Subpass : public ISubpass,
                 public GraphicsCommands
{
  explicit Subpass(const Context & ctx, const RenderPass & ownerPass, uint32_t subpassIndex,
                   uint32_t familyIndex);
  virtual ~Subpass() override;

public: // ISubpass Interface
  virtual void BeginPass() override;
  virtual void EndPass() override;
  virtual IPipeline & GetConfiguration() & noexcept override;
  virtual void SetEnabled(bool enabled) noexcept override;
  virtual bool IsEnabled() const noexcept override;

public: // IInvalidable Interface
  virtual void Invalidate() override;

public:
  const details::CommandBuffer & GetCommandBuffer() const & noexcept { return *m_buffer; }
  void LockWriting(bool lock) const noexcept;

private:
  const Context & m_context;
  const RenderPass & m_ownerPass;
  VkRenderPass m_cachedRenderPass = VK_NULL_HANDLE;
  std::unique_ptr<details::CommandBuffer> m_buffer;
  std::unique_ptr<Pipeline> m_pipeline;
  mutable std::mutex m_write_lock;
  std::atomic_bool m_enabled = true;
};

} // namespace RHI::vulkan
