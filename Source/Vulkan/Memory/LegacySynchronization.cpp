#include "LegacySynchronization.hpp"

#include <bit>
#include <stdexcept>
#include <unordered_map>

namespace
{
VkPipelineStageFlags ConvertPipelineStageFlagFromVk2ToVk1(VkPipelineStageFlagBits2 stage)
{
  if (!std::has_single_bit(stage))
    throw std::invalid_argument("Stage conversion requires exactly one flag");

  // Only explicit core stages are supported; broad graphics and command scopes are rejected.
  static const std::unordered_map<VkPipelineStageFlagBits2, VkPipelineStageFlags> mapping = {
    {VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT},
    {VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT},
    {VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT},
    {VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT},
    {VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT,
     VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT},
    {VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT,
     VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT},
    {VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT, VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT},
    {VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT},
    {VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT},
    {VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT},
    {VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
    {VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT},
    {VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT},
    {VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT},
    {VK_PIPELINE_STAGE_2_HOST_BIT, VK_PIPELINE_STAGE_HOST_BIT},
    {VK_PIPELINE_STAGE_2_COPY_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT},
    {VK_PIPELINE_STAGE_2_BLIT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT},
    {VK_PIPELINE_STAGE_2_RESOLVE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT},
    {VK_PIPELINE_STAGE_2_CLEAR_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT},
    {VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT},
    {VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT},
  };
  const auto entry = mapping.find(stage);
  if (entry == mapping.end())
    throw std::invalid_argument("Unsupported synchronization2 stage for legacy synchronization");
  return entry->second;
}

VkAccessFlags ConvertAccessFlagFromVk2ToVk1(VkAccessFlagBits2 access)
{
  if (!std::has_single_bit(access))
    throw std::invalid_argument("Access conversion requires exactly one flag");

  static const std::unordered_map<VkAccessFlagBits2, VkAccessFlags> mapping = {
    {VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT, VK_ACCESS_INDIRECT_COMMAND_READ_BIT},
    {VK_ACCESS_2_INDEX_READ_BIT, VK_ACCESS_INDEX_READ_BIT},
    {VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT},
    {VK_ACCESS_2_UNIFORM_READ_BIT, VK_ACCESS_UNIFORM_READ_BIT},
    {VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT, VK_ACCESS_INPUT_ATTACHMENT_READ_BIT},
    {VK_ACCESS_2_SHADER_READ_BIT, VK_ACCESS_SHADER_READ_BIT},
    {VK_ACCESS_2_SHADER_WRITE_BIT, VK_ACCESS_SHADER_WRITE_BIT},
    {VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT},
    {VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT},
    {VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT},
    {VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT},
    {VK_ACCESS_2_TRANSFER_READ_BIT, VK_ACCESS_TRANSFER_READ_BIT},
    {VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT},
    {VK_ACCESS_2_HOST_READ_BIT, VK_ACCESS_HOST_READ_BIT},
    {VK_ACCESS_2_HOST_WRITE_BIT, VK_ACCESS_HOST_WRITE_BIT},
    {VK_ACCESS_2_MEMORY_READ_BIT, VK_ACCESS_MEMORY_READ_BIT},
    {VK_ACCESS_2_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_WRITE_BIT},
    {VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_ACCESS_SHADER_READ_BIT},
    {VK_ACCESS_2_SHADER_STORAGE_READ_BIT, VK_ACCESS_SHADER_READ_BIT},
    {VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, VK_ACCESS_SHADER_WRITE_BIT},
  };
  const auto entry = mapping.find(access);
  if (entry == mapping.end())
    throw std::invalid_argument("Unsupported synchronization2 access for legacy synchronization");
  return entry->second;
}
} // namespace

namespace RHI::vulkan::details
{

VkPipelineStageFlags ConvertPipelineStageFromVk2ToVk1(VkPipelineStageFlags2 stages)
{
  VkPipelineStageFlags result = 0;
  while (stages)
  {
    const auto stage = std::bit_floor(stages);
    result |= ConvertPipelineStageFlagFromVk2ToVk1(stage);
    stages ^= stage;
  }
  return result;
}

VkAccessFlags ConvertAccessMaskFromVk2ToVk1(VkAccessFlags2 accessMask)
{
  VkAccessFlags result = 0;
  while (accessMask)
  {
    const auto access = std::bit_floor(accessMask);
    result |= ConvertAccessFlagFromVk2ToVk1(access);
    accessMask ^= access;
  }
  return result;
}
} // namespace RHI::vulkan::details
