#include "TestUtils.hpp"

#include <algorithm>
#include <cassert>
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

RHI::ITexture * UploadTexture(const char * path, RHI::IContext * ctx, bool with_alpha,
                              bool useMips /* = false*/)
{
  int w = 0, h = 0, channels = 3;
  uint8_t * pixel_data = stbi_load(path, &w, &h, &channels, with_alpha ? STBI_rgb_alpha : STBI_rgb);
  if (!pixel_data)
    throw std::runtime_error("Failed to load texture. Check it exists near the exe file");

  RHI::HostTextureView hostTexture{};
  {
    hostTexture.extent = {static_cast<uint32_t>(w), static_cast<uint32_t>(h), 1};
    hostTexture.pixelData = reinterpret_cast<uint8_t *>(pixel_data);
    hostTexture.format = with_alpha ? RHI::HostImageFormat::RGBA8 : RHI::HostImageFormat::RGB8;
    hostTexture.layersCount = 1;
  };

  RHI::TextureDescription imageArgs{};
  {
    imageArgs.extent = hostTexture.extent;
    imageArgs.layersCount = hostTexture.layersCount;
    imageArgs.type = RHI::ImageType::Image2D;
    imageArgs.format = with_alpha ? RHI::ImageFormat::RGBA8 : RHI::ImageFormat::RGB8;
    imageArgs.mipLevels = useMips ? RHI::CalcMaxMipLevels(imageArgs.extent) : 1;
    ;
  }
  auto texture = ctx->CreateTexture(imageArgs);

  RHI::UploadImageArgs args{};
  {
    args.srcTexture = hostTexture;
    args.copyRegion = {{0, 0, 0}, hostTexture.extent};
    args.dstOffset = {0, 0, 0};
    args.layerIndex = 0;
    args.layersCount = 1;
  }
  texture->UploadImage(args);
  if (useMips)
    texture->GenerateMipmaps();
  stbi_image_free(pixel_data);
  return texture;
}

RHI::ITexture * UploadLayeredTexture(RHI::IContext * ctx,
                                     const std::vector<std::filesystem::path> & paths,
                                     bool with_alpha, bool useMips /* = false*/)
{
  int width = 0, height = 0;
  std::vector<RHI::HostTextureView> textures;
  for (auto && path : paths)
  {
    int w = 0, h = 0, channels = 3;
    uint8_t * pixel_data =
      stbi_load(path.string().c_str(), &w, &h, &channels, with_alpha ? STBI_rgb_alpha : STBI_rgb);
    if (!pixel_data)
      throw std::runtime_error("Failed to load texture. Check it exists near the exe file");
    if (width != 0 && width != w || height != 0 && height != h)
      throw std::runtime_error("Images have different sizes");

    auto && hostTexture = textures.emplace_back();
    hostTexture.pixelData = pixel_data;
    hostTexture.extent = {static_cast<uint32_t>(w), static_cast<uint32_t>(h), 1};
    hostTexture.layersCount = 1;
    hostTexture.format = with_alpha ? RHI::HostImageFormat::RGBA8 : RHI::HostImageFormat::RGB8;
  }

  bool allTheSameSize = std::all_of(textures.begin(), textures.end(), [&](auto && texture)
                                    { return texture.extent == textures.front().extent; });
  assert(allTheSameSize);

  RHI::TextureDescription imageArgs{};
  {
    imageArgs.extent = textures.front().extent;
    imageArgs.type = RHI::ImageType::Image2D_Array;
    imageArgs.format = with_alpha ? RHI::ImageFormat::RGBA8 : RHI::ImageFormat::RGB8;
    imageArgs.mipLevels = useMips ? RHI::CalcMaxMipLevels(imageArgs.extent) : 1;
    imageArgs.layersCount = static_cast<uint32_t>(textures.size());
  }
  auto texture = ctx->CreateTexture(imageArgs);

  for (uint32_t i = 0; i < textures.size(); ++i)
  {
    RHI::HostTextureView & hostTexture = textures[i];
    RHI::UploadImageArgs args{};
    {
      args.srcTexture = hostTexture;
      args.copyRegion = {{0, 0, 0}, {hostTexture.extent}};
      args.dstOffset = {0, 0, 0};
      args.layerIndex = i;
      args.layersCount = 1;
    }
    texture->UploadImage(args);
  }

  if (useMips)
    texture->GenerateMipmaps();

  for (auto && hostTexture : textures)
    stbi_image_free(hostTexture.pixelData);
  textures.clear();
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
  std::filesystem::path result = path.parent_path() / path.stem();
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
