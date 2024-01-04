#include "track.h"

#include "display/renderer.h"
#include "physics/physxlib.h"

static constexpr int   MAX_BISECTION_ITERATIONS = 8;
static constexpr float TARGET_ANCHORS_DISTANCE = 2.f;
static constexpr float ACCEPTED_ANCHORS_DISTANCE_DELTA = .1f;
static constexpr float BISECTION_INITIAL_STEP = .2f;

Track::Track(BezierCurve curve, TrackProfile profile, std::vector<AttractionPoint> attractionPoints, pbl::GraphicalResourceRegistry &resources)
  : Track(
    std::move(curve),
    std::move(profile),
    std::move(attractionPoints),
    resources.loadEffect(L"res/shaders/miniphong.fx", pbl::BaseVertex::getShaderVertexLayout()),
    resources.loadTexture(L"res/textures/track.dds")
  )
{
}

Track::Track(BezierCurve curve, TrackProfile profile, std::vector<AttractionPoint> attractionPoints, pbl::Effect *effect, const pbl::Texture &texture)
  : m_curve(std::move(curve))
  , m_profile(std::move(profile))
  , m_attractionPoints(std::move(attractionPoints))
{
  setMesh(std::make_shared<pbl::Mesh>(buildTrackMesh(effect, texture)));
}

pbx::PhysicsBody *Track::buildPhysicsObject()
{
  if (m_physicsBody) return m_physicsBody.get();

  std::vector<AnchorPoint> anchorPoints = sampleEvenlySpacedPoints();

  physx::PxTriangleMeshGeometry geom(makePhysicsMeshFromModel({
    createMeshVertices(anchorPoints),
    createMeshIndices(anchorPoints)
  }));

  m_physicsBody = std::make_unique<pbx::PhysicsBody>(this);
  m_physicsBody->addActor(PxCreateStatic(pbx::Physics::getSdk(), physx::PxTransform({ 0,0,0 }), geom, *pbx::Physics::getSdk().createMaterial(1, 1, .2f)));
  return m_physicsBody.get();
}

Track::TrackProfile Track::TrackProfileTemplate::buildProfile() const
{
  return {{
    { +innerWidth*.5f,                0,                      00.f/120.f },
    { +innerWidth*.5f + bordersWidth, bordersOffset,          10.f/120.f },
    { +innerWidth*.5f + bordersWidth, bordersOffset - height, 20.f/120.f },
    { +innerWidth*.5f,               -height,                 30.f/120.f },
    { -innerWidth*.5f,               -height,                 40.f/120.f },
    { -innerWidth*.5f - bordersWidth, bordersOffset - height, 50.f/120.f },
    { -innerWidth*.5f - bordersWidth, bordersOffset,          60.f/120.f },
    { -innerWidth*.5f,                0,                      70.f/120.f },
    { +innerWidth*.5f,                0,                     100.f/120.f },
  }};
}

pbl::Mesh Track::buildTrackMesh(pbl::Effect *effect, const pbl::Texture &texture) const
{
  using namespace pbl;

  std::vector<AnchorPoint>  anchorPoints = sampleEvenlySpacedPoints();
  std::vector<BaseVertex>   meshVertices = createMeshVertices(anchorPoints);
  std::vector<Mesh::index_t> meshIndices = createMeshIndices(anchorPoints);

  std::vector<Mesh::SubMesh> submeshes;
  Mesh::SubMesh submesh;
  submesh.indexOffset = 0;
  submesh.indexCount = static_cast<Mesh::index_t>(meshIndices.size());
  submesh.effect = effect;
  submesh.textures = std::vector<TextureBinding>{ { "objectTexture", texture } };
  submesh.samplers = std::vector<SamplerBinding>{ { "samplerState", TextureManager::getSampler(SamplerState::BASIC) } };
  submesh.material.diffuse = { 1.f,1.f,1.f,1.f };
  submesh.material.specular = { .1f,.1f,.1f,1.f };
  submesh.material.specularExponent = 2.f;
  submeshes.push_back(submesh);

  return Mesh(meshIndices, meshVertices, std::move(submeshes));
}

std::vector<Track::AnchorPoint> Track::sampleEvenlySpacedPoints() const
{
  std::vector<AnchorPoint> points;

  vec3 prevPoint = m_curve.samplePoint(0);
  float t = 0;
  while(t < static_cast<float>(m_curve.controlPoints.size())-1) {
    vec3 nextPoint;

    // perform a bisection to find the next anchor point
    float bisectIntervalMin = t;
    float bisectIntervalMax = t + BISECTION_INITIAL_STEP*2.f;
    t += BISECTION_INITIAL_STEP;
    for(int i = 0; i < MAX_BISECTION_ITERATIONS; i++) {
      nextPoint = m_curve.samplePoint(t);
      float dist = XMVectorGetX(XMVector3Length(prevPoint - nextPoint));
      if (std::abs(dist - TARGET_ANCHORS_DISTANCE) < ACCEPTED_ANCHORS_DISTANCE_DELTA)
        break;
      if(dist < TARGET_ANCHORS_DISTANCE) {
        bisectIntervalMin = t;
        t = (t + bisectIntervalMax) * .5f;
      } else {
        bisectIntervalMax = t;
        t = (t + bisectIntervalMin) * .5f;
      }
    }

    vec3 forward = XMVector3Normalize(nextPoint - prevPoint);
    vec3 up = getOrientedVertical(nextPoint, forward);
    vec3 right = XMVector3Normalize(XMVector3Cross(up, forward));
    points.emplace_back(nextPoint, right, up, forward);

    prevPoint = nextPoint;
  }

  if (m_curve.isLoop)
    points.push_back(points.front());

  return points;
}

vec3 Track::getOrientedVertical(vec3 position, vec3 forward) const
{
  vec3 up{ 0,1,0 };

  for(const AttractionPoint &p : m_attractionPoints) {
    vec3 d = p.position - position;
    float l = XMVectorGetX(XMVector3Length(d));
    up = XMVectorLerp(up, XMVector3Normalize(d), 1.f / (1+l*l/p.strength));
  }

  return up - XMVector3Dot(up, forward) * forward;
}

std::vector<pbl::BaseVertex> Track::createMeshVertices(const std::vector<AnchorPoint> &anchorPoints) const
{
  std::vector<pbl::BaseVertex> vertices;

  float uvx = 0.f;
  for (const auto& anchorPoint : anchorPoints) {
    for (size_t j = 0; j < m_profile.size()-1; j++) {
      const TrackPoint &vertex = m_profile[j];
      const TrackPoint &nextVertex = m_profile[j+1];
      vec3 p1 = anchorPoint.position + anchorPoint.right * vertex.x + anchorPoint.up * vertex.y;
      vec3 p2 = anchorPoint.position + anchorPoint.right * nextVertex.x + anchorPoint.up * nextVertex.y;
      rvec3 pos1, pos2, normal;
      rvec2 uv1{ uvx, vertex.uvy };
      rvec2 uv2{ uvx, nextVertex.uvy };
      XMStoreFloat3(&pos1, p1);
      XMStoreFloat3(&pos2, p2);
      XMStoreFloat3(&normal, XMVector3Normalize(XMVector3Cross(p1 - p2, anchorPoint.forward)));
      vertices.emplace_back(pos1, normal, uv1);
      vertices.emplace_back(pos2, normal, uv2);
    }
    uvx+=0.1f;
  }

  std::array<std::pair<AnchorPoint, vec3>, 2> edgeAnchors{{
    { anchorPoints.front(), -anchorPoints.front().forward },
    { anchorPoints.back(),  +anchorPoints.back().forward  }
  }};
  auto extractAnchorX = [](const TrackPoint &p) { return p.x; };
  auto extractAnchorY = [](const TrackPoint &p) { return p.y; };
  float profileMinX = std::ranges::min_element(m_profile, std::less{}, extractAnchorX)->x;
  float profileMaxX = std::ranges::max_element(m_profile, std::less{}, extractAnchorX)->x;
  float profileMinY = std::ranges::min_element(m_profile, std::less{}, extractAnchorY)->y;
  float profileMaxY = std::ranges::max_element(m_profile, std::less{}, extractAnchorY)->y;

  for (const auto &[anchorPoint, normal] : edgeAnchors) {
    for (size_t j = 0; j < m_profile.size()-1; j++) {
      const TrackPoint &vertex = m_profile[j];
      vec3 p = anchorPoint.position + anchorPoint.right * vertex.x + anchorPoint.up * vertex.y;
      // uvs for edge vertices are in range x=0..1 y=l..1 (the bottom part of the texture) 
      rvec2 uv{
        mathf::inverseLerp(profileMinX, profileMaxX, vertex.x),
        mathf::inverseLerp(profileMinY, profileMaxY, vertex.y) * 20.f/120.f + 100.f/120.f // assuming track profile template uvs
      };
      rvec3 pos; XMStoreFloat3(&pos, p);
      rvec3 rnormal; XMStoreFloat3(&rnormal, normal);
      vertices.emplace_back(pos, rnormal, uv);
    }
  }

  return vertices;
}

std::vector<pbl::Mesh::index_t> Track::createMeshIndices(const std::vector<AnchorPoint>& anchorPoints) const
{
  using index_t = pbl::Mesh::index_t;
  std::vector<index_t> indices;

  index_t vertexPerTrackSection = static_cast<index_t>(m_profile.size()*2-2);

  for(int i = 0; i < static_cast<int>(anchorPoints.size()-1); i++) {
    for(int j = 0; j < static_cast<int>(vertexPerTrackSection-1); j += 2) {
      index_t offset = i * vertexPerTrackSection + j;
      indices.push_back(offset);
      indices.push_back(offset + 1 + vertexPerTrackSection);
      indices.push_back(offset + 1);
      indices.push_back(offset + 1 + vertexPerTrackSection);
      indices.push_back(offset);
      indices.push_back(offset + vertexPerTrackSection);
    }
  }

  // close the track
  // assuming the track profile template
  index_t links[] = { 0,1,2, 2,3,0, 3,4,0, 4,5,7, 5,6,7, 7,0,4 };
  index_t normalVertexCount = static_cast<index_t>(anchorPoints.size()*vertexPerTrackSection);
  index_t profileEndsVertexCount = static_cast<index_t>(m_profile.size()-1);
  for (size_t i = 0; i < std::size(links); i++) indices.push_back(normalVertexCount + links[i]);
  for (size_t i = 0; i < std::size(links); i++) indices.push_back(normalVertexCount + profileEndsVertexCount + links[std::size(links) - 1 - i]);

  return indices;
}
