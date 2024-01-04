#include "player_ribbons.h"

#include "player.h"
#include "display/graphical_resource.h"

struct TrailConstantBuffer {
  rvec4 vTrailSourceColor;
};

FollowingRibbon::FollowingRibbon(size_t stripCount, pbl::Effect *effect, const vec3 &initialLeft, const vec3 &initialRight)
  : m_ribbon(stripCount, effect)
{
  for (size_t i = 0; i < m_ribbon.getPoints().size(); i += 2) {
    m_previousVertices.push_back({ {0,0,0}, {0,0} });
    m_previousVertices.push_back({ {0,0,0}, {1,0} });
  }
}

void FollowingRibbon::pushVertices(const vec3 &left, const vec3 &right, float alpha)
{
  m_previousVertices.pop_back();
  m_previousVertices.pop_back();
  rvec3 lp, rp;
  XMStoreFloat3(&lp, left);
  XMStoreFloat3(&rp, right);
  m_previousVertices.push_front({ lp, {0,alpha} });
  m_previousVertices.push_front({ rp, {1,alpha} });
}

void FollowingRibbon::render(pbl::RenderContext &context)
{
  m_ribbon.setPoints({ m_previousVertices.begin(), m_previousVertices.end() });
  m_ribbon.render(context);
}

PlayerRibbons::PlayerRibbons(pbl::GraphicalResourceRegistry &resources)
  : m_ribbonCB(sizeof(pbl::RibbonConstantBuffer), pbl::GenericBuffer::BUFFER_CONSTANT)
  , m_trailCB(sizeof(TrailConstantBuffer), pbl::GenericBuffer::BUFFER_CONSTANT)
  , m_playerVehicle(nullptr)
  , m_wheelsEffect(resources.loadEffect(L"res/shaders/ribbon_vehicle_trails.fx", pbl::RibbonVertex::getVertexLayout()))
{
  pbl::RibbonConstantBuffer constantBuffer{ Transform{}.getWorldMatrix() };
  m_ribbonCB.setData(constantBuffer);
}

void PlayerRibbons::setPlayerVehicle(PlayerVehicle *vehicle)
{
  m_playerVehicle = vehicle;

  std::vector<PlayerFollowingRibbon> ribbons{{
    { rvec3{  .57f, -.62f, -1.6f }, rvec3{  .94f, -.62f, 1.63f }, rvec4{ 0.224f, 0.306f, 0.698f, .6f } },
    { rvec3{ -.57f, -.61f, -1.6f }, rvec3{ -.94f, -.61f, 1.63f }, rvec4{ 0.224f, 0.306f, 0.698f, .6f } },
    { rvec3{ -.7f, .51f, -1.6f }, rvec3{ +.7f, .51f, -1.6f }, rvec4{ 0.557f, 0.557f, 0.557f, .6f } },
  }};

  const Transform &vehicleTransform = m_playerVehicle->getTransform();
  vec3 right = vehicleTransform.getRight();
  vec3 forward = vehicleTransform.getForward();
  vec3 up = vehicleTransform.getUp();

  for(PlayerFollowingRibbon &ribbon : ribbons) {
    m_ribbons.emplace_back(ribbon, FollowingRibbon(200, m_wheelsEffect,
      vehicleTransform.position + right * ribbon.leftOffset.x  + up * ribbon.leftOffset.y  + forward * ribbon.leftOffset.z,
      vehicleTransform.position + right * ribbon.rightOffset.x + up * ribbon.rightOffset.y + forward * ribbon.rightOffset.z));
  }
}

void PlayerRibbons::renderTransparent(pbl::RenderContext &context)
{
  context.constantBufferBindings.emplace_back("cbObject", &m_ribbonCB);
  context.constantBufferBindings.emplace_back("cbTrail", &m_trailCB);
  TrailConstantBuffer constantBuffer;
  for (auto &[ribbonConfig, ribbon] : m_ribbons) {
    constantBuffer.vTrailSourceColor = ribbonConfig.sourceColor;
    m_trailCB.setData(constantBuffer);
    ribbon.render(context);
  }
  context.constantBufferBindings.pop_back();
  context.constantBufferBindings.pop_back();
}

void PlayerRibbons::update(double delta)
{
  const Transform &vehicleTransform = m_playerVehicle->getTransform();
  vec3 right = vehicleTransform.getRight();
  vec3 forward = vehicleTransform.getForward();
  vec3 up = vehicleTransform.getUp();

  float playerSpeed = m_playerVehicle->getFrame()->getLinearVelocity().magnitude() / Player::MAX_SPEED;
  float ribbonAlpha = mathf::smoothstep(.6f, 1.f, playerSpeed);

  for (auto &[ribbonConfig, ribbon] : m_ribbons) {
    ribbon.pushVertices(
      vehicleTransform.position + right * ribbonConfig.leftOffset.x  + up * ribbonConfig.leftOffset.y  + forward * ribbonConfig.leftOffset.z,
      vehicleTransform.position + right * ribbonConfig.rightOffset.x + up * ribbonConfig.rightOffset.y + forward * ribbonConfig.rightOffset.z,
      ribbonAlpha);
  }
}

