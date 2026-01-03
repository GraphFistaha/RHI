#include "TestUtils.hpp"

#include <fstream>

#include <GLFW/glfw3.h>

// clang-format off
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
// clang-format on


void ConsoleLog(RHI::LogMessageStatus status, const std::string & message)
{
  switch (status)
  {
    case RHI::LogMessageStatus::LOG_INFO:
      std::printf("INFO: - %s\n", message.c_str());
      break;
    case RHI::LogMessageStatus::LOG_WARNING:
      std::printf("WARNING: - %s\n", message.c_str());
      break;
    case RHI::LogMessageStatus::LOG_ERROR:
      std::printf("ERROR: - %s\n", message.c_str());
      break;
    case RHI::LogMessageStatus::LOG_DEBUG:
      std::printf("DEBUG: - %s\n", message.c_str());
      break;
  }
}

RHI::ITexture * UploadTexture(const char * path, RHI::IContext * ctx, bool with_alpha)
{
  int w = 0, h = 0, channels = 3;
  uint8_t * pixel_data = stbi_load(path, &w, &h, &channels, with_alpha ? STBI_rgb_alpha : STBI_rgb);
  if (!pixel_data)
    throw std::runtime_error("Failed to load texture. Check it exists near the exe file");

  RHI::TexelIndex extent = {static_cast<uint32_t>(w), static_cast<uint32_t>(h), 1};

  RHI::TextureDescription imageArgs{};
  imageArgs.extent = extent;
  imageArgs.type = RHI::ImageType::Image2D;
  imageArgs.format = with_alpha ? RHI::ImageFormat::RGBA8 : RHI::ImageFormat::RGB8;
  imageArgs.mipLevels = 1;
  auto texture = ctx->CreateTexture(imageArgs);
  texture->UploadImage(pixel_data, extent,
                       with_alpha ? RHI::HostImageFormat::RGBA8 : RHI::HostImageFormat::RGB8,
                       {{}, extent}, {{}, extent});
  stbi_image_free(pixel_data);
  return texture;
}

RHI::SpirV ReadSpirV(const std::filesystem::path & path)
{
  // Open file at end to get size immediately
  std::ifstream file(path, std::ios::ate | std::ios::binary);
  if (!file.is_open())
    throw std::runtime_error("Failed to open SPIR-V file: ");

  // Get size in bytes
  size_t fileSize = static_cast<size_t>(file.tellg());
  if (fileSize % 4 != 0)
    throw std::runtime_error("SPIR-V file size is not a multiple of 4 bytes: ");

  // Allocate vector with proper number of 32-bit words
  RHI::SpirV buffer(fileSize / 4, 0);

  // Seek back to start and read data
  file.seekg(0);
  file.read(reinterpret_cast<char *>(buffer.data()), fileSize);

  if (!file)
    throw std::runtime_error("Failed to read complete SPIR-V file");

  return buffer;
}

std::filesystem::path FromGLSL(const std::filesystem::path & path)
{
  constexpr const char * apiFolder = ".vulkan";
  constexpr const char * spirvExtension = ".spv";

  std::filesystem::path suffix;
  auto && ext = path.extension();
  if (ext == ".vert")
    suffix = "_vert";
  else if (ext == ".frag")
    suffix = "_frag";
  else if (ext == ".geom")
    suffix = "_geom";
  else if (ext == ".glsl")
  {
    suffix = "";
  }
  else
    assert(false && "Invalid format for shader file");
  std::filesystem::path result = apiFolder / path.parent_path() / path.stem();
  result += suffix;
  result += spirvExtension;
  return result;
}

namespace RHI::test_examples
{

GlfwInstance::GlfwInstance()
{
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
}

GlfwInstance::~GlfwInstance()
{
  glfwTerminate();
}

} // namespace RHI::test_examples
