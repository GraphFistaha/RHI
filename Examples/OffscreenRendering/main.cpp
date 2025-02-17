#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <RHI.hpp>

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
    case RHI::LogMessageStatus::LOG_DEBUG:
      std::printf("DEBUG: - %s\n", message.c_str());
      break;
  }
}

int main()
{
  std::unique_ptr<RHI::IContext> ctx = RHI::CreateContext(nullptr, ConsoleLog);

  std::unique_ptr<RHI::ISwapchain> swapchain = ctx->CreateOffscreenSwapchain(100, 100, 3);
  RHI::ImageCreateArguments description;
  description.extent = {100, 100, 1};
  description.format = RHI::ImageFormat::RGBA8;
  description.mipLevels = 1;
  description.samples = RHI::SamplesCount::One;
  description.type = RHI::ImageType::Image2D;
  description.shared = false;
  swapchain->AddImageAttachment(0, description); // Мб она должна вернуть какой-то объект, который можно будет передать в другой Swapchain и тем самым связать два Swapchain'a

  float t = 0.0;
  std::vector<uint8_t> pixels;
  for (int i = 0; i < 1; ++i)
  {
    if (RHI::IRenderTarget * renderTarget = swapchain->AcquireFrame())
    {
      renderTarget->SetClearValue(0, 0.1f, std::abs(std::sin(t)), 0.4f, 1.0f);
      auto * awaitable = swapchain->RenderFrame();
      awaitable->Wait(); // после этого, все прикрепления должны перейти в состояние finalLayout

      RHI::ImageRegion region{{0, 0, 0}, description.extent};
      auto future = renderTarget->GetImage(0)->DownloadImage(RHI::HostImageFormat::RGBA8, region);
      ctx->Flush();
      ctx->Flush();
      pixels = std::move(future.get());
    }
    t += 0.001f;
  }
  return 0;
}
