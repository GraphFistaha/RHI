#include <GLFW/glfw3.h>

#include "TestUtils.hpp"

namespace RHI::test_examples
{

GlfwInstance::GlfwInstance()
{
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
}

GlfwInstance::~GlfwInstance()
{
  glfwTerminate();
}

} // namespace RHI::test_examples
