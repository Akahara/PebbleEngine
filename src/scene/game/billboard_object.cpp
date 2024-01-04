#include "billboard_object.h"

BillboardsObject::BillboardsObject(pbl::GraphicalResourceRegistry &resources)
  : m_spriteRenderer(resources)
{
}

BillboardsObject::BillboardsObject(pbl::Effect *effect)
  : m_spriteRenderer(nullptr, effect)
{
}

void BillboardsObject::renderTransparent(pbl::RenderContext &context)
{
  pbl::RenderContext cleanContext{ context.camera };
  m_spriteRenderer.renderBillboards(cleanContext);
}
