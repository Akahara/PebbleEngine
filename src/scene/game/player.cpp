#include "player.h"

#include "player_vehicle.h"
#include "inputs/user_inputs.h"
#include "physics/structures.h"
#include "utils/debug.h"
#include "game_vfx.h"
#include <algorithm>

using namespace physx;
using namespace pbl;

Player::Player()
  : m_frame(nullptr)
  , m_vehicle(nullptr)
{
  Device::addWindowResizeEventHandler(m_screenResizeEventHandler = std::make_shared<ScreenResizeEventHandler>([this](int, int) {
    m_firstPersonCamera.updateProjection();
    m_thirdPersonCamera.updateProjection();
    m_fixedCamera.updateProjection();
  }));
}

void Player::setVehicleObject(PlayerVehicle *vehicleObject)
{
  m_frame = vehicleObject->getFrame();
  m_vehicle = vehicleObject;
  m_thirdPersonCamera.setVehicle(vehicleObject);
  m_firstPersonCamera.setVehicle(vehicleObject);
  m_fixedCamera.setVehicle(vehicleObject);
}

void Player::calculateResetTransform()
{
  PxVec3 forwardVector = m_frame->getGlobalPose().rotate(PxVec3{ 1.0f, 0.0f, 0.0f });
  PxVec3 rightVector = m_frame->getGlobalPose().rotate(PxVec3{ 0.0f, 0.0f, 1.0f });
  PxVec3 upVector = m_frame->getGlobalPose().rotate(PxVec3{ 0.0f, 1.0f, 0.0f });

  PxVec3 directions[] = { forwardVector, rightVector, upVector, -forwardVector, -rightVector, -upVector };

  for (PxVec3 direction : directions) {
    PxRaycastHit hitsBuffer[MAX_RAYCAST_HIT_PER_WHEEL];
    PxRaycastBuffer hit = PxRaycastBuffer(hitsBuffer, MAX_RAYCAST_HIT_PER_WHEEL);
    bool didHitGround = m_frame->getScene()->raycast(
      m_frame->getGlobalPose().p,
      direction,
      HOVER_HEIGHT,
      hit,
      PxHitFlag::eDEFAULT,
      PxQueryFilterData{}
    );

    if (didHitGround) {
      for (PxU32 j = 0; j < hit.getNbAnyHits(); j++) {
        WorldObject *object = static_cast<WorldObject *>(pbx::filterDataToObjectPtr(hit.getAnyHit(j).shape->getQueryFilterData()));
        if (object != nullptr && object->getLayer() != Layer::TRIGGER && object->getLayer() != Layer::PLAYER) {
          m_resetPosition = hit.getAnyHit(j).position - direction * HOVER_HEIGHT;
          m_resetUpVector = hit.getAnyHit(j).normal;
          changeMovementState(MovementState::RESETTING);
          return;
        }
      }
    }
  }
}

void Player::resetPosition(const Transform &p)
{
  Transform q{ p.position, {1,1,1,1}, p.rotation };
  m_frame->setGlobalPose(pbx::scene2physicsTransform(q));
  m_frame->setAngularVelocity(PxZERO{});
  m_frame->setLinearVelocity(PxZERO{});
  m_vehicle->setTransform(q);
  m_firstPersonCamera.resetPosition();
  m_thirdPersonCamera.resetPosition();
  m_fixedCamera.resetPosition();
}

void Player::setFixedCameraActive(bool active)
{
  if(active) {
    m_fixedCamera.resetPosition();
    m_activeCamera = &m_fixedCamera;
  } else if(m_activeCamera == &m_fixedCamera) {
    m_activeCamera = &m_thirdPersonCamera;
  }
}

void Player::render()
{
  // debug metrics
  logs::physics.beginWindow();
  ImGui::BeginDisabled();
  float vel = m_frame->getLinearVelocity().magnitude();
  ImGui::DragFloat("Linear velocity", &vel);
  ImGui::EndDisabled();
  ImGui::End();
}

void Player::update(double deltaTime)
{
  handleInputs(deltaTime);
  handleMovement(deltaTime);
  updateCameras(deltaTime);
}

PxQuat Player::calculateRotationQuaternion(const PxVec3 &from, const PxVec3 &to)
{
  // Normaliser les vecteurs d'origine et de destination.
  PxVec3 normalizedFrom = from.getNormalized();
  PxVec3 normalizedTo = to.getNormalized();

  // Calculer l'axe de rotation en faisant le produit vectoriel entre les deux vecteurs.
  PxVec3 axis = normalizedFrom.cross(normalizedTo);

  // Calculer l'angle de rotation en utilisant le produit scalaire.
  float dotProduct = normalizedFrom.dot(normalizedTo);
  float angle = std::acos(dotProduct);

  // Convertir l'axe et l'angle en quaternion.
  PxQuat result{ angle, axis };

  return result;
}

float Player::airControlCurve(float time) const
{
  return std::clamp(AIR_CONTROL_TIME - time, 0.0f, 1.0f);
}

float Player::accelerationCurve(float time) const
{
  return MAX_ACCELERATION * (1 - std::exp(-TAU * time));
}

void Player::handleInputs(double deltaTime)
{
  if (!m_canControlVehicle)
    return;

  m_forwardInput = 0;
  m_turnInput = 0;
  m_strafInput = 0;
  m_boosting = false;

  logs::scene.logm("m_chrono = ", std::to_string(m_jumpChrono));

  if (UserInputs::hasController())
  {
      if (m_movementState == MovementState::FALLING || m_movementState == MovementState::JUMPING)
          m_forwardInput = (abs(UserInputs::getLeftStickYAxis()) > MINIMUM_JOYSTICK_SENSITIVITY ? UserInputs::getLeftStickYAxis() : 0);
      else
          m_forwardInput = (abs(UserInputs::getZAxis()) > MINIMUM_Z_AXIS_SENSITIVITY ? UserInputs::getZAxis() : 0);
      m_turnInput = -(abs(UserInputs::getLeftStickXAxis()) > MINIMUM_JOYSTICK_SENSITIVITY ? UserInputs::getLeftStickXAxis() : 0);
  }

  if (UserInputs::isKeyPressed(keys::SC_W) || ((UserInputs::isDPadPressed(keys::SC_DPAD_UP) || UserInputs::isDPadPressed(keys::SC_DPAD_UP_LEFT) || UserInputs::isDPadPressed(keys::SC_DPAD_UP_RIGHT)) && (m_movementState == MovementState::FALLING || m_movementState == MovementState::JUMPING)))
    m_forwardInput = 1;
  if (UserInputs::isKeyPressed(keys::SC_A) || UserInputs::isDPadPressed(keys::SC_DPAD_LEFT) || UserInputs::isDPadPressed(keys::SC_DPAD_UP_LEFT) || UserInputs::isDPadPressed(keys::SC_DPAD_DOWN_LEFT))
    m_turnInput = 1;
  if (UserInputs::isKeyPressed(keys::SC_S) || ((UserInputs::isDPadPressed(keys::SC_DPAD_DOWN) || UserInputs::isDPadPressed(keys::SC_DPAD_DOWN_LEFT) || UserInputs::isDPadPressed(keys::SC_DPAD_DOWN_RIGHT)) && (m_movementState == MovementState::FALLING || m_movementState == MovementState::JUMPING)))
    m_forwardInput = -1;
  if (UserInputs::isKeyPressed(keys::SC_D) || UserInputs::isDPadPressed(keys::SC_DPAD_RIGHT) || UserInputs::isDPadPressed(keys::SC_DPAD_UP_RIGHT) || UserInputs::isDPadPressed(keys::SC_DPAD_DOWN_RIGHT))
    m_turnInput = -1;
  if (UserInputs::isKeyPressed(keys::SC_LEFT_ARROW) || UserInputs::isButtonPressed(keys::SC_L1))
    --m_strafInput;
  if (UserInputs::isKeyPressed(keys::SC_RIGHT_ARROW) || UserInputs::isButtonPressed(keys::SC_R1))
    ++m_strafInput;
  if (UserInputs::consumeKeyPress(keys::SC_BACKSPACE) || UserInputs::consumeButtonPress(keys::SC_TRIANGLE)) {
    //calculateResetTransform();
    //changeMovementState(MovementState::RESETTING);
    resetPosition(m_checkpointTransform);
  }
  if (UserInputs::consumeKeyPress(keys::SC_F) || UserInputs::consumeButtonPress(keys::SC_L3))
    m_activeCamera = m_activeCamera == &m_firstPersonCamera ? static_cast<CameraController*>(&m_thirdPersonCamera) : &m_firstPersonCamera;
  if (UserInputs::isKeyPressed(keys::SC_SPACE) || UserInputs::isButtonPressed(keys::SC_CIRCLE) || UserInputs::isButtonPressed(keys::SC_CROSS)) {
    m_remainingFuel -= static_cast<float>(deltaTime);
    m_boosting = m_remainingFuel > 0.0f;
    m_remainingFuel = std::max(0.f, m_remainingFuel);
  }
  m_change_air_rotation = UserInputs::isKeyPressed(keys::SC_LEFT_SHIFT) || UserInputs::isButtonPressed(keys::SC_SQUARE);
}

void Player::handleMovement(double deltaTime)
{
    m_jumpChrono -= static_cast<float>(deltaTime);
  switch (m_movementState) {
  case MovementState::ON_GROUND: groundMovement(deltaTime); break;
  case MovementState::JUMPING: jumpMovement(deltaTime); break;
  case MovementState::FALLING: fallMovement(deltaTime); break;
  case MovementState::RESETTING: resetMovement(deltaTime); break;
  }
}

void Player::updateCameras(double delta)
{
  m_thirdPersonCamera.update(delta);
  m_firstPersonCamera.update(delta);
  m_fixedCamera.update(delta);
}

void Player::changeMovementState(MovementState movementState)
{
  if (m_movementState == movementState) return;

  switch (movementState) {
  case MovementState::ON_GROUND: 
    break;
  case MovementState::JUMPING:
    if (m_movementState == MovementState::FALLING)
      break;
    m_airTime = 0.0f;
    break;
  case MovementState::FALLING: 
    break;
  case MovementState::RESETTING: 
    m_frame->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, true);
    m_frame->setAngularVelocity(PxZERO{});
    m_frame->setLinearVelocity(PxZERO{});
    m_firstPersonCamera.resetPosition();
    m_thirdPersonCamera.resetPosition();
    m_fixedCamera.resetPosition();
    break;
  }

  m_movementState = movementState;
}

void Player::turn(float scale)
{
  physx::PxTransform currentTransform = m_frame->getGlobalPose();
  PxVec3 upVector = currentTransform.rotate(PxVec3{ 0.0f, 1.0f, 0.0f });

  physx::PxQuat rotation = physx::PxQuat(scale, upVector);
  currentTransform.q = rotation * currentTransform.q;
  m_frame->setGlobalPose(currentTransform);
}

void Player::groundMovement(double deltaTime)
{
  PxVec3 forwardVector = m_frame->getGlobalPose().rotate(PxVec3{ 1.0f, 0.0f, 0.0f });
  PxVec3 rightVector = m_frame->getGlobalPose().rotate(PxVec3{ 0.0f, 0.0f, 1.0f });
  PxVec3 upVector = m_frame->getGlobalPose().rotate(PxVec3{ 0.0f, 1.0f, 0.0f });

  // Application des inputs
  if (m_forwardInput > 0) {
    m_accelerationTime += static_cast<float>(deltaTime);
    m_decelerationTime -= static_cast<float>(deltaTime);
  } else if (m_forwardInput < 0) {
    m_accelerationTime -= static_cast<float>(deltaTime);
    m_decelerationTime += static_cast<float>(deltaTime);
  } else {
    m_accelerationTime -= static_cast<float>(deltaTime);
    m_decelerationTime -= static_cast<float>(deltaTime);
  }

  m_accelerationTime = std::clamp(m_accelerationTime, 0.0f, 5 * TAU);
  m_decelerationTime = std::clamp(m_decelerationTime, 0.0f, 5 * TAU);

  float accelerationScale = accelerationCurve(m_accelerationTime) * (m_boosting ? 2.0f : 1.0f);
  float decelerationScale = accelerationCurve(m_decelerationTime) * 0.9f;

  if (m_forwardInput > 0)
    m_frame->addForce(forwardVector * accelerationScale, PxForceMode::eACCELERATION);
  else if (m_forwardInput < 0)
    m_frame->addForce(-forwardVector * decelerationScale, PxForceMode::eACCELERATION);

  m_frame->addForce(rightVector * MAX_ACCELERATION * static_cast<float>(m_strafInput), PxForceMode::eACCELERATION);
  turn(TURNING_SPEED * m_turnInput);

  // TODO: a ameliorer pour le straffing
  forwardVector = m_frame->getGlobalPose().rotate(PxVec3{ 1.0f, 0.0f, 0.0f });
  PxVec3 currentVelocity = m_frame->getLinearVelocity();
  float scalaire = forwardVector.dot(currentVelocity);
  if (m_jumpChrono < 0)
  {
    if (m_turnInput && scalaire > 0) m_frame->setLinearVelocity(forwardVector * currentVelocity.magnitude());
    else if (m_turnInput && scalaire < 0) m_frame->setLinearVelocity(-forwardVector * currentVelocity.magnitude());
  }
  else 
  {
    // Force de jumpPads
    m_frame->addForce(PxVec3{ 0.f, m_jumpForce, 0.f }, PxForceMode::eIMPULSE);
    m_jumpForce = 0;
  }

  // Force de frottement de l'air
  m_frame->addTorque(-m_frame->getAngularVelocity() * 10.0f, PxForceMode::eACCELERATION);

  // Permet au vehicule de retomber plus vite
  m_frame->addForce({ 0.0f, -9.8f, 0.0f }, PxForceMode::eACCELERATION);

  handleRaycasts(deltaTime);
  handleWheels();

  // Stick to ground the faster the vehicle is
  PxTransform globalTransform = m_frame->getGlobalPose();

  float downAccelerationAmount = m_frame->getLinearVelocity().magnitude() / MAX_SPEED;
  m_frame->addForce(-upVector * DOWN_ACCELERATION_SCALE * downAccelerationAmount, PxForceMode::eACCELERATION);
}

void Player::jumpMovement(double deltaTime)
{
  m_airTime += static_cast<float>(deltaTime);

  if (m_airTime > AIR_CONTROL_TIME) {
    changeMovementState(MovementState::FALLING);
    fallMovement(deltaTime);
    return;
  }

  PxVec3 forwardVector = m_frame->getGlobalPose().rotate(PxVec3{ 1.0f, 0.0f, 0.0f });
  PxVec3 rightVector = m_frame->getGlobalPose().rotate(PxVec3{ 0.0f, 0.0f, 1.0f });
  PxVec3 upVector = m_frame->getGlobalPose().rotate(PxVec3{ 0.0f, 1.0f, 0.0f });

  // Application des inputs
  float controlScale = airControlCurve(m_airTime);
  if (m_jumpChrono < 0.f)
  {
      m_frame->addTorque((m_change_air_rotation ? -forwardVector : upVector) * AIR_CONTROL * static_cast<float>(m_turnInput), PxForceMode::eACCELERATION);
      m_frame->addTorque((GameVFX::getSettings().invertYAxis ? -1.f : 1.f) * rightVector * AIR_CONTROL * static_cast<float>(m_forwardInput), PxForceMode::eACCELERATION);
  }
  m_frame->addForce(rightVector * MAX_ACCELERATION * controlScale * static_cast<float>(m_strafInput), PxForceMode::eACCELERATION);
  if (m_boosting)
    m_frame->addForce(forwardVector * MAX_ACCELERATION, PxForceMode::eACCELERATION);

  // Force de frottement de l'air
  m_frame->addTorque(-m_frame->getAngularVelocity() * 10.0f, PxForceMode::eACCELERATION);

  // Permet au vehicule de retomber plus vite
  m_frame->addForce({ 0.0f, -9.8f, 0.0f }, PxForceMode::eACCELERATION);

  handleRaycasts(deltaTime);
}

void Player::fallMovement(double deltaTime)
{
  PxVec3 forwardVector = m_frame->getGlobalPose().rotate(PxVec3{ 1.0f, 0.0f, 0.0f });
  PxVec3 rightVector = m_frame->getGlobalPose().rotate(PxVec3{ 0.0f, 0.0f, 1.0f });
  PxVec3 upVector = m_frame->getGlobalPose().rotate(PxVec3{ 0.0f, 1.0f, 0.0f });

  // Application des inputs
  if (m_jumpChrono < 0.f)
  {
      m_frame->addTorque( (m_change_air_rotation ? -forwardVector : upVector) * AIR_CONTROL * static_cast<float>(m_turnInput), PxForceMode::eACCELERATION);
      m_frame->addTorque((GameVFX::getSettings().invertYAxis ? -1.f : 1.f) * rightVector * AIR_CONTROL * static_cast<float>(m_forwardInput), PxForceMode::eACCELERATION);
  }
  if (m_boosting)
    m_frame->addForce(forwardVector * MAX_ACCELERATION, PxForceMode::eACCELERATION);

  // Force de frottement de l'air
  m_frame->addTorque(-m_frame->getAngularVelocity() * 10.0f, PxForceMode::eACCELERATION);

  // Permet au vehicule de retomber plus vite
  m_frame->addForce({ 0.0f, -9.8f, 0.0f }, PxForceMode::eACCELERATION);

  handleRaycasts(deltaTime);
}

void Player::resetMovement(double deltaTime)
{
  static double duration = 0.0;
  duration += deltaTime;

  if (duration > 1.0) {
    duration = 0.0;
    m_frame->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, false);
    m_movementState = MovementState::ON_GROUND;
    return;
  }

  PxVec3 upVector = m_frame->getGlobalPose().rotate(PxVec3{ 0.0f, 1.0f, 0.0f });

  PxVec3 pos = m_frame->getGlobalPose().p;
  PxQuat quat = m_frame->getGlobalPose().q;

  PxQuat rotationVersLeSol = calculateRotationQuaternion(upVector, m_resetUpVector);
  PxQuat nouvelleOrientation = rotationVersLeSol * quat;
  nouvelleOrientation.normalize();

  if (duration < 0.5)
    pos = mathf::lerp(pos, m_resetPosition, .1f);
  else
    quat = PxSlerp(0.1f, quat, nouvelleOrientation);

  m_frame->setGlobalPose({ pos, quat });
}

void Player::handleRaycasts(double deltaTime)
{
  PxTransform globalTransform = m_frame->getGlobalPose();
  PxVec3 upVector = globalTransform.rotate(PxVec3{ 0.0f, 1.0f, 0.0f });

  bool anyWheelAboveBoostPlate = false;

  for (int i = 0; i < WHEEL_NB; ++i) {
    PxVec3 globalCorner = globalTransform.transform(LOCAL_WHEELS[i]);

    PxRaycastHit hitsBuffer[MAX_RAYCAST_HIT_PER_WHEEL];
    PxRaycastBuffer hit = PxRaycastBuffer(hitsBuffer, MAX_RAYCAST_HIT_PER_WHEEL);
    bool didHitGround = m_frame->getScene()->raycast(
      globalCorner,
      -upVector,
      HOVER_HEIGHT,
      hit,
      PxHitFlag::eDEFAULT,
      PxQueryFilterData{}
    );

    m_wheelHitInfo[i].first = false;

    // On regarde si au moins un des objets touchés par le raycast n'est PAS un trigger :
    if (didHitGround) {
      for (PxU32 j = 0; j < hit.getNbAnyHits(); j++) {
        WorldObject *object = static_cast<WorldObject *>(pbx::filterDataToObjectPtr(hit.getAnyHit(j).shape->getQueryFilterData()));
        anyWheelAboveBoostPlate |= object != nullptr && object->getLayer() == Layer::BOOST_PLATE;
        if (object != nullptr
          && object->getLayer() != Layer::TRIGGER 
          && object->getLayer() != Layer::PLAYER
          && (!m_wheelHitInfo[i].first || hit.getAnyHit(j).distance < m_wheelHitInfo[i].second.distance))
        {
          m_wheelHitInfo[i] = { true, hit.getAnyHit(j) };
        }
      }
    }
  }

  PxRaycastHit hitsBuffer[MAX_RAYCAST_HIT_PER_WHEEL];
  PxRaycastBuffer hit = PxRaycastBuffer(hitsBuffer, MAX_RAYCAST_HIT_PER_WHEEL);
  bool didHitGround = m_frame->getScene()->raycast(
    globalTransform.p - PxVec3{ 0.0f, PlayerVehicle::FRAME_HALF_Y, 0.0f },
    -upVector,
    HOVER_HEIGHT,
    hit,
    PxHitFlag::eDEFAULT,
    PxQueryFilterData{}
  );

  m_frameHitInfo.first = false;
  bool flying = true;

  // On regarde si au moins un des objets touchés par le raycast n'est PAS un trigger :
  if (didHitGround) {
    for (PxU32 j = 0; j < hit.getNbAnyHits(); j++) {
      WorldObject *object = static_cast<WorldObject *>(pbx::filterDataToObjectPtr(hit.getAnyHit(j).shape->getQueryFilterData()));
      if (object != nullptr && object->getLayer() != Layer::TRIGGER && object->getLayer() != Layer::PLAYER) {
        m_frameHitInfo = { true, hit.getAnyHit(j) };
        flying = false;
        changeMovementState(MovementState::ON_GROUND);
        break;
      }
    }
  }

  if (anyWheelAboveBoostPlate) {
    m_remainingFuel = std::min(m_remainingFuel + static_cast<float>(deltaTime) * FUEL_REGENERATION_SPEED, 1.f);
  }

  if (flying) {
      changeMovementState(MovementState::JUMPING);
  }
}

void Player::handleWheels()
{
  if (m_movementState != MovementState::ON_GROUND) return;

  // Check if the vehicle need to stick to the road
  bool shouldStick = false;
  WorldObject *object = static_cast<WorldObject *>(pbx::filterDataToObjectPtr(m_frameHitInfo.second.shape->getQueryFilterData()));
  if (object->getLayer() == Layer::STICKY_ROAD) shouldStick = true;

  PxTransform globalTransform = m_frame->getGlobalPose();
  PxVec3 upVector = globalTransform.rotate(PxVec3{ 0.0f, 1.0f, 0.0f });

  if (!shouldStick) {
    for (int i = 0; i < WHEEL_NB; ++i) {
      if (!m_wheelHitInfo[i].first) continue;

      PxVec3 globalCorner = globalTransform.transform(LOCAL_WHEELS[i]);
      PxRaycastHit hit = m_wheelHitInfo[i].second;

      // Calculate the spring force to apply
      float compression = 1 - (hit.distance / HOVER_HEIGHT);
      float upwardVelocity = m_frame->getLinearVelocity().dot(upVector);
      PxVec3 force = upVector * (compression * STIFFNESS - DAMPING * upwardVelocity) * 0.25;
      PxRigidBodyExt::addForceAtPos(
        *m_frame,
        force,
        globalCorner
      );

      // Simple friction
      auto materials = std::make_unique<PxMaterial*[]>(hit.shape->getNbMaterials());

      if (hit.shape->getMaterials(materials.get(), hit.shape->getNbMaterials()) > 0) {
        PxVec3 velocity = PxRigidBodyExt::getVelocityAtPos(*m_frame, globalCorner) -
          PxRigidBodyExt::getVelocityAtPos(*m_frame, globalCorner).dot(hit.normal) * hit.normal;
        m_frame->addForce(-velocity * materials[0]->getDynamicFriction() * 0.25, PxForceMode::eACCELERATION);
      }
    }

    handleAntiRoll();
  } else {
    PxVec3 groundNormal = PxVec3{ 0.0f, 0.0f, 0.0f };

    for (int i = 0; i < 4; ++i)
      if (m_wheelHitInfo[i].first) groundNormal += m_wheelHitInfo[i].second.normal;

    PxRaycastHit hit = m_frameHitInfo.second;

    // Calculate the spring force to apply
    float compression = 1 - (hit.distance / HOVER_HEIGHT);
    float upwardVelocity = m_frame->getLinearVelocity().dot(upVector);
    PxVec3 force = upVector * (compression * STIFFNESS - DAMPING * upwardVelocity);
    force -= upVector * DOWNWARD_ACCELERATION * PlayerVehicle::MASS;
    m_frame->addForce(force, PxForceMode::eFORCE);

    // Simple friction
    auto materials = std::make_unique<PxMaterial*[]>(hit.shape->getNbMaterials());

    if (hit.shape->getMaterials(materials.get(), hit.shape->getNbMaterials()) > 0) {
      PxVec3 velocity = PxRigidBodyExt::getVelocityAtPos(*m_frame, globalTransform.p) -
        PxRigidBodyExt::getVelocityAtPos(*m_frame, globalTransform.p).dot(hit.normal) * hit.normal;
      m_frame->addForce(-velocity * materials[0]->getDynamicFriction(), PxForceMode::eACCELERATION);
    }

    /* Vehicule parallel au sol */
    if (!groundNormal.isZero()) {
      groundNormal.normalize();

      PxQuat orientationActuelle = m_frame->getGlobalPose().q;

      // Calculez la rotation nécessaire pour aligner l'orientation du véhicule avec la normale du sol.
      PxQuat rotationVersLeSol = calculateRotationQuaternion(upVector, groundNormal);

      // Appliquez la rotation au véhicule pour qu'il soit parallèle au sol.
      PxQuat nouvelleOrientation = rotationVersLeSol * orientationActuelle;
      nouvelleOrientation.normalize();
      m_frame->setGlobalPose(PxTransform(m_frame->getGlobalPose().p, nouvelleOrientation));
    }
  }
}

void Player::handleAntiRoll()
{
  PxTransform globalTransform = m_frame->getGlobalPose();
  PxVec3 upVector = globalTransform.rotate(PxVec3{ 0.0f, 1.0f, 0.0f });

  for (int i = 0; i < WHEEL_NB / 2; ++i) {
    float travelL = 1.0f;
    float travelR = 1.0f;

    int leftWheelIndex = 2 * i;
    int rightWheelIndex = 2 * i + 1;

    if (m_wheelHitInfo[leftWheelIndex].first) travelL = m_wheelHitInfo[leftWheelIndex].second.distance / HOVER_HEIGHT;
    if (m_wheelHitInfo[rightWheelIndex].first) travelR = m_wheelHitInfo[rightWheelIndex].second.distance / HOVER_HEIGHT;

    float antiRollForce = (travelL - travelR) * ANTI_ROLL;

    if (m_wheelHitInfo[leftWheelIndex].first)
      PxRigidBodyExt::addForceAtLocalPos(
        *m_frame,
        -upVector * antiRollForce,
        LOCAL_WHEELS[leftWheelIndex]
      );

    if (m_wheelHitInfo[rightWheelIndex].first)
      PxRigidBodyExt::addForceAtLocalPos(
        *m_frame,
        upVector * antiRollForce,
        LOCAL_WHEELS[rightWheelIndex]
      );
  }
}