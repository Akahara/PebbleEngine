﻿#pragma once

#include <cassert>

#include "./math.h"

struct Transform;

/* Axis-aligned bounding box */
class AABB {
private:
  vec3 m_origin;
  vec3 m_size;
public:
  AABB()
    : m_origin{}, m_size{} { }
  AABB(const vec3 &origin, const vec3 &size)
    : m_origin(origin), m_size(size)
  {
    assert(XMVectorGetX(size) >= 0);
    assert(XMVectorGetY(size) >= 0);
    assert(XMVectorGetZ(size) >= 0);
  }

  static AABB make_aabb(const vec3 &A, const vec3 &B)
  {
    vec3 origin = XMVectorMin(A, B);
    vec3 size = XMVectorAbs(B-A);
    return { origin, size };
  }

  const vec3 &getOrigin() const { return m_origin; }
  const vec3 &getSize() const { return m_size; }

  bool isInBounds(const vec3 &point) const 
  {
    return XMVector3InBounds(point - m_origin, m_size);
  }

  /*
   * return a copy of this aabb where each side is moved away by
   * `absoluteGrowth`. If a negative amount is given and one of
   * the side length is small enough, a generate AABB will be returned.
   */
  AABB grow(float absoluteGrowth) const
  {
    float l = absoluteGrowth*.5f;
    vec3 g = { l,l,l,0 };
    return { m_origin - g, m_size + g };
  }

  AABB getRotationIndependantBoundingBox(const Transform &transform) const;

};
