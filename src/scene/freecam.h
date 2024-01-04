#pragma once

#include "display/camera.h"

/*
 * A free camera controller, used in the development process but not
 * in the actual game.
 * #processUserInputs() does not handle the (de)activation of this camera,
 * the caller will have to handle toggling between cameras.
 */
class Freecam
{
public:
  Freecam();

  void processUserInputs(double delta);

  void setSpeed(float speed) { m_playerSpeed = speed; }
  bool &isActive() { return m_isActive; }
  void setActive(bool m_active) { m_isActive = m_active; }
  pbl::Camera &getCamera() { return m_camera; }

  // returns either the given camera or the freecam if it is active
  pbl::Camera &getActiveCamera(pbl::Camera &defaultCamera) { return m_isActive ? m_camera : defaultCamera; }
  const pbl::Camera &getActiveCamera(const pbl::Camera &defaultCamera) const { return m_isActive ? m_camera : defaultCamera; }

private:
  float       m_playerSpeed = 15.f;
  float       m_inputCooldown{};
  bool        m_cursorLocked{};
  bool        m_isActive{};
  pbl::Camera m_camera;
};
