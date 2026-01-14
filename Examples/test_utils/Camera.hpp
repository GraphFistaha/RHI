#pragma once
#include <glm/glm.hpp>

namespace RHI::test_examples
{

class Camera final
{
public:
  explicit Camera(const glm::vec3 & up);
  void OnCursorMoved(const glm::vec2 & delta);
  void MoveCamera(const glm::vec3 & offset);
  void SetSpeed(float speed) { m_speed = speed; }
  void SetSensetivity(float sensetivity) { m_sensetivity = sensetivity; }
  void SetAspectRatio(float ratio);

  glm::mat4 GetViewMatrix() const noexcept;
  glm::mat4 GetProjectionMatrix() const noexcept;
  glm::mat4 GetVP() const noexcept;
  glm::vec3 GetRightVector() const noexcept;
  glm::vec3 GetFrontVector() const noexcept;

private:
  const glm::vec3 m_up;
  float m_fov;
  float m_speed = 0.05f;
  float m_sensetivity = 0.01f;
  glm::vec2 m_zRange{0.01f, 100.0f};
  float m_aspect = 1.0f;

  glm::vec3 m_position{0.0f};
  glm::vec3 m_direction{0.0, 0.0, 1.0f};

  mutable glm::mat4 m_proj;
  mutable bool m_invalidProjection = true;
};

} // namespace RHI::test_examples
