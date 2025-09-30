#pragma once
#include <mutex>

#include <CommandsExecution/AsyncTask.hpp>
#include <RHI.hpp>

namespace RHI::vulkan
{
struct CompositeAsyncTask : public RHI::IAwaitable
{
  CompositeAsyncTask() = default;
  CompositeAsyncTask(CompositeAsyncTask && rhs) noexcept;
  CompositeAsyncTask & operator=(CompositeAsyncTask && rhs) noexcept;
  void SetTasks(std::vector<IAwaitable *> && tasks);

public: // IAwaitable interface
  virtual bool Wait() noexcept override;

private:
  std::mutex m_mutex;
  std::vector<IAwaitable *> m_tasks;
};
} // namespace RHI::vulkan
