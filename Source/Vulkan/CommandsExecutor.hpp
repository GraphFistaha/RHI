#pragma once

#include "VulkanContext.hpp"

struct CommandsExecutor
{
  void BeginExecute(); // creates writing buffer
  void EndExecute(); // moves writing buffer into commited buffer (thread safety)
  void CancelExecute(); // destroyes writing-buffer

  // Here a lot of functions from OpenGL which are writes commands into buffer


private:
  CommandBuffer writing_buffer;
  CommandBuffer commited_buffer;
};
