#include <stdexcept>

#include <catch2/catch_test_macros.hpp>

#include <Memory/LegacySynchronization.hpp>

namespace RHI::vulkan::details
{
/// @brief Single-flag converters reject masks instead of silently substituting synchronization.
TEST_CASE("Legacy flag conversion accepts exactly one supported bit", "[vulkan][memory]")
{
  REQUIRE(ConvertPipelineStageFromVk2ToVk1(VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT) ==
          VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
  REQUIRE(ConvertPipelineStageFromVk2ToVk1(VK_PIPELINE_STAGE_2_COPY_BIT) ==
          VK_PIPELINE_STAGE_TRANSFER_BIT);
  REQUIRE(ConvertAccessMaskFromVk2ToVk1(VK_ACCESS_2_SHADER_STORAGE_READ_BIT) ==
          VK_ACCESS_SHADER_READ_BIT);
  REQUIRE_THROWS_AS(ConvertPipelineStageFromVk2ToVk1(VK_PIPELINE_STAGE_2_NONE),
                    std::invalid_argument);
  REQUIRE_THROWS_AS(ConvertAccessMaskFromVk2ToVk1(VK_ACCESS_2_NONE), std::invalid_argument);
  REQUIRE_THROWS_AS(ConvertPipelineStageFromVk2ToVk1(VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT |
                                                     VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT),
                    std::invalid_argument);
  REQUIRE_THROWS_AS(ConvertAccessMaskFromVk2ToVk1(VK_ACCESS_2_SHADER_READ_BIT |
                                                  VK_ACCESS_2_SHADER_WRITE_BIT),
                    std::invalid_argument);
  REQUIRE_THROWS_AS(ConvertPipelineStageFromVk2ToVk1(VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT),
                    std::invalid_argument);
  REQUIRE_THROWS_AS(ConvertAccessMaskFromVk2ToVk1(VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR),
                    std::invalid_argument);
}

/// @brief Guards against dropping access scopes when multiple bits are requested.
TEST_CASE("Legacy conversion preserves combined scopes", "[vulkan][memory]")
{
  REQUIRE(ConvertPipelineStageFromVk2ToVk1(VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT |
                                               VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT) ==
          (VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT));
  REQUIRE(
    ConvertAccessMaskFromVk2ToVk1(VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT |
                                  VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT) ==
    (VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT));
}

/// @brief Verifies that finer synchronization2 scopes collapse to their legacy equivalents.
TEST_CASE("Legacy conversion merges finer scopes", "[vulkan][memory]")
{
  REQUIRE(ConvertPipelineStageFromVk2ToVk1(
            VK_PIPELINE_STAGE_2_COPY_BIT | VK_PIPELINE_STAGE_2_BLIT_BIT |
            VK_PIPELINE_STAGE_2_RESOLVE_BIT | VK_PIPELINE_STAGE_2_CLEAR_BIT |
            VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT | VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT) ==
          (VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_VERTEX_INPUT_BIT));
  REQUIRE(ConvertAccessMaskFromVk2ToVk1(VK_ACCESS_2_SHADER_SAMPLED_READ_BIT |
                                        VK_ACCESS_2_SHADER_STORAGE_READ_BIT |
                                        VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT) ==
          (VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT));
}

/// @brief Broad graphics scopes must be replaced by the actual stages used by the caller.
TEST_CASE("Legacy conversion requires explicit graphics and command stages", "[vulkan][memory]")
{
  REQUIRE_THROWS_AS(ConvertPipelineStageFromVk2ToVk1(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT),
                    std::invalid_argument);
  REQUIRE_THROWS_AS(ConvertPipelineStageFromVk2ToVk1(VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT),
                    std::invalid_argument);
  REQUIRE_THROWS_AS(ConvertPipelineStageFromVk2ToVk1(
                      VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT |
                      VK_PIPELINE_STAGE_2_PRE_RASTERIZATION_SHADERS_BIT),
                    std::invalid_argument);
}

/// @brief Verifies that empty scopes stay empty and unsupported bits cannot be silently discarded.
TEST_CASE("Legacy conversion validates scopes", "[vulkan][memory]")
{
  REQUIRE(ConvertPipelineStageFromVk2ToVk1(VK_PIPELINE_STAGE_2_NONE) == 0);
  REQUIRE(ConvertAccessMaskFromVk2ToVk1(VK_ACCESS_2_NONE) == 0);
  REQUIRE_THROWS_AS(ConvertPipelineStageFromVk2ToVk1(
                      VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT |
                      VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR),
                    std::invalid_argument);
  REQUIRE_THROWS_AS(ConvertAccessMaskFromVk2ToVk1(VK_ACCESS_2_SHADER_READ_BIT |
                                                  VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR),
                    std::invalid_argument);
}
} // namespace RHI::vulkan::details
