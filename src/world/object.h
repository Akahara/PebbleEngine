#pragma once

#include <memory>
#include <optional>

#include "display/mesh.h"
#include "physics/physics.h"
#include "transform.h"

namespace physx
{
class PxTriangleMesh;
}

namespace pbl
{

struct ObjectConstantData
{
  mat4 matWorld;
};

struct WorldUniforms
{
  mat4 vWorldMatViewProj;
  mat4 vWorldMatShadowViewProj;
  vec3 vWorldLightDir;
  vec3 vWorldCameraPos;
  vec4 vWorldAmbiantLight;
  vec4 vWorldDiffuseLight;
  vec4 vWorldSpecularLight;
  float vWorldTime;
  float __padding[3];
};

using layer_t = uint8_t;
namespace Layer
{
constexpr layer_t
  DEFAULT     = 0,
  PLAYER      = 1 << 1,
  TRIGGER     = 1 << 2,
  STICKY_ROAD = 1 << 3,
  BOOST_PLATE = 1 << 4;
};

/*
 * WorldObject is the base building block of scenes.
 * It has a transform, and can be updated and rendered. For static visual
 * objects see WorldProp.
 */
class WorldObject
{
public:
  WorldObject() = default;
  virtual ~WorldObject() = default;

  WorldObject(const WorldObject &) = delete;
  WorldObject &operator=(const WorldObject &) = delete;
  WorldObject(WorldObject &&) = delete;
  WorldObject &operator=(WorldObject &&) = delete;

  virtual void render(RenderContext &context) = 0;
  virtual void renderTransparent(RenderContext &context) {}
  virtual void renderShadows(RenderContext &context) {}
  virtual void update(double delta) {}
  /*
   * Builds a physics body for this object, if this object has no physics associated with it
   * a nullptr is returned. The PhysicsBody's lifetime must be garantied by the WorldObject.
   * When in the editor this function will be called at the start of each live session, the
   * PhysicsBody returned by the WorldObject implementation must be updated according to the
   * changes made in the editor each time (a full rebuild each time is possible but costly).
   */
  virtual pbx::PhysicsBody *buildPhysicsObject() { return nullptr; }

  Transform &getTransform() { return m_transform; }
  const Transform &getTransform() const { return m_transform; }
  void setTransform(const Transform &transform) { m_transform = transform; }

  layer_t getLayer() const { return m_layer; }
  void setLayer(layer_t layer) { m_layer = layer; }

  static void loadGlobalResources();
  static void unloadGlobalResources();

protected:
  Transform m_transform;
  layer_t m_layer = Layer::DEFAULT;

  static std::shared_ptr<GenericBuffer> s_objectConstantBuffer;
};

class WorldProp : public WorldObject
{
public:
  explicit WorldProp(const std::shared_ptr<Mesh> &mesh)
    : m_mesh(mesh)
  {}
  WorldProp() = default;

  static std::unique_ptr<WorldProp> makeObjectFromFile(GraphicalResourceRegistry &resources, const std::wstring &meshFilePath);
  static std::unique_ptr<WorldProp> makePhysicsfullObjectFromFile(GraphicalResourceRegistry &resources, const std::wstring &meshFilePath, const std::wstring &physicsMeshFilePath, const std::wstring &effectFilePath);
  static std::unique_ptr<WorldProp> makePhysicsfullObjectFromFile(GraphicalResourceRegistry &resources, const std::wstring &meshFilePath, const std::wstring &effectFilePath = {})
  { return makePhysicsfullObjectFromFile(resources, meshFilePath, meshFilePath, effectFilePath); }
  static physx::PxTriangleMesh *makePhysicsMeshFromModel(const Model &model);

  void render(RenderContext &context) override;
  void renderShadows(RenderContext &context) override;

protected:
  void setMesh(const std::shared_ptr<Mesh> &mesh) { m_mesh = mesh; }
  void setPhysics(pbx::PhysicsBody &&body) { m_body = std::move(body); }

  pbx::PhysicsBody *buildPhysicsObject() override;

protected:
  std::shared_ptr<Mesh> m_mesh;
  pbx::PhysicsBody m_body;
};

}
