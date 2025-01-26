#include "AsyncTask.hpp"

#include "../VulkanContext.hpp"

namespace RHI::vulkan
{

AsyncTask::AsyncTask(const Context & ctx)
  : m_context(ctx)
{
  m_semaphore = utils::CreateVkSemaphore(ctx.GetDevice());
  m_fence = utils::CreateFence(ctx.GetDevice(), true);
}

AsyncTask::~AsyncTask()
{
  m_context.GetGarbageCollector().PushVkObjectToDestroy(m_semaphore, nullptr);
  m_context.GetGarbageCollector().PushVkObjectToDestroy(m_fence, nullptr);
}

AsyncTask::AsyncTask(AsyncTask && rhs) noexcept
  : m_context(rhs.m_context)
{
  std::swap(m_fence, rhs.m_fence);
  std::swap(m_semaphore, rhs.m_semaphore);
}

AsyncTask & AsyncTask::operator=(AsyncTask && rhs) noexcept
{
  if (this != &rhs && &m_context == &rhs.m_context)
  {
    std::swap(m_fence, rhs.m_fence);
    std::swap(m_semaphore, rhs.m_semaphore);
  }
  return *this;
}

bool AsyncTask::Wait() noexcept
{
  auto res = vkWaitForFences(m_context.GetDevice(), 1, &m_fence, VK_TRUE, UINT64_MAX);
  return res == VK_SUCCESS;
}

void AsyncTask::StartTask() noexcept
{
  Wait();
  vkResetFences(m_context.GetDevice(), 1, &m_fence);
}


} // namespace RHI::vulkan
