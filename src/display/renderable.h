#pragma once

#include <string>
#include <vector>

#include "camera.h"
#include "texture.h"

namespace pbl
{

class Effect;
class GenericBuffer;

struct ConstantBufferBinding
{
  const char *bindingName;
  GenericBuffer *buffer;
};

struct TextureBinding
{
  const char *bindingName;
  Texture     texture;
};

struct CubemapBinding
{
  const char *bindingName;
  Cubemap     cubemap;
};

struct SamplerBinding
{
  const char *bindingName;
  SamplerState sampler;
};

/*
 * A render context is the set of variables and effect bindings that will be passed
 * to WorldObjects::render(). The bindings must be bound by the world object, other
 * variables (such as the camera) can be used according to the object's needs.
 *
 * Note: for 3D objects, the world constant buffer is set by the scene and contains
 * camera properties. The object's render() method does not have to create it.
 */
struct RenderContext
{
  Camera camera;
  Frustum cameraFrustum;

  vec3 focusPosition; // the position of the player, for 3D rendering contexts

  std::vector<ConstantBufferBinding> constantBufferBindings;
  std::vector<TextureBinding> textureBindings;

  RenderContext() = default;
  explicit RenderContext(const Camera &camera)
    : camera(camera)
    , cameraFrustum(Frustum::createFrustumFromCamera(camera)) 
  {}

  void bindTo(const Effect &effect) const;
};

struct Renderable
{
  static Effect *getShadowPassEffect();

  static void loadGlobalResources(GraphicalResourceRegistry &resources);
  static void unloadGlobalResources();
};

}
