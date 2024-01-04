#pragma once

#include "generic_buffer.h"
#include "renderable.h"
#include "shader.h"

namespace pbl
{

struct RibbonConstantBuffer {
  mat4 vObjectMatWorld;
};

struct RibbonVertex {
  rvec3 position;
  rvec2 uv;

  static ShaderVertexLayout getVertexLayout();
};
  
class Ribbon
{
public:
  Ribbon(size_t stripCount, Effect *effect);

  void render(RenderContext &context);

  void setPoints(std::vector<RibbonVertex> points);

  const std::vector<RibbonVertex> &getPoints() const { return m_vertices; }

private:
  bool m_meshUpdated = true;
  std::vector<rvec2> m_pointsOrigins;
  std::vector<RibbonVertex> m_vertices;
  Effect *m_effect;
  GenericBuffer m_vbo;
  GenericBuffer m_ibo;
};

}
