#include <cmath>
#include <cstdio>
#include <cstring>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include <RHI.hpp>

#include "stb_image.h"
#include "stb_image_write.h"

// Custom log function used by RHI::Context
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
  }
}

/// @brief uploads image from file and create RHI image object
RHI::ITexture * CreateAndLoadImage(RHI::IContext & ctx, const char * path, bool with_alpha)
{
  int w = 0, h = 0, channels = 3;
  uint8_t * pixel_data = stbi_load(path, &w, &h, &channels, with_alpha ? STBI_rgb_alpha : STBI_rgb);
  if (!pixel_data)
  {
    throw std::runtime_error("Failed to load texture. Check it exists near the exe file");
  }

  RHI::ImageExtent extent = {static_cast<uint32_t>(w), static_cast<uint32_t>(h), 1};

  RHI::ImageCreateArguments imageArgs{};
  imageArgs.extent = extent;
  imageArgs.type = RHI::ImageType::Image2D;
  imageArgs.shared = false;
  imageArgs.format = with_alpha ? RHI::ImageFormat::RGBA8 : RHI::ImageFormat::RGB8;
  imageArgs.mipLevels = 1;
  imageArgs.samples = RHI::SamplesCount::One;
  auto texture = ctx.AllocImage(imageArgs);
  RHI::CopyImageArguments copyArgs{};
  copyArgs.hostFormat = with_alpha ? RHI::HostImageFormat::RGBA8 : RHI::HostImageFormat::RGB8;
  copyArgs.src.extent = extent;
  copyArgs.dst.extent = extent;
  texture->UploadImage(pixel_data, copyArgs);
  stbi_image_free(pixel_data);
  return texture;
}

int main()
{
  std::unique_ptr<RHI::IContext> ctx = RHI::CreateContext(nullptr, ConsoleLog);

  auto texture = CreateAndLoadImage(*ctx, "mike_wazowski.jpg", false);
  RHI::ImageRegion region2Download;
  region2Download.extent = texture->GetDescription().extent;
  region2Download.offset = {0, 0, 0};
  auto future = texture->DownloadImage(RHI::HostImageFormat::RGB8, region2Download);

  while (!future._Is_ready())
    ctx->Flush();
  auto result = future.get();
  stbi_write_bmp("downloaded_image.bmp", region2Download.extent[0], region2Download.extent[1], 3,
                 result.data());
  return 0;
}
