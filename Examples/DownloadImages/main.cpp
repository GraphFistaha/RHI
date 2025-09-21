#include <RHI.hpp>
#include <TestUtils.hpp>

// clang-format off
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
// clang-format on
extern RHI::ITexture * UploadTexture(const char *, RHI::IContext *, bool);
int main()
{
  RHI::GpuTraits gpuTraits{};
  std::unique_ptr<RHI::IContext> ctx = RHI::CreateContext(gpuTraits, ConsoleLog);

  auto * texture = UploadTexture("mike_wazowski.jpg", ctx.get(), false);
  RHI::TextureRegion region2Download;
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
