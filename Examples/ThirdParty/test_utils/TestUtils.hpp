#pragma once
#include <cstdio>
#include <string>

#include <RHI.hpp>

namespace RHI::test_examples
{

// Custom log function used by RHI::Context
inline void ConsoleLog(RHI::LogMessageStatus status, const std::string & message)
{
  switch (status)
  {
    case RHI::LogMessageStatus::LOG_INFO:
      std::printf("INFO: - %s\n", message.c_str());
      break;
    case RHI::LogMessageStatus::LOG_WARNING:
      std::printf("WARNING: - %s\n", message.c_str());
      break;
    case RHI::LogMessageStatus::LOG_ERROR:
      std::printf("ERROR: - %s\n", message.c_str());
      break;
    case RHI::LogMessageStatus::LOG_DEBUG:
      std::printf("DEBUG: - %s\n", message.c_str());
      break;
  }
}

struct GlfwInstance final
{
  GlfwInstance();
  ~GlfwInstance();
};

} // namespace RHI::test_example
