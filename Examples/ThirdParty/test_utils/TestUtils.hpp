#pragma once
#include "test_utils_def.h"

#include <cstdio>
#include <string>

#include <RHI.hpp>



// Custom log function used by RHI::Context
void ConsoleLog(RHI::LogMessageStatus status, const std::string & message);


RHI::ITexture * UploadTexture(const char * path, RHI::IContext * ctx,
                                             bool with_alpha);

namespace RHI::test_examples
{
struct GlfwInstance final
{
  GlfwInstance();
  ~GlfwInstance();
};

} // namespace RHI::test_examples
