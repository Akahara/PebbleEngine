#pragma once

#include "utils/aabb.h"
#include "utils/math.h"

struct Transform
{
  vec3 position{ 0,0,0,1 };
  vec3 scale{ 1,1,1,1 };
  quat rotation = DirectX::XMQuaternionIdentity();

  mat4 getWorldMatrix() const;
  vec3 getUp() const;
  vec3 getForward() const;
  vec3 getRight() const;

  vec3 transform(const vec3 &localPos) const { return XMVector3Rotate(localPos * scale, rotation) + position; }
  quat transformRotation(const quat &localRot) const { return XMQuaternionMultiply(localRot, rotation); }
  Transform transform(const Transform &localTransform) const;
  vec3 rotate(vec3 localPos) const { return XMVector3Rotate(localPos, rotation); }

  // raw approximation of a bounding box, only works if the original model
  // fits in -1..+1 on all axes before this transform's scale is applied
  AABB getBoundingBox() const;
  // returns whether the scale of this transform is the default one.
  // this is important because physics actors cannot be scaled
  bool hasUnitScale() const
  {
    rvec3 rscale; XMStoreFloat3(&rscale, scale);
    return rscale.x == 1.f && rscale.y == 1.f && rscale.z == 1.f;
  }
};
