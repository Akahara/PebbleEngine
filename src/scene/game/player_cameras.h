#pragma once

#include <vector>
#include <random>

#include "player_vehicle.h"
#include "display/camera.h"
#include "utils/math.h"


class CameraController
{
public:
  virtual ~CameraController() = default;

  const pbl::Camera &getCamera() const { return m_camera; }
  void setVehicle(const PlayerVehicle *vehicle) { m_vehicle = vehicle; resetPosition(); }

  virtual void updateProjection() = 0;
  virtual void resetPosition() = 0;
  virtual void update(double delta) = 0;

protected:
  static void updateCameraProjection(pbl::Camera &camera);

protected:
  const PlayerVehicle *m_vehicle = nullptr;
  pbl::Camera m_camera;
};

struct CameraRail {
  std::vector<vec3> points;

  vec3 getClosestPointOnTrack(vec3 p) const;
};

struct ShakeParameters
{
  constexpr static float SHAKE_ANGLE_RANGE = 220.f;
	float m_chrono{};
	float m_force{};
	std::mt19937 m_rd;
	int m_frameSkip{};

	float m_latestAngle{};
	int m_skippedFrames{};

public:
	ShakeParameters() : m_rd(std::random_device{}()){}
  
	void addShake(float duration, float force, int frameSkip = 1);
	vec3 shakeDisplacement(double delta);
};

class ThirdPersonCameraController : public CameraController
{
public:
  ThirdPersonCameraController();

  void updateProjection() override;
  void resetPosition() override;
  void update(double delta) override;

  void addShake(const float duration, const float force, const int frameSkip = 1) 
  { m_shake.addShake(duration, force, frameSkip); }

private:
  vec3 m_cameraProbe{};
  ShakeParameters m_shake;
  vec3 m_trueCameraPos;
};

class FirstPersonCameraController : public CameraController
{
public:
  FirstPersonCameraController();

  void updateProjection() override;
  void resetPosition() override;
  void update(double delta) override;

private:
  const vec3 m_cameraProbe{ 0.f, +.3f, +.6f };
};

class FixedCameraController : public CameraController
{
public:
  explicit FixedCameraController();

  void updateProjection() override;
  void resetPosition() override;
  void update(double delta) override;

  void addRail(CameraRail rail) { m_rails.push_back(std::move(rail)); }
  void setOffset(vec3 offset) { m_offset = offset; }

private:
  std::vector<CameraRail> m_rails;
  vec3 m_offset;
};