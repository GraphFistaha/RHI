#include "CompositeAsyncTask.hpp"

#include <numeric>

namespace RHI::vulkan
{
CompositeAsyncTask::CompositeAsyncTask(CompositeAsyncTask && rhs) noexcept
{
  std::lock_guard lk{rhs.m_mutex};
  std::swap(m_tasks, rhs.m_tasks);
}

CompositeAsyncTask & CompositeAsyncTask::operator=(CompositeAsyncTask && rhs) noexcept
{
  if (this != &rhs)
  {
    std::lock_guard lk{rhs.m_mutex};
    std::swap(m_tasks, rhs.m_tasks);
  }
  return *this;
}

void CompositeAsyncTask::SetTasks(std::vector<IAwaitable *> && tasks)
{
  std::lock_guard lk{m_mutex};
  m_tasks.insert(m_tasks.end(), tasks.begin(), tasks.end());
}

bool CompositeAsyncTask::Wait() noexcept
{
  std::lock_guard lk{m_mutex};
  bool res = std::accumulate(m_tasks.begin(), m_tasks.end(), true,
                             [](bool acc, auto && task) { return acc && task->Wait(); });
  m_tasks.clear();
  return res;
}

} // namespace RHI::vulkan
