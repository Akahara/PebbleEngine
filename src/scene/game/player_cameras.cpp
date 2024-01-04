#include "player_cameras.h"

#include "player.h"
#include "physics/physics.h"
#include "physics/physxlib.h"
#include "utils/regions.h"

void CameraController::updateCameraProjection(pbl::Camera &camera)
{
  struct {
    pbl::CameraProjection operator()(pbl::PerspectiveProjection proj) {
      proj.aspect = pbl::ScreenRegion::SCREEN_WIDTH / pbl::ScreenRegion::SCREEN_HEIGHT;
      return proj;
    }
    pbl::CameraProjection operator()(pbl::OrthographicProjection proj) {
      proj.width = proj.height * pbl::ScreenRegion::SCREEN_WIDTH / pbl::ScreenRegion::SCREEN_HEIGHT;
      return proj;
    }
  } visitor;

  camera.setProjection(std::visit(visitor, camera.getProjection()));
}

vec3 CameraRail::getClosestPointOnTrack(vec3 p) const
{
  vec3 nearest = points[0];
  float nearestDist = XMVectorGetX(XMVector3Length(p-nearest));
  for(size_t i = 0; i < points.size()-1; i++) {
    const vec3 &p0 = points[i], &p1 = points[i+1];
    vec3 r = p1-p0;
    vec3 d = XMVector3Normalize(r);
    vec3 nearestOnSegment = p0 + d * mathf::clamp(XMVectorGetX(XMVector3Dot(p - p0, d)), 0.f, XMVectorGetX(XMVector3Length(r)));
    float dist = XMVectorGetX(XMVector3Length(p-nearestOnSegment)); // suboptimal
    if(nearestDist > dist) {
      nearestDist = dist;
      nearest = nearestOnSegment;
    }
  }
  return nearest;
}

void ShakeParameters::addShake(float duration, float force, int frameSkip)
{
  m_chrono = duration;
  m_force = force;
  m_frameSkip = frameSkip;
  m_skippedFrames = -1;
}

vec3 ShakeParameters::shakeDisplacement(double delta)
{
  if (m_chrono > 0) {
    m_skippedFrames = (m_skippedFrames + 1) % m_frameSkip;
    if (m_skippedFrames == 0) {
      m_chrono -= static_cast<float>(delta);
      float r = mathf::inverseLerp(static_cast<float>(std::mt19937::min()), static_cast<float>(std::mt19937::max()), static_cast<float>(m_rd()));
      m_latestAngle = mathf::lerp(-SHAKE_ANGLE_RANGE*.5f, +SHAKE_ANGLE_RANGE*.5f, r);
      return (pbl::Camera::RIGHT * cos(m_latestAngle/180.f*PI) + pbl::Camera::UP * sin(m_latestAngle/180.f*PI)) * m_force * (1.f - 1.f / (1.f+m_chrono));
    }
  }
  return { 0,0,0 };
}

ThirdPersonCameraController::ThirdPersonCameraController()
{
  m_camera.setProjection(pbl::PerspectiveProjection{});
  updateProjection();
}

void ThirdPersonCameraController::updateProjection()
{
  updateCameraProjection(m_camera);
}

void ThirdPersonCameraController::resetPosition()
{
  m_camera.setPosition(pbx::physics2scenePosition(m_vehicle->getFrame()->getGlobalPose().p + physx::PxVec3(0.0f, 6.0f, 0.0f)));
  for (int i = 0; i < 6; i++)
    update(.1f);
}

void ThirdPersonCameraController::update(double delta)
{
  float angularVelocity = m_vehicle->getFrame()->getAngularVelocity().magnitude();
  physx::PxTransform globalTransform = m_vehicle->getFrame()->getGlobalPose();

  // when the vehicle goes flying and rotating rapidly, do not make the camera follow the vehicle's rotation
  if (angularVelocity > 6000.f) {
    vec3 position = pbx::physics2scenePosition(globalTransform.p) + m_cameraProbe;
    m_camera.setPosition(position);
    m_camera.lookAt(pbx::physics2scenePosition(m_vehicle->getFrame()->getGlobalPose().p));
    m_camera.updateViewMatrix();
    return;
  }

  physx::PxVec3 forwardVector = globalTransform.rotate(physx::PxVec3{ 1.0f, 0.0f, 0.0f });
  float vehicleSpeed = m_vehicle->getFrame()->getLinearVelocity().dot(forwardVector);
  float vehicleSpeedNormalized = mathf::clamp(mathf::inverseLerp(0.f, 150.f, vehicleSpeed));
  float cameraHeight = mathf::lerp(.9f, .6f, vehicleSpeedNormalized);
  float cameraProbeLength = mathf::lerp(7.f, 5.f, vehicleSpeedNormalized);
  vec3 cameraPosition = pbx::physics2scenePosition(m_vehicle->getFrame()->getGlobalPose().p + globalTransform.rotate(physx::PxVec3{ -1.5f, cameraHeight, 0 }) * cameraProbeLength);
  float targetFov = mathf::lerp(DirectX::XM_PI * .35f, DirectX::XM_PI * .45f, vehicleSpeedNormalized);

  pbl::PerspectiveProjection proj = std::get<pbl::PerspectiveProjection>(m_camera.getProjection());
  proj.fovy = mathf::lerp(proj.fovy, targetFov, .05f);
  m_cameraProbe = m_camera.getPosition() - pbx::physics2scenePosition(globalTransform.p);

  m_trueCameraPos = mathf::lerp(m_trueCameraPos, cameraPosition, .3f);

  m_camera.setPosition(m_trueCameraPos);
  m_camera.lookAt(pbx::physics2scenePosition(m_vehicle->getFrame()->getGlobalPose().p));

  // Screenshake
  m_camera.setPosition(m_trueCameraPos + XMVector3Rotate(m_shake.shakeDisplacement(delta), m_camera.getRotation()));

  m_camera.setProjection(proj);
  m_camera.updateViewMatrix();
}

FirstPersonCameraController::FirstPersonCameraController()
{
  m_camera.setProjection(pbl::PerspectiveProjection{});
  updateCameraProjection(m_camera);
}

void FirstPersonCameraController::updateProjection()
{
  updateCameraProjection(m_camera);
}

void FirstPersonCameraController::resetPosition()
{
  update(0);
}

void FirstPersonCameraController::update(double delta)
{
  physx::PxTransform globalTransform = m_vehicle->getFrame()->getGlobalPose();

  physx::PxVec3 forwardVector = globalTransform.rotate(physx::PxVec3{ 1.0f, 0.0f, 0.0f });
  float vehicleSpeed = m_vehicle->getFrame()->getLinearVelocity().dot(forwardVector)/Player::MAX_SPEED;
  pbl::PerspectiveProjection proj = std::get<pbl::PerspectiveProjection>(m_camera.getProjection());
  float targetFov = mathf::lerp(DirectX::XM_PI * .3f, DirectX::XM_PI * .45f, vehicleSpeed);
  proj.fovy = mathf::lerp(proj.fovy, targetFov, .05f);
  m_camera.setPosition(pbx::physics2scenePosition(globalTransform.transform(pbx::scene2physicsPosition(m_cameraProbe))));
  m_camera.setRotation(XMQuaternionSlerp(m_camera.getRotation(), pbx::physics2sceneRotation(globalTransform.q), .2f));
  m_camera.setProjection(proj);
  m_camera.updateViewMatrix();
}

FixedCameraController::FixedCameraController()
{
  m_camera.setProjection(pbl::PerspectiveProjection{});
  updateProjection();
}

void FixedCameraController::updateProjection()
{
  updateCameraProjection(m_camera);
}

void FixedCameraController::resetPosition()
{
  update(0);
}

void FixedCameraController::update(double delta)
{
  vec3 closest;
  vec3 playerPos = m_vehicle->getTransform().position;
  if(XMVectorGetX(XMVector3LengthSq(m_offset)) == 0.f) {
    float closestDistance = std::numeric_limits<float>::max();
    for(CameraRail &rail : m_rails) {
      vec3 c = rail.getClosestPointOnTrack(playerPos);
      float d = XMVectorGetX(XMVector3Length(c - playerPos));
      if(d < closestDistance) {
        closestDistance = d;
        closest = c;
      }
    }
  } else {
    closest = playerPos + m_offset;
  }
  m_camera.setPosition(closest);
  m_camera.lookAt(playerPos);
  m_camera.updateViewMatrix();
}
