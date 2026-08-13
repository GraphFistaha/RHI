#pragma once

#include <Descriptors/BaseDescriptor.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>


namespace RHI::vulkan
{

struct InputAttachmentUniform final : public details::BaseDescriptor
{
  explicit InputAttachmentUniform(Context & ctx, DescriptorBufferLayout & owner, LayoutIndex index);
  virtual ~InputAttachmentUniform() override = default;


  virtual UpdateDescriptorTask CreateUpdateTask() const noexcept override;

public: // IResourceUser
  virtual void CollectResources(std::vector<ResourcePtr> & resources) const override;
  virtual void SynchroniseResources(details::CommandBuffer & commands) const override;

public: // IInvalidable interface
  void Invalidate();
  void SetInvalid();
};

} // namespace RHI::vulkan
