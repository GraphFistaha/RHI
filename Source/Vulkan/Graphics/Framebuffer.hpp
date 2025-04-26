#pragma once

#include <bitset>
#include <vector>

#include <OwnedBy.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "RenderPass.hpp"
#include "RenderTarget.hpp"
#include "TextureInterface.hpp"

namespace RHI::vulkan
{

/// @brief vulkan implementation for renderer
struct Framebuffer : public IFramebuffer,
                     public OwnedBy<Context>
{
  explicit Framebuffer(Context & ctx);
  virtual ~Framebuffer() override;
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);

public: // IFramebuffer interface
  /// begins rendering
  virtual IRenderTarget * BeginFrame() override;
  /// finish rendering
  virtual IAwaitable * EndFrame() override;
  ///
  virtual ISubpass * CreateSubpass() override;
  /// @brief adds attachment to all frames
  /// @param binding - index of binding
  /// @param args - arguments for image creation
  virtual void AddAttachment(uint32_t binding, IAttachment * attachment) override;
  /// @brief removes all images from all frames
  virtual void ClearAttachments() noexcept override;
  /// @brief operation which add or remove some frames from swapchain
  /// @param frames_count
  virtual void SetFramesCount(uint32_t frames_count) override;

public: // RHI-only API
  size_t GetImagesCount() const noexcept;
  void Invalidate();

  using AttachmentProcessFunc = std::function<void(IInternalAttachment*)>;
  void ForEachAttachment(AttachmentProcessFunc && func);

protected:
  std::vector<RenderTarget> m_targets;
  uint32_t m_activeTarget = -1;
  RenderPass m_renderPass;

  std::vector<IInternalAttachment *> m_attachments; //sort by count of buffers
  bool m_attachmentsChanged = false;
  std::vector<VkAttachmentDescription> m_attachmentDescriptions;
  std::vector<VkSemaphore> m_imagesAvailabilitySemaphores;

  uint32_t m_framesCount = 0;

  std::atomic_bool m_frameStarted = false;
};

} // namespace RHI::vulkan


/// @brief Compare operator for VkAttachmentDescription
inline bool operator==(const VkAttachmentDescription & lhs,
                       const VkAttachmentDescription & rhs) noexcept
{
  return std::memcmp(&lhs, &rhs, sizeof(VkAttachmentDescription)) == 0;
}
