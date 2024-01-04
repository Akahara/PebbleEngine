#include "aabb.h"

#include "world/transform.h"

AABB AABB::getRotationIndependantBoundingBox(const Transform &transform) const
{
  rvec4 rot;
  XMStoreFloat4(&rot, transform.rotation);
  float l = (rot.x == 0 && rot.y == 0 && rot.z == 0 && rot.w == 1) ? 1 : XMVectorGetX(XMVector3Length(m_size*.5f));
  return AABB(transform.position + m_origin * transform.scale * l, m_size * transform.scale * l);
}
