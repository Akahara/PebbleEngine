#pragma once

#include "display/frame_buffer.h"
#include "display/sprite.h"
#include "world/object.h"

class Sun : public pbl::WorldObject
{
public:
  Sun(pbl::GraphicalResourceRegistry &resources, pbl::SpriteRenderer *sprites);

  void update(double delta) override;
  void render(pbl::RenderContext &context) override;
  const vec3 &getSunDirection() const { return m_direction; }
  const mat4 &getSunViewProjectionMatrix() const { return m_camera.getViewProjectionMatrix(); }

  pbl::Texture renderDepthTexture(const std::vector<std::shared_ptr<WorldObject>> &shadowCastingObjects, vec3 primaryTarget);

private:
  static constexpr float WORLD_DISTANCE = 100000.f;
  static constexpr float WORLD_SIZE = 1.f;
  pbl::Camera          m_camera;
  pbl::GenericBuffer   m_worldConstantBuffer;
  pbl::FrameBuffer     m_depthFBO;
  pbl::SpriteRenderer *m_sprites;
  pbl::Texture         m_texture;
  float                m_angle{};
  vec3                 m_direction;
};
