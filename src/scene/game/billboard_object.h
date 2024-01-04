#pragma once

#include "display/sprite.h"
#include "world/object.h"

class BillboardsObject : public pbl::WorldObject
{
public:
  explicit BillboardsObject(pbl::GraphicalResourceRegistry &resources);
  explicit BillboardsObject(pbl::Effect *effect);

  void render(pbl::RenderContext& context) override {}
  void renderTransparent(pbl::RenderContext& context) override;

  std::vector<pbl::BillBoard> &getBillboards() { return m_spriteRenderer.getBillboards(); }

private:
  pbl::SpriteRenderer m_spriteRenderer;
};
