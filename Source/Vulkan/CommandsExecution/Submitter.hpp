#pragma once

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "AsyncTask.hpp"
#include "CommandBuffer.hpp"

namespace RHI::vulkan
{
enum class QueueType : uint8_t;
struct Context;
} // namespace RHI::vulkan

namespace RHI::vulkan::details
{
/// @brief Submits commands into queue, owns primary command buffer
struct Submitter : public CommandBuffer
{
  explicit Submitter(const Context & ctx, VkQueue queue, uint32_t queueFamily,
                     VkPipelineStageFlags waitStages);
  virtual ~Submitter() override = default;
  Submitter(Submitter && rhs) noexcept;
  Submitter & operator=(Submitter && rhs) noexcept;

  AsyncTask * Submit(bool waitPrevSubmitOnGPU, std::vector<VkSemaphore> && waitSemaphores);
  void WaitForSubmitCompleted();

protected:
  VkPipelineStageFlags m_waitStages;
  uint32_t m_queueFamily;
  VkQueue m_queue;

  AsyncTask m_oldBarrier;
  AsyncTask m_newBarrier;
  bool m_isFirstSubmit = true;
};

} // namespace RHI::vulkan::details
