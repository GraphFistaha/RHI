#pragma once

#include <bitset>
#include <span>
#include <vector>

#include <Attachments/Attachment.hpp>
#include <Memory/ResourceUser.hpp>
#include <Private/OwnedBy.hpp>
#include <RenderPass/RenderTarget.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

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
  /// @brief adds attachment to all frames
  /// @param binding - index of binding
  /// @param args - arguments for image creation
  virtual void AddAttachment(uint32_t binding, IAttachment * attachment) override;

  virtual void Resize(uint32_t width, uint32_t height) override;
  virtual RHI::TexelIndex GetExtent() const override;

public: // ICommandWriter
  void RecordCommands(details::CommandBuffer & commands);

public: // IResourceUser
  void CollectResources(std::vector<ResourcePtr> & resources) const;

public: // RHI-only API
  size_t GetImagesCount() const noexcept;
  bool Invalidate(std::span<const VkImageUsageFlags> attachmentsUsageFlags);
  /// begins rendering
  RenderTarget * BeginFrame();
  /// finish rendering
  void EndFrame(VkSemaphore renderPassSemaphore);

  //TODO: shared_ptr
  std::span<IInternalAttachment *> GetAttachments() noexcept;
  IInternalAttachment * GetAttachment(uint32_t idx) const;
  RHI::SamplesCount CalcSamplesCount() const noexcept;
  //std::span<const VkAttachmentDescription> GetAttachementsDescription() const noexcept;

protected:
  std::vector<RenderTarget> m_targets; //TODO: small_vector
  uint32_t m_activeTarget = -1;

  //TODO: shared_ptr
  std::vector<IInternalAttachment *> m_attachments; //sort by count of buffers
  bool m_attachmentsChanged = false;
};

} // namespace RHI::vulkan
