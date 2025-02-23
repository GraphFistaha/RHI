#pragma once

#include <RHI.hpp>
#include <Types.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan::utils
{
template<typename InternalClassT, typename InterfaceClassT>
constexpr decltype(auto) CastInterfaceClass2Internal(InterfaceClassT && obj)
{
    using ResultType = RHI::utils::copy_cv_reference_t<InterfaceClassT, InternalClassT>;
  return dynamic_cast<ResultType>(std::forward<InterfaceClassT>(obj));
}

template<typename VulkanEnumT, typename InterfaceEnumT>
constexpr inline VulkanEnumT CastInterfaceEnum2Vulkan(InterfaceEnumT value);

} // namespace RHI::vulkan::utils


// ------------------------ Specializations ---------------------------
namespace RHI::vulkan::utils
{

template<>
constexpr inline VkIndexType CastInterfaceEnum2Vulkan<VkIndexType, RHI::IndexType>(
  RHI::IndexType type)
{
  using namespace RHI;
  switch (type)
  {
    case IndexType::UINT8:
      return VkIndexType::VK_INDEX_TYPE_UINT8_EXT;
    case IndexType::UINT16:
      return VkIndexType::VK_INDEX_TYPE_UINT16;
    case IndexType::UINT32:
      return VkIndexType::VK_INDEX_TYPE_UINT32;
    default:
      throw std::runtime_error("Failed to cast IndexType to vulkan enum");
  }
}

template<>
constexpr inline VkBufferUsageFlags CastInterfaceEnum2Vulkan<
  VkBufferUsageFlags, RHI::BufferGPUUsage>(RHI::BufferGPUUsage usage)
{
  switch (usage)
  {
    case BufferGPUUsage::VertexBuffer:
      return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    case BufferGPUUsage::IndexBuffer:
      return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    case BufferGPUUsage::UniformBuffer:
      return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    case BufferGPUUsage::StorageBuffer:
      return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    case BufferGPUUsage::TransformFeedbackBuffer:
      return VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT;
    case BufferGPUUsage::IndirectBuffer:
      return VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    default:
      throw std::range_error("Failed to cast usage in Vulkan enum");
  }
}

template<>
constexpr inline VkImageType CastInterfaceEnum2Vulkan<VkImageType, ImageType>(ImageType type)
{
  switch (type)
  {
    case ImageType::Image1D:
      return VK_IMAGE_TYPE_1D;
    case ImageType::Image2D:
      return VK_IMAGE_TYPE_2D;
    case ImageType::Image3D:
      return VK_IMAGE_TYPE_3D;
    case ImageType::Image1D_Array:
      return VK_IMAGE_TYPE_2D;
    case ImageType::Image2D_Array:
    case ImageType::Cubemap:
      return VK_IMAGE_TYPE_3D;
    default:
      throw std::invalid_argument("Invalid image type");
  }
}

template<>
constexpr inline VkImageViewType CastInterfaceEnum2Vulkan<VkImageViewType, ImageType>(
  ImageType type)
{
  switch (type)
  {
    case ImageType::Image1D:
      return VK_IMAGE_VIEW_TYPE_1D;
    case ImageType::Image2D:
      return VK_IMAGE_VIEW_TYPE_2D;
    case ImageType::Image3D:
      return VK_IMAGE_VIEW_TYPE_3D;
    case ImageType::Image1D_Array:
      return VK_IMAGE_VIEW_TYPE_1D_ARRAY;
    case ImageType::Image2D_Array:
      return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    case ImageType::Cubemap:
      return VK_IMAGE_VIEW_TYPE_CUBE;
    default:
      throw std::range_error("Invalid ImageType");
  }
}

/// @brief cats RHI::ImageFormat into internal image format
template<>
constexpr inline VkFormat CastInterfaceEnum2Vulkan<VkFormat, ImageFormat>(ImageFormat format)
{
  switch (format)
  {
    case ImageFormat::R8:
    case ImageFormat::A8:
      return VK_FORMAT_R8_SRGB;
    case ImageFormat::RG8:
    case ImageFormat::RGB8:
    case ImageFormat::RGBA8:
      return VK_FORMAT_R8G8B8A8_SRGB;
    case ImageFormat::BGR8:
    case ImageFormat::BGRA8:
      return VK_FORMAT_B8G8R8A8_SRGB;
    case ImageFormat::DEPTH:
      return VK_FORMAT_D32_SFLOAT;
    case ImageFormat::DEPTH_STENCIL:
      return VK_FORMAT_D24_UNORM_S8_UINT;
    default:
      return VK_FORMAT_UNDEFINED;
  }
}

template<>
constexpr inline VkSampleCountFlagBits CastInterfaceEnum2Vulkan<VkSampleCountFlagBits,
                                                                SamplesCount>(SamplesCount count)
{
  return static_cast<VkSampleCountFlagBits>(count);
}

template<>
inline VkShaderStageFlagBits CastInterfaceEnum2Vulkan<VkShaderStageFlagBits, RHI::ShaderType>(
  RHI::ShaderType type)
{
  return static_cast<VkShaderStageFlagBits>(type);
}

template<>
constexpr inline VkVertexInputRate CastInterfaceEnum2Vulkan<
  VkVertexInputRate, RHI::InputBindingType>(InputBindingType type)
{
  switch (type)
  {
    case InputBindingType::VertexData:
      return VK_VERTEX_INPUT_RATE_VERTEX;
    case InputBindingType::InstanceData:
      return VK_VERTEX_INPUT_RATE_INSTANCE;
    default:
      throw std::runtime_error("Failed to cast InputBindingType to vulkan enum");
  }
}

} // namespace RHI::vulkan::utils
