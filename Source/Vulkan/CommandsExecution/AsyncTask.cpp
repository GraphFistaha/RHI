#include "AsyncTask.hpp"

#include "../VulkanContext.hpp"

namespace RHI::vulkan
{

AsyncTask::AsyncTask(Context & ctx)
  : ContextualObject(ctx)
{
  m_semaphore = utils::CreateVkSemaphore(ctx.GetDevice());
  m_fence = utils::CreateFence(ctx.GetDevice(), true);
}

AsyncTask::~AsyncTask()
{
  GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_semaphore, nullptr);
  GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_fence, nullptr);
}

AsyncTask::AsyncTask(AsyncTask && rhs) noexcept
  : ContextualObject(std::move(rhs))
{
  std::swap(m_fence, rhs.m_fence);
  std::swap(m_semaphore, rhs.m_semaphore);
}

AsyncTask & AsyncTask::operator=(AsyncTask && rhs) noexcept
{
  if (this != &rhs)
  {
    ContextualObject::operator=(std::move(rhs));
    std::swap(m_fence, rhs.m_fence);
    std::swap(m_semaphore, rhs.m_semaphore);
  }
  return *this;
}

bool AsyncTask::Wait() noexcept
{
  auto res = vkWaitForFences(GetContext().GetDevice(), 1, &m_fence, VK_TRUE, UINT64_MAX);
  return res == VK_SUCCESS;
}

void AsyncTask::StartTask() noexcept
{
  Wait();
  vkResetFences(GetContext().GetDevice(), 1, &m_fence);
}


} // namespace RHI::vulkan
