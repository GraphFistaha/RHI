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
  gpuTraits.require_presentation = true;
  std::unique_ptr<RHI::IContext> ctx = RHI::CreateContext(gpuTraits, ConsoleLog);

  auto * texture = UploadTexture("mike_wazowski.jpg", ctx.get(), false);
  RHI::DownloadImageArgs args{};
  args.format = RHI::HostImageFormat::RGB8;
  args.copyRegion = {{0, 0, 0}, texture->GetDescription().extent};
  auto future = texture->DownloadImage(args);
  ctx->TransferPass(true /*flush*/); // call with true to future will complete
  auto result = future.get();
  if (result.empty())
    throw std::runtime_error("Failed to download");
  stbi_write_bmp("downloaded_image.bmp", args.copyRegion.extent[0], args.copyRegion.extent[1], 3,
                 result.data());
  return 0;
}
