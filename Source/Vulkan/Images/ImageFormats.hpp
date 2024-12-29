#pragma once
#include <RHI.hpp>
// Table with all vulkan image formats and their mapping of C types
#define FOR_EACH_VULKAN_IMAGE_FORMAT(IMAGE_FORMAT_MACRO)                                           \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8G8B8A8_SRGB, uint32_t)                                  \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8G8B8A8_SNORM, uint32_t)                                 \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8G8B8A8_UNORM, uint32_t)                                 \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8G8B8A8_SINT, uint32_t)                                  \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8G8B8A8_UINT, uint32_t)                                  \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8G8B8A8_SSCALED, uint32_t)                               \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8G8B8A8_USCALED, uint32_t)                               \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8G8B8_SRGB, char24_t)                                    \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8G8B8_SNORM, char24_t)                                   \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8G8B8_UNORM, char24_t)                                   \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8G8B8_SINT, char24_t)                                    \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8G8B8_UINT, char24_t)                                    \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8G8B8_SSCALED, char24_t)                                 \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8G8B8_USCALED, char24_t)                                 \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8G8_SRGB, uint16_t)                                      \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8G8_SNORM, uint16_t)                                     \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8G8_UNORM, uint16_t)                                     \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8G8_SINT, uint16_t)                                      \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8G8_UINT, uint16_t)                                      \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8G8_SSCALED, uint16_t)                                   \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8G8_USCALED, uint16_t)                                   \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8_SRGB, uint8_t)                                         \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8_SNORM, uint8_t)                                        \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8_UNORM, uint8_t)                                        \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8_SINT, uint8_t)                                         \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8_UINT, uint8_t)                                         \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8_SSCALED, uint8_t)                                      \
  IMAGE_FORMAT_MACRO(VkFormat, VK_FORMAT_R8_USCALED, uint8_t)

// Table with all host image formats and their mapping of C types
#define FOR_EACH_HOST_IMAGE_FORMAT(IMAGE_FORMAT_MACRO)                                             \
  IMAGE_FORMAT_MACRO(RHI::HostImageFormat, RHI::HostImageFormat::RGBA8, uint32_t)                            \
  IMAGE_FORMAT_MACRO(RHI::HostImageFormat, RHI::HostImageFormat::A8, uint8_t)                                \
  IMAGE_FORMAT_MACRO(RHI::HostImageFormat, RHI::HostImageFormat::R8, uint8_t)                                \
  IMAGE_FORMAT_MACRO(RHI::HostImageFormat, RHI::HostImageFormat::RG8, uint16_t)                              \
  IMAGE_FORMAT_MACRO(RHI::HostImageFormat, RHI::HostImageFormat::RGB8, char24_t)                             \
  IMAGE_FORMAT_MACRO(RHI::HostImageFormat, RHI::HostImageFormat::BGR8, char24_t)                             \
  IMAGE_FORMAT_MACRO(RHI::HostImageFormat, RHI::HostImageFormat::BGRA8, uint32_t)
