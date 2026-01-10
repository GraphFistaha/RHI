#include <RHI.hpp>
#include <TestUtils.hpp>

// clang-format off
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
// clang-format on
extern RHI::ITexture * UploadTexture(const char *, RHI::IContext *, bool, bool);
int main()
{
  RHI::GpuTraits gpuTraits{};
  std::unique_ptr<RHI::IContext> ctx = RHI::CreateContext(gpuTraits, ConsoleLog);

  auto * texture = UploadTexture("mike_wazowski.jpg", ctx.get(), false);
  RHI::DownloadImageArgs args{};
  args.format = RHI::HostImageFormat::RGB8;
  args.copyRegion = {{0, 0, 0}, texture->GetDescription().extent};
  args.layerIndex = 0;
  args.layersCount = 1;
  auto future = texture->DownloadImage(args);

  while (!future._Is_ready())
    ctx->TransferPass();
  auto result = future.get();
  stbi_write_bmp("downloaded_image.bmp", args.copyRegion.extent[0], args.copyRegion.extent[1], 3,
                 result.data());
  return 0;
}
