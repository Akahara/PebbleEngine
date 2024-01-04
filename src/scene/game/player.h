#pragma once

#include "display/camera.h"
#include "world/object.h"
#include "physics/physics.h"
#include "physics/physxlib.h"
#include "player_cameras.h"
#include "player_vehicle.h"
#include "engine/device.h"

enum class MovementState
{
  ON_GROUND,
  JUMPING,
  FALLING,
  RESETTING
};

class Player
{
public:
  static constexpr int WHEEL_NB = 4;
  inline static const physx::PxVec3 RIGHT_FRONT_WHEEL = {  PlayerVehicle::FRAME_HALF_X, -PlayerVehicle::FRAME_HALF_Y,  PlayerVehicle::FRAME_HALF_Z };
  inline static const physx::PxVec3 LEFT_FRONT_WHEEL  = {  PlayerVehicle::FRAME_HALF_X, -PlayerVehicle::FRAME_HALF_Y, -PlayerVehicle::FRAME_HALF_Z };
  inline static const physx::PxVec3 LEFT_REAR_WHEEL   = { -PlayerVehicle::FRAME_HALF_X, -PlayerVehicle::FRAME_HALF_Y, -PlayerVehicle::FRAME_HALF_Z };
  inline static const physx::PxVec3 RIGHT_REAR_WHEEL  = { -PlayerVehicle::FRAME_HALF_X, -PlayerVehicle::FRAME_HALF_Y,  PlayerVehicle::FRAME_HALF_Z };

  inline static const physx::PxVec3 LOCAL_WHEELS[] = { LEFT_FRONT_WHEEL, RIGHT_FRONT_WHEEL, LEFT_REAR_WHEEL, RIGHT_REAR_WHEEL };

  static constexpr float MAX_SPEED = 115.0f;
  static constexpr float TURNING_SPEED = 0.04f;
  static constexpr float MAX_ACCELERATION = 85.0f;
  static constexpr float BOOST_DEPLETION = 0.5f;
  static constexpr float TAU = 1.0f; // 3*tau -> 95% MAX_ACCELERATION
  static constexpr float DOWNWARD_ACCELERATION = 20.0f;

  static constexpr float AIR_CONTROL_TIME = 2.0f; // en secondes
  static constexpr float AIR_CONTROL = 20.0f; // scale in air control
  static constexpr float ROLL_INACTIVITY_TIME = 0.1f; // time for which you can't rotate in air
  static constexpr float FUEL_REGENERATION_SPEED = 1.f/3.f; // fraction of total fuel recovered per second

  // Parametres des ressorts
  // Q = sqrt(STIFFNESS * MASS) / DAMPING
  static constexpr float HOVER_HEIGHT = 2.0f;
  static constexpr float STIFFNESS = 400.0f;
  static constexpr float DAMPING = 20.0f;
  static constexpr float DOWN_ACCELERATION_SCALE = 200.0f;
  static constexpr float ANTI_ROLL = 200.0f;

  // Contrôles manette
  static constexpr float MINIMUM_JOYSTICK_SENSITIVITY = 0.1f;
  static constexpr float MINIMUM_Z_AXIS_SENSITIVITY = 0.05f;

private:
  static constexpr unsigned int MAX_RAYCAST_HIT_PER_WHEEL = 10;

  FirstPersonCameraController m_firstPersonCamera;
  ThirdPersonCameraController m_thirdPersonCamera;
  FixedCameraController       m_fixedCamera;
  CameraController           *m_activeCamera = &m_thirdPersonCamera;
  std::shared_ptr<pbl::ScreenResizeEventHandler> m_screenResizeEventHandler;

  std::pair<bool, physx::PxRaycastHit> m_wheelHitInfo[4];
  std::pair<bool, physx::PxRaycastHit> m_frameHitInfo;
  pbx::PhysicsBody m_physicsBody;
  PlayerVehicle *m_vehicle;
  physx::PxRigidDynamic *m_frame;

  float m_airTime{};
  float m_accelerationTime{};
  float m_decelerationTime{};

  MovementState m_movementState = MovementState::ON_GROUND;
  physx::PxVec3 m_resetPosition{};
  physx::PxVec3 m_resetUpVector{};
  float m_remainingFuel = 0;

  Transform m_checkpointTransform{};

  bool m_canControlVehicle = true; // becomes false when the game ends
  float m_forwardInput = 0;
  float m_turnInput = 0;
  int m_strafInput = 0;
  bool m_boosting = false;
  float m_jumpForce = 0;
  float m_jumpChrono{};
  bool m_change_air_rotation = false;

  //shouldn't be here, but it's the easiest way for now
  float m_coloredEdgesTargetAlpha{};

public:
  Player();
  ~Player() = default;

  void update(double deltaTime);
  void render();

  void setVehicleObject(PlayerVehicle *vehicleObject);
  const PlayerVehicle &getVehicle() const { return *m_vehicle; }
  PlayerVehicle &getVehicle() { return *m_vehicle; }

  void resetPosition(const Transform& p);

  const pbl::Camera &getActiveCamera() const { return m_activeCamera->getCamera(); }
  ThirdPersonCameraController &getThirdPersonCamera() { return m_thirdPersonCamera; }
  FixedCameraController &getFixedCamera() { return m_fixedCamera; }
  void setFixedCameraActive(bool active);
  void setCanControlVehicle(bool canControlVehicle) { m_canControlVehicle = canControlVehicle; }

  float getCurrentVelocity() const { return m_frame->getLinearVelocity().magnitude(); }
  float getRemainingFuel() const { return m_remainingFuel; }
  void setRemainingFuel(float fuel) { m_remainingFuel = std::min(1.f, std::max(0.f,fuel)); }
  bool isFlying() const { return m_movementState == MovementState::FALLING || m_movementState == MovementState::JUMPING; }
  void jump(float force) { m_jumpForce = force; m_jumpChrono = ROLL_INACTIVITY_TIME; m_movementState = MovementState::ON_GROUND; }

  void setColoredEdgesTargetAlpha(float alpha) { m_coloredEdgesTargetAlpha = alpha; }
  float getColoredEdgesTargetAlpha() { return m_coloredEdgesTargetAlpha; }

  void setCheckpointTransform(Transform p) { m_checkpointTransform = Transform{ p }; }

protected:
  void handleInputs(double deltaTime);
  void handleMovement(double deltaTime);
  void updateCameras(double deltaTime);

  void changeMovementState(MovementState movementState);

private:
  physx::PxQuat calculateRotationQuaternion(const physx::PxVec3 &from, const physx::PxVec3 &to);

  float airControlCurve(float time) const;
  float accelerationCurve(float time) const;

  void handleRaycasts(double deltaTime);
  void handleWheels();
  void handleAntiRoll();

  void calculateResetTransform();

  void turn(float scale);

  void groundMovement(double deltaTime);
  void jumpMovement(double deltaTime);
  void fallMovement(double deltaTime);
  void resetMovement(double deltaTime);
};
