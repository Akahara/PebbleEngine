#pragma once

#include "world/object.h"

class Tunnel : public pbl::WorldObject
{
private:
  static constexpr size_t MAXIMUM_LIGHTS_COUNT = 32;

  // beware of alignment when adding new members!
  struct Light {
    static constexpr uint32_t TYPE_POINTLIGHT = 0;
    static constexpr uint32_t TYPE_SPOTLIGHT = 1;

    uint32_t lightType;

    // all lights
    rvec3 aPosition;
    rvec3 aColor;
    // point lights
    float pStrength;
    // spot lights
    rvec3 sDirection;
    float sInnerAngle;
    float sOuterAngle;
    float sFalloffBegin;
    float sFalloffEnd;

    float __padding[1];
  };

  struct LightsConstantBuffer {
    uint32_t activeLights;
    float __padding[3];
    Light lights[MAXIMUM_LIGHTS_COUNT];
  };

public:
  explicit Tunnel(pbl::GraphicalResourceRegistry &resources);

  pbx::PhysicsBody *buildPhysicsObject() override;

  void render(pbl::RenderContext& context) override;

private:
  std::vector<std::pair<Transform, std::shared_ptr<pbl::Mesh>>> m_meshes;
  pbx::PhysicsBody   m_physicsBody;
  pbl::GenericBuffer m_lightsCBO;
  std::vector<Light> m_spotLights;
};
