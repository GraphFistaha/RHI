#include <ImageUtils/ImageUtils.hpp>
#include <TransferPass/TransferAlgorithm.hpp>
#include <VulkanContext.hpp>

namespace RHI::vulkan::details
{

TrasferTaskPtr GenerateMipmaps(Context & ctx, IInternalTexture & dst)
{
  // derives extent in 2
  auto extentDiv2 = [](const VkOffset3D & extent)
  {
    return VkOffset3D{std::max(1, extent.x / 2), std::max(1, extent.y / 2),
                      std::max(1, extent.z / 2)};
  };

  // lambda to make a barrier for mip level
  auto transferLayoutForMipLevel = [&dst](details::CommandBuffer & commands,
                                          VkImageLayout oldLayout, VkImageLayout newLayout,
                                          uint32_t level)
  {
    assert(oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL ||
           oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    assert(newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL ||
           newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    VkImageMemoryBarrier barrier{};
    {
      barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      barrier.oldLayout = oldLayout;
      barrier.newLayout = newLayout;
      barrier.srcQueueFamilyIndex = commands.GetBoundQueueFamily();
      barrier.dstQueueFamilyIndex = commands.GetBoundQueueFamily();
      barrier.image = dst.GetHandle();
      barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      barrier.subresourceRange.baseMipLevel = level;
      barrier.subresourceRange.levelCount = 1;
      barrier.subresourceRange.baseArrayLayer = 0;
      barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
      barrier.srcAccessMask = newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
                              ? VK_ACCESS_TRANSFER_WRITE_BIT
                              : VK_ACCESS_TRANSFER_READ_BIT;
      barrier.dstAccessMask = newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
                              ? VK_ACCESS_TRANSFER_READ_BIT
                              : VK_ACCESS_TRANSFER_WRITE_BIT;
    }
    commands.PushCommand(vkCmdPipelineBarrier, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
  };

  // if texture has no mip levels, then do nothing
  if (dst.GetMipLevelsCount() <= 1)
  {
    return nullptr;
  }


  auto command = [=, &dst](details::CommandBuffer & commands)
  {
    /*
                   Algorithm description:
                   Given an texture with N layers and M mip levels to generate.
                   you should generate all mip levels for each layer

                   1) transfer layout to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL for all layers/mipLevels
                   2) for i = 1 to M:
                        2.1) transfer layout to VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL for i - 1 mip level.
                               This step blocks mip level for reading
                        2.2) blit image (all N layers) from i - 1 to i mip level with linear filteration.
                               Note: i'th level has only half of i-1'th level's extent
                        2.3) transfer layout to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL for i - 1 mip level
                               this step waits for reading is completed and blocks for writing
                        2.4) div extent in 2
                 */

    // help variables for algorithm
    VkExtent3D extent = dst.GetInternalExtent();
    VkOffset3D oldMipExtent = {static_cast<int32_t>(extent.width),
                               static_cast<int32_t>(extent.height),
                               static_cast<int32_t>(extent.depth)};
    VkOffset3D mipExtent = extentDiv2(oldMipExtent);

    dst.GetSynchronizer().RequireSynchronize(VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                                             VK_ACCESS_2_TRANSFER_WRITE_BIT, commands,
                                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    for (uint32_t level = 1; level < dst.GetMipLevelsCount(); ++level)
    {
      VkImageBlit blit{};
      {
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = oldMipExtent;
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = level - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = dst.GetLayersCount();
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = mipExtent;
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = level;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = dst.GetLayersCount();
      }

      transferLayoutForMipLevel(commands, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, level - 1);

      commands.PushCommand(vkCmdBlitImage, dst.GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           dst.GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit,
                           VK_FILTER_LINEAR);

      transferLayoutForMipLevel(commands, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, level - 1);

      oldMipExtent = mipExtent;
      mipExtent = extentDiv2(mipExtent);
    }
  };


  return std::make_shared<TransferTask>(ctx, std::move(command), nullptr);
}

TrasferTaskPtr GenerateMipmapsByRegions(Context & ctx, IInternalTexture & dst,
                                        std::span<const RHI::TextureRegion> regions)
{
  auto calcMipOffset = [](const VkOffset3D & srcOffset, const VkExtent3D & srcExtent,
                          uint32_t layer) -> std::array<VkOffset3D, 2>
  {
    VkOffset3D offset0{srcOffset.x >> layer, srcOffset.y >> layer, srcOffset.z >> layer};
    VkOffset3D offset1 = {offset0.x + std::max(1, static_cast<int32_t>(srcExtent.width >> layer)),
                          offset0.y + std::max(1, static_cast<int32_t>(srcExtent.height >> layer)),
                          offset0.z + std::max(1, static_cast<int32_t>(srcExtent.depth >> layer))};
    return {offset0, offset1};
  };

  // lambda to make a barrier for mip level
  auto transferLayoutForMipLevel = [&dst](details::CommandBuffer & commands,
                                          VkImageLayout oldLayout, VkImageLayout newLayout,
                                          uint32_t level)
  {
    assert(oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL ||
           oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    assert(newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL ||
           newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    VkImageMemoryBarrier barrier{};
    {
      barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      barrier.oldLayout = oldLayout;
      barrier.newLayout = newLayout;
      barrier.srcQueueFamilyIndex = commands.GetBoundQueueFamily();
      barrier.dstQueueFamilyIndex = commands.GetBoundQueueFamily();
      barrier.image = dst.GetHandle();
      barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      barrier.subresourceRange.baseMipLevel = level;
      barrier.subresourceRange.levelCount = 1;
      barrier.subresourceRange.baseArrayLayer = 0;
      barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
      barrier.srcAccessMask = newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
                              ? VK_ACCESS_TRANSFER_WRITE_BIT
                              : VK_ACCESS_TRANSFER_READ_BIT;
      barrier.dstAccessMask = newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
                              ? VK_ACCESS_TRANSFER_READ_BIT
                              : VK_ACCESS_TRANSFER_WRITE_BIT;
    }
    commands.PushCommand(vkCmdPipelineBarrier, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
  };

  // if texture has no mip levels, then do nothing
  if (dst.GetMipLevelsCount() <= 1)
  {
    return nullptr;
  }

  auto command = [=, &dst](details::CommandBuffer & commands)
  {
    /*
                                      Algorithm description:
                                      Given an texture with N layers and M mip levels to generate.
                                      you should generate all mip levels for each layer
                                      Also given Regions array to create mip levels from

                                      1) transfer layout to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL for all layers/mipLevels
                                      2) for i = 1 to M:
                                           2.1) transfer layout to VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL for i - 1 mip level.
                                                  This step blocks mip level for reading
                                           2.2) for j = 1 to R:
                                               2.2.1) blit subimage from i - 1 to i mip level with linear filteration.
                                                      Note: i'th level has only half of i-1'th level's extent
                                           2.3) transfer layout to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL for i - 1 mip level
                                                  this step waits for reading is completed and blocks for writing
                    */

    dst.GetSynchronizer().RequireSynchronize(VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                                             VK_ACCESS_2_TRANSFER_WRITE_BIT, commands,
                                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    std::vector<VkImageBlit> blits;
    blits.reserve(regions.size());
    for (uint32_t level = 1; level < dst.GetMipLevelsCount(); ++level)
    {
      blits.clear();

      for (auto && region : regions)
      {
        auto [srcExtent, layersCount] =
          utils::UnpackExtentAndLayers(region.extent, dst.GetImageType());
        auto [srcOffset, baseLayer] =
          utils::UnpackOffsetAndBaseLayer(region.offset, dst.GetImageType());
        auto [oldMipOffset0, oldMipOffset1] = calcMipOffset(srcOffset, srcExtent, level - 1);
        auto [mipOffset0, mipOffset1] = calcMipOffset(srcOffset, srcExtent, level);

        // probably it's need to filter double copying
        //if (oldMipOffset1.x - oldMipOffset0.x == 1 && oldMipOffset1.y - oldMipOffset0.y == 1 &&
        //    oldMipOffset1.z - oldMipOffset0.z == 1)
        //  continue;

        VkImageBlit blit{};
        {
          blit.srcOffsets[0] = oldMipOffset0;
          blit.srcOffsets[1] = oldMipOffset1;
          blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
          blit.srcSubresource.mipLevel = level - 1;
          blit.srcSubresource.baseArrayLayer = baseLayer;
          blit.srcSubresource.layerCount = layersCount;
          blit.dstOffsets[0] = mipOffset0;
          blit.dstOffsets[1] = mipOffset1;
          blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
          blit.dstSubresource.mipLevel = level;
          blit.dstSubresource.baseArrayLayer = baseLayer;
          blit.dstSubresource.layerCount = layersCount;
        }
        blits.push_back(blit);
      }

      if (!blits.empty())
      {
        transferLayoutForMipLevel(commands, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                  VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, level - 1);

        commands.PushCommand(vkCmdBlitImage, dst.GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                             dst.GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                             static_cast<uint32_t>(blits.size()), blits.data(), VK_FILTER_LINEAR);

        transferLayoutForMipLevel(commands, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, level - 1);
      }
    }
  };


  return std::make_shared<TransferTask>(ctx, std::move(command), nullptr);
}
} // namespace RHI::vulkan::details
