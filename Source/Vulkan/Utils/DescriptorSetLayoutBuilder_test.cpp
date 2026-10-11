#include <stdexcept>

#include <catch2/catch_test_macros.hpp>

#include <Utils/DescriptorSetLayoutBuilder.hpp>

namespace RHI::vulkan::utils
{
/// @brief Checks that declarations reject gaps and duplicates without consuming a binding.
TEST_CASE("Descriptor layout bindings must be contiguous", "[vulkan][utils]")
{
  DescriptorSetLayoutBuilder builder;
  REQUIRE_THROWS_AS(builder.DeclareDescriptor(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                              ShaderType::Vertex),
                    std::runtime_error);
  REQUIRE_NOTHROW(
    builder.DeclareDescriptor(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, ShaderType::Vertex));
  REQUIRE_THROWS_AS(builder.DeclareDescriptor(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                              ShaderType::Vertex),
                    std::runtime_error);
  REQUIRE_NOTHROW(
    builder.DeclareDescriptor(1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, ShaderType::Fragment));
}

/// @brief Checks that descriptor arrays occupy one binding regardless of their element count.
TEST_CASE("Descriptor arrays advance one binding", "[vulkan][utils]")
{
  // Several elements distinguish an array's descriptor count from its binding count.
  constexpr uint32_t arraySize = 4;
  DescriptorSetLayoutBuilder builder;
  REQUIRE_NOTHROW(builder.DeclareDescriptorsArray(0, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                                                  ShaderType::Fragment, arraySize));
  REQUIRE_NOTHROW(
    builder.DeclareDescriptor(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, ShaderType::Vertex));
}

/// @brief Checks that reset allows the builder to be reused from binding zero.
TEST_CASE("Descriptor layout reset clears declarations", "[vulkan][utils]")
{
  DescriptorSetLayoutBuilder builder;
  builder.DeclareDescriptor(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, ShaderType::Vertex);
  builder.Reset();
  REQUIRE_THROWS_AS(builder.DeclareDescriptor(1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                                              ShaderType::Fragment),
                    std::runtime_error);
  REQUIRE_NOTHROW(
    builder.DeclareDescriptor(0, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, ShaderType::Fragment));
  builder.Reset();
  builder.Reset();
  REQUIRE_NOTHROW(
    builder.DeclareDescriptor(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, ShaderType::Vertex));
}
} // namespace RHI::vulkan::utils
