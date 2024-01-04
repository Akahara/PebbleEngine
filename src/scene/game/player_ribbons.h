#pragma once

#include "player_vehicle.h"
#include "display/ribbon.h"
#include "world/object.h"

class FollowingRibbon
{
public:
  FollowingRibbon(size_t stripCount, pbl::Effect *effect, const vec3 &initialLeft, const vec3 &initialRight);

  void pushVertices(const vec3 &left, const vec3 &right, float alpha);
  void render(pbl::RenderContext &context);

private:
  std::deque<pbl::RibbonVertex> m_previousVertices;
  pbl::Ribbon m_ribbon;
};

class PlayerRibbons : public pbl::WorldObject
{
public:
  explicit PlayerRibbons(pbl::GraphicalResourceRegistry &resources);

  // must be called before the first call to render()
  void setPlayerVehicle(PlayerVehicle *vehicle);

  void render(pbl::RenderContext& context) override {}
  void renderTransparent(pbl::RenderContext& context) override;
  void update(double delta) override;

private:
  struct PlayerFollowingRibbon {
    rvec3 leftOffset, rightOffset;
    rvec4 sourceColor;
  };

private:
  std::vector<std::pair<PlayerFollowingRibbon, FollowingRibbon>> m_ribbons;
  PlayerVehicle     *m_playerVehicle;
  pbl::Effect       *m_wheelsEffect;
  pbl::GenericBuffer m_ribbonCB;
  pbl::GenericBuffer m_trailCB;
};
