#pragma once

#include <filesystem>
#include <vector>

#include "./math.h"

struct DiscreteCurve {
  std::vector<vec3> points;

  /*
  * returns interpolate(points[floor(t)], points[floor(t+1)], fract(t))
  * meaning that if points are unevenly distributed samples will not be
  * evenly distanced
  */
  vec3 samplePoint(float t) const;
  /*
  * Same as samplePoint but t wraps arround in range [0,points.size()[
  */
  vec3 sampleLoopPoint(float t) const;

  /*
  * Tangent sampling, eighter straight (exact) or smooth, both wrap arround
  * if t is not in range [0,points.size()[
  */
  vec3 sampleStraightTangent(float t) const;
  vec3 sampleSmoothTangent(float t, float delta=.5f) const;

  // t must be in range 0..1
  static vec3 interpolate(const vec3 &p0, const vec3 &p1, float t) 
  {
    return mathf::lerp(p0, p1, t);
  }
};

struct BezierControlPoint {
  vec3 position{};
  vec3 handleLeft{};
  vec3 handleRight{};
};

/* A cubic bezier curve */
struct BezierCurve {
  std::vector<BezierControlPoint> controlPoints;
  bool isLoop = false;

  struct EvenDiscretizationSettings {
    // binary search settings
    int   maxBinarySearchIterations = 8;
    float binarySearchInitialStep = .2f;
    // world settings (the distance at which two discretized points should be, on average)
    // depends heavily on your use-case
    float targetEvenDistance = 2.f;
    float acceptedEvenDistanceDelta = .1f;
  };

  /*
   * returns interpolate(points[floor(t)], points[floor(t+1)], fract(t))
   * meaning that if points are unevenly distributed samples will not be
   * evenly distanced
   */
  vec3 samplePoint(float t) const;

  DiscreteCurve discretize(float delta) const;
  DiscreteCurve discretizeEvenly(const EvenDiscretizationSettings &settings={}) const;

  // t must be in range 0..1
  static vec3 interpolate(const BezierControlPoint &c0, const BezierControlPoint &c1, float t);

  /*
   * Reads a bezier curve from a text file.
   * A valid bezier file contains 9 numbers per control point, layed out as
   * the BezierControlPoint structure, separated by blank characters (space,
   * tabs, new lines...). There is no requirement on the number of control points.
   */
  static BezierCurve loadFromFile(const std::filesystem::path &filePath);
  static void writeToFile(const std::filesystem::path &filePath, const BezierCurve &bezier);
};
