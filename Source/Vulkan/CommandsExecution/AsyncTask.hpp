#pragma once
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan
{
struct Context;
}

namespace RHI::vulkan
{

struct AsyncTask : public RHI::IAwaitable
{
  explicit AsyncTask(const Context & ctx);
  virtual ~AsyncTask() override;

  AsyncTask(AsyncTask && rhs) noexcept;
  AsyncTask & operator=(AsyncTask && rhs) noexcept;

public: // IAwaitable interface
  virtual bool Wait() noexcept override;

public:
  void StartTask() noexcept;
  VkSemaphore GetSemaphore() const noexcept { return m_semaphore; }
  VkFence GetFence() const noexcept { return m_fence; }

private:
  const Context & m_context;
  VkSemaphore m_semaphore = VK_NULL_HANDLE;
  VkFence m_fence = VK_NULL_HANDLE;

private:
  AsyncTask(const AsyncTask &) = delete;
  AsyncTask & operator=(const AsyncTask &) = delete;
};

} // namespace RHI::vulkan
