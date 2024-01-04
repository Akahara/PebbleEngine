#include "freecam.h"

#include "inputs/user_inputs.h"
#include "utils/debug.h"

Freecam::Freecam()
{
  m_camera.setProjection(pbl::PerspectiveProjection{});
  m_camera.setPosition({ 10, 10, 10 });
  m_camera.setRotation(-DirectX::XM_PI*.75f, DirectX::XM_PI*.25f);
  m_camera.updateViewMatrix();
}

void Freecam::processUserInputs(double delta)
{
  if (!m_isActive) return;

  m_inputCooldown -= static_cast<float>(delta);

  constexpr float mouseSensitivity = 1 / 1000.f;

  const pbl::MouseState mouse = pbl::UserInputs::getMouseState();
  float movement = static_cast<float>(delta) * m_playerSpeed;

  if (m_cursorLocked && (mouse.deltaX != 0 || mouse.deltaY != 0 || mouse.deltaScroll != 0)) {
    m_camera.rotate(0, static_cast<float>(mouse.deltaX) * mouseSensitivity, 0, false);
    m_camera.rotate(static_cast<float>(mouse.deltaY) * mouseSensitivity, 0, 0, true);
  }
  if (pbl::UserInputs::isKeyPressed(keys::SC_W)) m_camera.move(+movement * m_camera.getFlatForward());
  if (pbl::UserInputs::isKeyPressed(keys::SC_A)) m_camera.move(-movement * m_camera.getRight());
  if (pbl::UserInputs::isKeyPressed(keys::SC_S)) m_camera.move(-movement * m_camera.getFlatForward());
  if (pbl::UserInputs::isKeyPressed(keys::SC_D)) m_camera.move(+movement * m_camera.getRight());
  if (pbl::UserInputs::isKeyPressed(keys::SC_LEFT_SHIFT)) m_camera.move(+movement * pbl::Camera::UP);
  if (pbl::UserInputs::isKeyPressed(keys::SC_LEFT_CTRL))  m_camera.move(-movement * pbl::Camera::UP);
  if (mouse.deltaScroll != 0) {
    m_playerSpeed *= pow(1.1f, mouse.deltaScroll < 0 ? -1.f : +1.f);
    m_playerSpeed = mathf::clamp(m_playerSpeed, .1f, 1000.f);
  }
  if (pbl::UserInputs::isKeyPressed(keys::SC_F) && m_inputCooldown < 0) {
    pbl::UserInputs::setCursorLocked(m_cursorLocked = !m_cursorLocked);
    m_inputCooldown = .4f;
  }
  m_camera.updateViewMatrix();
}
