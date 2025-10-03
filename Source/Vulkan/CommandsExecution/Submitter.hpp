#pragma once

#include <CommandsExecution/AsyncTask.hpp>
#include <CommandsExecution/CommandBuffer.hpp>
#include <Device.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan::details
{
/// @brief Submits commands into queue, owns primary command buffer
struct Submitter : public CommandBuffer
{
  explicit Submitter(Context & ctx, QueueType type, VkPipelineStageFlags waitStages);
  virtual ~Submitter() override = default;
  Submitter(Submitter && rhs) noexcept;
  Submitter & operator=(Submitter && rhs) noexcept;

  AsyncTask * Submit(bool waitPrevSubmitOnGPU, std::vector<VkSemaphore> && waitSemaphores);
  void WaitForSubmitCompleted();

protected:
  VkPipelineStageFlags m_waitStages;
  QueueType m_queueType;

  AsyncTask m_oldBarrier;
  AsyncTask m_newBarrier;
  bool m_isFirstSubmit = true;
};

} // namespace RHI::vulkan::details
