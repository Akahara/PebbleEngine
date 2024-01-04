#include "renderable.h"

#include "graphical_resource.h"
#include "shader.h"

namespace pbl
{

static struct GlobalResources {
  Effect *shadowPassEffect;
} *g_globalResources;

void RenderContext::bindTo(const Effect &effect) const
{
  for (auto &binding : constantBufferBindings)
    effect.bindBuffer(*binding.buffer, binding.bindingName);
  for (auto &binding : textureBindings)
    effect.bindTexture(binding.texture, binding.bindingName);
}

Effect *Renderable::getShadowPassEffect()
{
  return g_globalResources->shadowPassEffect;
}

void Renderable::loadGlobalResources(GraphicalResourceRegistry &resources)
{
  g_globalResources = new GlobalResources;
  g_globalResources->shadowPassEffect = resources.loadEffect(L"res/shaders/shadow.fx", BaseVertex::getShaderVertexLayout());
}

void Renderable::unloadGlobalResources()
{
  delete g_globalResources;
}


}
