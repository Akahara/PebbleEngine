#include "ribbon.h"

#include "directxlib.h"
#include "mesh.h"
#include "render_profiles.h"
#include "engine/windowsengine.h"
#include "utils/debug.h"

namespace pbl
{

static std::vector<Model::index_t> generateStripIndices(size_t stripCount)
{
  using index_t = Model::index_t;
  std::vector<index_t> indices;
  for(index_t i = 0; i < stripCount; i++) {
    indices.push_back(i*2+0);
    indices.push_back(i*2+1);
    indices.push_back(i*2+2);
    indices.push_back(i*2+1);
    indices.push_back(i*2+2);
    indices.push_back(i*2+3);
  }
  return indices;
}

Ribbon::Ribbon(size_t stripCount, Effect *effect)
  : m_effect(effect)
  , m_vbo(sizeof(RibbonVertex) * (stripCount+1)*2, GenericBuffer::BUFFER_VERTEX | GenericBuffer::FLAG_MUTABLE)
  , m_ibo(sizeof(Mesh::index_t) * (stripCount)*6, GenericBuffer::BUFFER_INDEX, generateStripIndices(stripCount).data())
{
  PBL_ASSERT(effect, "A ribbon effect cannot be null");

  m_pointsOrigins.reserve((stripCount+1)*2);
  m_vertices.reserve((stripCount+1)*2);
  for(size_t i = 0; i <= stripCount; i++) {
    m_pointsOrigins.emplace_back(0.f, static_cast<float>(i)/static_cast<float>(stripCount));
    m_pointsOrigins.emplace_back(1.f, static_cast<float>(i)/static_cast<float>(stripCount));
    float y = static_cast<float>(i)/(stripCount-2);
    m_vertices.push_back({ {0,y,0}, {0,y} });
    m_vertices.push_back({ {1,y,0}, {1,y} });
  }
}

void Ribbon::render(RenderContext &context)
{
  RenderProfiles::pushBlendProfile(BlendProfile::BLEND);
  RenderProfiles::pushDepthProfile(DepthProfile::TESTONLY_DEPTH);
  RenderProfiles::pushRasterProfile(RasterizerProfile::NOCULL_RASTERIZER);

  if(m_meshUpdated) {
    m_vbo.setRawData(m_vertices.data());
    m_meshUpdated = false;
  }

  context.bindTo(*m_effect);
  m_effect->bind();
  const UINT stride = sizeof(RibbonVertex);
  const UINT offset = 0;
  auto &d3context = WindowsEngine::d3dcontext();
  d3context.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  d3context.IASetVertexBuffers(0, 1, &m_vbo.getRawBuffer(), &stride, &offset);
  d3context.IASetIndexBuffer(m_ibo.getRawBuffer(), DXGI_FORMAT_R32_UINT, 0);
  d3context.DrawIndexed(static_cast<UINT>(m_pointsOrigins.size()-2) * 3, 0, 0);

  RenderProfiles::popDepthProfile();
  RenderProfiles::popRasterProfile();
  RenderProfiles::popBlendProfile();
}

void Ribbon::setPoints(std::vector<RibbonVertex> points)
{
  PBL_ASSERT(points.size() == m_vertices.size(), "Invalid number of ribbon points given");
  m_vertices = std::move(points);
  m_meshUpdated = true;
}

ShaderVertexLayout RibbonVertex::getVertexLayout()
{
  return ShaderVertexLayout{}
    .addField<float>("VERTEX_POSITION", 3)
    .addField<float>("VERTEX_TEXCOORD", 2);
}

}
