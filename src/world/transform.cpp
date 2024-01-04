#include "transform.h"

#include "display/camera.h"
#include "utils/debug.h"

mat4 Transform::getWorldMatrix() const
{
  return  XMMatrixScalingFromVector(scale) * XMMatrixRotationQuaternion(rotation) * XMMatrixTranslationFromVector(position);
}

vec3 Transform::getUp() const
{
  return XMVector3Rotate(pbl::Camera::UP, rotation);
}

vec3 Transform::getForward() const
{
  return XMVector3Rotate(pbl::Camera::FORWARD, rotation);
}

vec3 Transform::getRight() const
{
  return XMVector3Rotate(pbl::Camera::RIGHT, rotation);
}

Transform Transform::transform(const Transform &localTransform) const
{
  PBL_ASSERT(localTransform.hasUnitScale(), "Cannot apply a transform to a scaled transformation");
  return { transform(localTransform.position), scale, transformRotation(localTransform.rotation) };
}

AABB Transform::getBoundingBox() const
{
  // take the bounding box of the bounding sphere
  // that way we don't have to consider rotations
  float l = XMVectorGetX(XMVector3Length(scale));
  vec3 halfSize = { l,l,l,0 };
  return AABB(position - halfSize, 2*halfSize);
}
