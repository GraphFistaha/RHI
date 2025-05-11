#include "Camera.hpp"

#include <glm/ext.hpp>

namespace
{
glm::vec3 OrthogonalVector(const glm::vec3 & v1, const glm::vec3 & v2)
{
  return glm::normalize(glm::cross(v1, v2));
}
} // namespace

namespace RHI::test_examples
{
Camera::Camera(const glm::vec3 & up)
  : m_up(up)
  , m_fov(glm::radians(45.0f))
{
}

void Camera::OnCursorMoved(const glm::vec2 & delta)
{
  glm::vec2 offset = delta * m_sensetivity;
  glm::vec4 newCamDirection = glm::vec4(m_direction, 0.0);
  glm::mat4 rotYaw = glm::rotate(glm::identity<glm::mat4>(), offset.x, glm::vec3(0, 1, 0));
  newCamDirection = rotYaw * newCamDirection;
  if (glm::abs(glm::dot(m_direction, m_up) - glm::radians(89.0f)) > 0.01f)
  {
    glm::mat4 rotPitch = glm::rotate(glm::identity<glm::mat4>(), offset.y, GetRightVector());
    newCamDirection = rotPitch * newCamDirection;
  }
  m_direction = glm::normalize(newCamDirection);
}

void Camera::MoveCamera(const glm::vec3 & direction)
{
  m_position += direction * m_speed;
}

void Camera::OnResolutionChanged(const glm::vec2 & resolution)
{
  m_aspect = resolution.x / resolution.y;
  m_invalidProjection = true;
}

glm::mat4 Camera::GetViewMatrix() const noexcept
{
  return glm::lookAt(m_position, m_direction + m_position, m_up);
}

glm::mat4 Camera::GetProjectionMatrix() const noexcept
{
  if (m_invalidProjection)
  {
    m_proj = glm::perspectiveZO(m_fov, m_aspect, m_zRange.x, m_zRange.y);
    m_invalidProjection = false;
  }
  return m_proj;
}

glm::mat4 Camera::GetVP() const noexcept
{
  return GetProjectionMatrix() * GetViewMatrix();
}

glm::vec3 Camera::GetRightVector() const noexcept
{
  return -OrthogonalVector(m_up, m_direction);
}

glm::vec3 Camera::GetFrontVector() const noexcept
{
  return m_direction;
}
} // namespace RHI::test_examples
