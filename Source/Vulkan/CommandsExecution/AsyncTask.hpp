#pragma once
#include <OwnedBy.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan
{
struct Context;
}

namespace RHI::vulkan
{

struct AsyncTask : public RHI::IAwaitable,
                   public OwnedBy<Context>
{
  explicit AsyncTask(Context & ctx);
  virtual ~AsyncTask() override;

  AsyncTask(AsyncTask && rhs) noexcept;
  AsyncTask & operator=(AsyncTask && rhs) noexcept;

public: // IAwaitable interface
  virtual bool Wait() noexcept override;

public:
  void StartTask() noexcept;
  VkSemaphore GetSemaphore() const noexcept { return m_semaphore; }
  VkFence GetFence() const noexcept { return m_fence; }
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);

private:
  VkSemaphore m_semaphore = VK_NULL_HANDLE;
  VkFence m_fence = VK_NULL_HANDLE;

private:
  AsyncTask(const AsyncTask &) = delete;
  AsyncTask & operator=(const AsyncTask &) = delete;
};

} // namespace RHI::vulkan
