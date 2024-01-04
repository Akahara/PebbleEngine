#include "bezier_curve.h"

#include <fstream>

vec3 BezierCurve::samplePoint(float t) const
{
  if (t <= 0) return controlPoints.front().position;
  if (t >= static_cast<float>(controlPoints.size()-1)) return controlPoints.back().position;
  int ft = static_cast<int>(t);
  return interpolate(controlPoints[ft], controlPoints[ft+1], t-static_cast<float>(ft));
}

DiscreteCurve BezierCurve::discretize(float delta) const
{
  DiscreteCurve curve;
  for (float t = 0; t < static_cast<float>(controlPoints.size()); t += delta)
    curve.points.push_back(samplePoint(t));
  return curve;
}

DiscreteCurve BezierCurve::discretizeEvenly(const EvenDiscretizationSettings &settings) const
{
  DiscreteCurve curve;

  if (controlPoints.empty())
    return curve;

  vec3 prevPoint = controlPoints[0].position;
  vec3 nextPoint;
  for (float t = 0; t < static_cast<float>(controlPoints.size()); ) {
    // perform a bisection to find the next anchor point
    float bisectIntervalMin = t;
    float bisectIntervalMax = t + settings.binarySearchInitialStep*2.f;
    t += settings.binarySearchInitialStep;
    for(int i = 0; i < settings.maxBinarySearchIterations; i++) {
      nextPoint = samplePoint(t);
      float dist = XMVectorGetX(XMVector3Length(prevPoint - nextPoint));
      if (std::abs(dist - settings.targetEvenDistance) < settings.acceptedEvenDistanceDelta)
        break;
      if(dist < settings.targetEvenDistance) {
        bisectIntervalMin = t;
        t = (t + bisectIntervalMax) * .5f;
      } else {
        bisectIntervalMax = t;
        t = (t + bisectIntervalMin) * .5f;
      }
    }

    prevPoint = nextPoint;
    curve.points.emplace_back(nextPoint);
  }

  return curve;
}

vec3 BezierCurve::interpolate(const BezierControlPoint& c0, const BezierControlPoint& c1, float t)
{
  // this code could use a nice matrix multiplication
  const vec3 &p0 = c0.position;
  const vec3 &p1 = c0.handleRight;
  const vec3 &p2 = c1.handleLeft;
  const vec3 &p3 = c1.position;
  float q = 1-t;
  float t0 = 1*q*q*q;
  float t1 = 3*t*q*q;
  float t2 = 3*t*t*q;
  float t3 = 1*t*t*t;
  return t0 * p0 + t1 * p1 + t2 * p2 + t3 * p3;
}

BezierCurve BezierCurve::loadFromFile(const std::filesystem::path& filePath)
{
  std::ifstream is{ filePath };
  if (!is) throw std::runtime_error("Could not open bezier curve file: " + filePath.string());
  BezierCurve bezier;
  rvec3 position, handleLeft, handleRight;
  while(is) {
    if(is
      >> position.x >> position.y >> position.z
      >> handleLeft.x >> handleLeft.y >> handleLeft.z
      >> handleRight.x >> handleRight.y >> handleRight.z)
      bezier.controlPoints.push_back({ XMLoadFloat3(&position), XMLoadFloat3(&handleLeft), XMLoadFloat3(&handleRight) });
  }
  bezier.isLoop = XMVector3Equal(bezier.controlPoints.front().position, bezier.controlPoints.back().position);
  return bezier;
}

void BezierCurve::writeToFile(const std::filesystem::path& filePath, const BezierCurve &bezier)
{
  std::ofstream os{ filePath };
  if (!os) throw std::runtime_error("Could not open bezier curve file for write: " + filePath.string());
  rvec3 position, handleLeft, handleRight;
  for(const BezierControlPoint &pt : bezier.controlPoints) {
    XMStoreFloat3(&position, pt.position);
    XMStoreFloat3(&handleLeft, pt.handleLeft);
    XMStoreFloat3(&handleRight, pt.handleRight);
    os
      << position.x << ' ' << position.y << ' ' << position.z << ' '
      << handleLeft.x << ' ' << handleLeft.y << ' ' << handleLeft.z << " "
      << handleRight.x << ' ' << handleRight.y << ' ' << handleRight.z << "\n";
  }
}

vec3 DiscreteCurve::samplePoint(float t) const
{
  if (t <= 0) return points.front();
  if (t >= static_cast<float>(points.size()-1)) return points.back();
  int ft = static_cast<int>(t);
  return interpolate(points[ft], points[ft+1], t-static_cast<float>(ft));
}

vec3 DiscreteCurve::sampleLoopPoint(float t) const
{
  return mathf::lerp(
    points[mathf::positiveModulo(static_cast<size_t>(t), points.size())],
    points[mathf::positiveModulo(static_cast<size_t>(t)+1, points.size())],
    mathf::fract(t));
}

vec3 DiscreteCurve::sampleStraightTangent(float t) const
{
  return XMVector3Normalize(
      points[mathf::positiveModulo(static_cast<size_t>(t)+1, points.size())]
    - points[mathf::positiveModulo(static_cast<size_t>(t), points.size())]);
}

vec3 DiscreteCurve::sampleSmoothTangent(float t, float delta) const
{
  return XMVector3Normalize(sampleLoopPoint(t + delta) - sampleLoopPoint(t));
}
