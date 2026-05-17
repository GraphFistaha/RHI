#pragma once
#include <Private/OwnedBy.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan::details
{
struct CommandBuffer;
}
namespace RHI::vulkan
{
struct Context;
}

namespace RHI::vulkan::details
{
struct Synchronizer final : public OwnedBy<Context>
{
  explicit Synchronizer(Context & ctx, VkImage image);
  explicit Synchronizer(Context & ctx, VkBuffer buffer);
  ~Synchronizer();
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);

public:
  void RequireSynchronize(VkPipelineStageFlags2 currentStage, VkAccessFlagBits2 requiredAccess,
                          details::CommandBuffer & commands,
                          VkImageLayout requiredLayout = VK_IMAGE_LAYOUT_UNDEFINED);

  VkImageLayout GetLayout() const noexcept;
  /// @brief for external set of layout (f.e. in renderPass begin/end)
  void SetLayout(VkImageLayout layout) noexcept;

private:
  struct BarrierInfo
  {
    VkPipelineStageFlags2 currentStage;
    VkAccessFlagBits2 requiredAccess;
    VkImageLayout requiredLayout;
  };
  VkImage m_image = VK_NULL_HANDLE;   ///< synchronizable image
  VkBuffer m_buffer = VK_NULL_HANDLE; ///< synchronizable buffer

  BarrierInfo m_prevBarrier;
};
} // namespace RHI::vulkan::details
