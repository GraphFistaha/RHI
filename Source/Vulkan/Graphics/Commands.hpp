#pragma once
#include <atomic>

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan
{
namespace details
{
inline VkIndexType IndexType2VulkanIndexType(RHI::IndexType type)
{
  using namespace RHI;
  switch (type)
  {
    case IndexType::UINT8:
      return VkIndexType::VK_INDEX_TYPE_UINT8_EXT;
    case IndexType::UINT16:
      return VkIndexType::VK_INDEX_TYPE_UINT16;
    case IndexType::UINT32:
      return VkIndexType::VK_INDEX_TYPE_UINT32;
    default:
      throw std::runtime_error("Failed to cast IndexType to vulkan enum");
  }
}
} // namespace details


class GraphicsCommands : virtual public GraphicsCommandsContainer
{
public: // GraphicsCommandsContainer - interface
  /// @brief draw vertices command (analog glDrawArrays)
  void DrawVertices(std::uint32_t vertexCount, std::uint32_t instanceCount,
                    std::uint32_t firstVertex = 0, std::uint32_t firstInstance = 0) override
  {
    vkCmdDraw(m_buffer, vertexCount, instanceCount, firstVertex, firstInstance);
    m_commandsCount++;
  }

  /// @brief draw vertices with indieces (analog glDrawElements)
  void DrawIndexedVertices(std::uint32_t indexCount, std::uint32_t instanceCount,
                           std::uint32_t firstIndex = 0, int32_t vertexOffset = 0,
                           std::uint32_t firstInstance = 0) override
  {
    vkCmdDrawIndexed(m_buffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    m_commandsCount++;
  }

  /// @brief Set viewport command
  void SetViewport(float width, float height) override
  {
    VkViewport vp{0.0f, 0.0f, width, height, 0.0f, 1.0f};
    vkCmdSetViewport(m_buffer, 0, 1, &vp);
    m_commandsCount++;
  }

  /// @brief Set scissor command
  void SetScissor(int32_t x, int32_t y, std::uint32_t width, std::uint32_t height) override
  {
    VkRect2D scissor{};
    scissor.extent = {width, height};
    scissor.offset = {x, y};
    vkCmdSetScissor(m_buffer, 0, 1, &scissor);
    m_commandsCount++;
  }

  /// @brief binds buffer as input attribute data
  void BindVertexBuffer(std::uint32_t binding, const IBufferGPU & buffer,
                        std::uint32_t offset = 0) override
  {
    VkDeviceSize vkOffset = offset;
    VkBuffer buf = reinterpret_cast<VkBuffer>(buffer.GetHandle());
    vkCmdBindVertexBuffers(m_buffer, 0, 1, &buf, &vkOffset);
    m_commandsCount++;
  }

  /// @brief binds buffer as index buffer
  void BindIndexBuffer(const IBufferGPU & buffer, IndexType type, std::uint32_t offset = 0) override
  {
    VkBuffer buf = reinterpret_cast<VkBuffer>(buffer.GetHandle());
    vkCmdBindIndexBuffer(m_buffer, buf, VkDeviceSize{offset},
                         details::IndexType2VulkanIndexType(type));
    m_commandsCount++;
  }

public:
  void BindCommandBuffer(VkCommandBuffer buffer) { m_buffer = buffer; }
  bool HasNoCommands() const noexcept { return m_commandsCount == 0; }
  void ResetCommandsCounter() noexcept { m_commandsCount = 0; }

private:
  VkCommandBuffer m_buffer = VK_NULL_HANDLE;
  std::atomic_size_t m_commandsCount = 0;
};

} // namespace RHI::vulkan
