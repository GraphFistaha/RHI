#include "InputAttachmentUniform.hpp"

#include <Descriptors/DescriptorBufferLayout.hpp>
#include <Memory/Synchronizer.hpp>
#include <Private/FastDynamicCast.hpp>
#include <RenderPass/SubpassConfiguration.hpp>
#include <Utils/CastHelper.hpp>
#include <VulkanContext.hpp>

namespace RHI::vulkan
{
InputAttachmentUniform::InputAttachmentUniform(Context & ctx, DescriptorBufferLayout & owner,
                                               LayoutIndex index)
  : BaseDescriptor(ctx, owner, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, index, 0)
{
}

UpdateDescriptorTask InputAttachmentUniform::CreateUpdateTask() const noexcept
{
  return [binding = GetBinding(), arrayIdx = GetArrayIndex(), type = GetDescriptorType(),
          setIdx = GetSet()](const Context & ctx, std::span<const VkDescriptorSet> sets) mutable
  {
    VkWriteDescriptorSet writeInfo{};
    writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeInfo.descriptorType = type;
    writeInfo.dstArrayElement = arrayIdx;
    writeInfo.dstBinding = binding;
    writeInfo.dstSet = sets[setIdx];
    writeInfo.descriptorCount = 0;
    //writeInfo.pImageInfo[0].
    vkUpdateDescriptorSets(ctx.GetGpuConnection().GetDevice(), 1, &writeInfo, 0, nullptr);
  };
}

void InputAttachmentUniform::CollectResources(std::vector<ResourcePtr> & resources) const
{
}

void InputAttachmentUniform::SynchroniseResources(details::CommandBuffer & commands) const
{
}

void InputAttachmentUniform::Invalidate()
{
}

void InputAttachmentUniform::SetInvalid()
{
}

} // namespace RHI::vulkan
