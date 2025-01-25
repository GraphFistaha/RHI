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
  RHI::ImageDescription description;
  description.extent = {100, 100, 1};
  description.format = RHI::ImageFormat::RGBA8;
  description.mipLevels = 1;
  description.samples = RHI::SamplesCount::One;
  description.type = RHI::ImageType::Image2D;
  description.usage = RHI::ImageUsage::SHADER_OUTPUT;
  description.shared = false;
  swapchain->AddImageAttachment(0, description);

  float t = 0.0;
  for (int i = 0; i < 10; ++i)
  {
    if (RHI::IRenderTarget * renderTarget = swapchain->AcquireFrame())
    {
      renderTarget->SetClearValue(0, 0.1f, std::abs(std::sin(t)), 0.4f, 1.0f);
      swapchain->FlushFrame();
    }
    t += 0.001f;
  }
  return 0;
}
