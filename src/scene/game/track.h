#pragma once

#include "display/graphical_resource.h"
#include "physics/physics.h"
#include "utils/bezier_curve.h"
#include "world/object.h"

class Track : public pbl::WorldProp
{
private:
  struct AnchorPoint {
    vec3 position;
    vec3 right;
    vec3 up;
    vec3 forward;
  };

public:
  struct AttractionPoint {
    vec3 position;
    float strength;
  };

  struct TrackPoint {
    float x, y; 
    float uvy;
  };

  using TrackProfile = std::vector<TrackPoint>;

  struct TrackProfileTemplate {
    float innerWidth = 20.f;
    float height = 1.f;
    float bordersOffset = 1.3f;
    float bordersWidth = 4.5f;

    TrackProfile buildProfile() const;
  };
    
public:
  Track(BezierCurve curve, TrackProfile profile, std::vector<AttractionPoint> attractionPoints, pbl::GraphicalResourceRegistry &resources);
  Track(BezierCurve curve, TrackProfile profile, std::vector<AttractionPoint> attractionPoints, pbl::Effect *effect, const pbl::Texture &texture);

  pbx::PhysicsBody *buildPhysicsObject() override;

private:
  pbl::Mesh buildTrackMesh(pbl::Effect *effect, const pbl::Texture &texture) const;

  std::vector<AnchorPoint> sampleEvenlySpacedPoints() const;
  vec3 getOrientedVertical(vec3 position, vec3 forward) const;
  std::vector<pbl::BaseVertex> createMeshVertices(const std::vector<AnchorPoint> &anchorPoints) const;
  std::vector<pbl::Mesh::index_t> createMeshIndices(const std::vector<AnchorPoint> &anchorPoints) const;

private:
  TrackProfile m_profile;
  BezierCurve m_curve;
  std::unique_ptr<pbx::PhysicsBody> m_physicsBody;
  std::vector<AttractionPoint> m_attractionPoints;
};
