#include "TestUtils.hpp"

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

RHI::ITexture * UploadTexture(const char * path, RHI::IContext * ctx,
                                             bool with_alpha)
{
  int w = 0, h = 0, channels = 3;
  uint8_t * pixel_data = stbi_load(path, &w, &h, &channels, with_alpha ? STBI_rgb_alpha : STBI_rgb);
  if (!pixel_data)
    throw std::runtime_error("Failed to load texture. Check it exists near the exe file");

  RHI::TexelIndex extent = {static_cast<uint32_t>(w), static_cast<uint32_t>(h), 1};

  RHI::ImageCreateArguments imageArgs{};
  imageArgs.extent = extent;
  imageArgs.type = RHI::ImageType::Image2D;
  imageArgs.format = with_alpha ? RHI::ImageFormat::RGBA8 : RHI::ImageFormat::RGB8;
  imageArgs.mipLevels = 1;
  auto texture = ctx->AllocImage(imageArgs);
  texture->UploadImage(pixel_data, extent,
                       with_alpha ? RHI::HostImageFormat::RGBA8 : RHI::HostImageFormat::RGB8,
                       {{}, extent}, {{}, extent});
  stbi_image_free(pixel_data);
  return texture;
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
