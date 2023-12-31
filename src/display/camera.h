#pragma once

#include <variant>

#include "utils/aabb.h"
#include "utils/math.h"

namespace pbl
{

struct PerspectiveProjection {
  float fovy   = DirectX::XM_PI * .4f;
  float aspect = 16.f/9.f;
  float zNear  = 3.f;     // world space
  float zFar   = 10000.f; // world space

  mat4 buildProjectionMatrix() const;
};

struct OrthographicProjection {
  float width  = 160;    // world space
  float height = 90;     // world space
  float zNear  = 3.f;    // world space
  float zFar = 10000.f;  // world space

  mat4 buildProjectionMatrix() const;
};

using CameraProjection = std::variant<PerspectiveProjection, OrthographicProjection>;


class Camera {
public:
  static constexpr vec3 RIGHT   { 1,0,0,0 };
  static constexpr vec3 UP      { 0,1,0,0 };
  static constexpr vec3 FORWARD { 0,0,1,0 };
public:
  Camera() = default;

  const CameraProjection &getProjection() const { return m_projection; }
  void setProjection(const CameraProjection &proj);
  void setPosition(const vec3 &position) { m_position = XMVectorSetW(position, 1.f); }
  void setRotation(const quat &rotation) { m_rotation = rotation; }
  void setRotation(float yaw, float pitch);
  void lookAt(const vec3 &target);

  void rotate(const quat rot) { m_rotation *= rot; }
  void rotate(float rx, float ry, float rz, bool absolute = false);
  void move(const vec3 &dl);

  static quat quaternionLookAt(const vec3 &pos, const vec3 &target);

  // must be called after a position/rotation update !
  void updateViewMatrix();
  // must be called after a view/projection update !
  void updateViewProjectionMatrix();

  const mat4 &getViewProjectionMatrix() const { return m_viewProjectionMatrix; }
  const mat4 &getProjectionMatrix() const { return m_projectionMatrix; }
  const mat4 &getViewMatrix() const { return m_viewMatrix; }
  const vec3 &getPosition() const { return m_position; }
  const quat &getRotation() const { return m_rotation; }

  vec3 getRight() const;
  vec3 getUp() const;
  vec3 getForward() const;
  vec3 getFlatForward() const;

private:
  CameraProjection m_projection;
  vec3  m_position = { 0,0,0,1 };
  quat  m_rotation = DirectX::XMQuaternionIdentity();
  mat4  m_projectionMatrix{};
  mat4  m_viewMatrix{};
  mat4  m_viewProjectionMatrix{};
};

struct Plane
{
  vec3 normal{};
  float distanceToOrigin = 0;

  Plane() = default;

  Plane(const vec3& pl, const vec3 norm) 
    : normal(XMVector3Normalize(norm))
    , distanceToOrigin(XMVectorGetX(XMVector3Dot(normal, pl)))
  {}

  float signedDistanceTo(const vec3& point) const
  {
    return XMVectorGetX(XMVector3Dot(normal, point)) - distanceToOrigin;
  }
};

/*
 * A frustum is the world region that is visible to a camera.
 * Frustum intersections can be checked against AABBs and such to do frustum culling.
 */
struct Frustum
{
  Plane topFace;
  Plane bottomFace;
  Plane rightFace;
  Plane leftFace;
  Plane farFace;
  Plane nearFace;

  bool isOnFrustum(const AABB &boudingBox) const;

  static Frustum createFrustumFromCamera(const Camera &cam);
  static Frustum createFrustumFromProjection(const Camera &cam, const OrthographicProjection &proj);
  static Frustum createFrustumFromProjection(const Camera &cam, const PerspectiveProjection &proj);

  static bool isOnOrForwardPlan(const Plane &plane, const AABB &boundingBox);
};


}