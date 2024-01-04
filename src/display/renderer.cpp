#include "renderer.h"

#include <vector>

#include "directxlib.h"
#include "generic_buffer.h"
#include "render_profiles.h"
#include "graphical_resource.h"
#include "utils/aabb.h"
#include "utils/bezier_curve.h"
#include "engine/windowsengine.h"

namespace pbl::renderer
{

struct LineVertex {
  rvec4 position;
  rvec4 color;

  static ShaderVertexLayout getLayout() {
    return ShaderVertexLayout{}
      .addField<float>("VERTEX_POSITION", 4)
      .addField<float>("VERTEX_COLOR", 4);
  }
};

struct LinesWorldConstantBuffer {
  mat4 matViewProj;
};

struct CubeData {
  mat4 transformMat;
  rvec4 color;
};

struct CubeConstantBuffer {
  mat4 matViewProj;
  mat4 transformMat;
  rvec4 color;
};

struct GlobalResources {
  static constexpr size_t maxLinesVerticesCount = 500;
  std::vector<LineVertex> linesVertices;
  Effect                 *linesEffect;
  GenericBuffer           linesWorldCB{ sizeof(LinesWorldConstantBuffer), GenericBuffer::BUFFER_CONSTANT };
  GenericBuffer           linesVBO{ sizeof(LineVertex) * maxLinesVerticesCount, GenericBuffer::BUFFER_VERTEX | GenericBuffer::FLAG_MUTABLE };
  std::vector<CubeData>   cubes;
  Effect                 *cubeEffect;
  GenericBuffer           cubeCB{ sizeof(CubeConstantBuffer), GenericBuffer::BUFFER_CONSTANT };

  static_assert(maxLinesVerticesCount % 2 == 0);
} *g_globalResources;

unsigned colorFromVector(const vec3& v)
{
  constexpr vec3 min{0,0,0,0};
  constexpr float m = (float)(0xff-1)/(0xff);
  constexpr vec3 max{m,m,m,m};
  return static_cast<unsigned>(XMVectorGetX(XMVector4Dot(XMVectorClamp(v, min, max), { 0xff, 0xff<<8, 0xff<<16, 0xff<<24 })));
}

rvec4 float4FromColor(unsigned color)
{
  float m = 0xff;
  return {
    static_cast<float>(color>>0 &0xff)/m,
    static_cast<float>(color>>8 &0xff)/m,
    static_cast<float>(color>>16&0xff)/m,
    static_cast<float>(color>>24&0xff)/m,
  };
}

void loadGlobalResources(GraphicalResourceRegistry &resources)
{
  g_globalResources = new GlobalResources;
  g_globalResources->linesEffect = resources.loadEffect(L"res/shaders/lines.fx", LineVertex::getLayout());
  g_globalResources->cubeEffect = resources.loadEffect(L"res/shaders/cube.fx", ShaderVertexLayout{});
}

void unloadGlobalResources()
{
  delete g_globalResources;
}

void renderLine(rvec3 p0, rvec3 p1, unsigned color)
{
  auto col = float4FromColor(color);
  g_globalResources->linesVertices.emplace_back(rvec4{ p0.x, p0.y, p0.z, 1 }, col);
  g_globalResources->linesVertices.emplace_back(rvec4{ p1.x, p1.y, p1.z, 1 }, col);
}

void renderLine(vec3 p0, vec3 p1, unsigned color)
{
  rvec3 v0, v1;
  XMStoreFloat3(&v0, p0);
  XMStoreFloat3(&v1, p1);
  renderLine(v0, v1, color);
}

void renderAABB(const AABB& aabb, unsigned color)
{
  vec3 o = aabb.getOrigin();
  vec3 x = { XMVectorGetX(aabb.getSize()), 0, 0 };
  vec3 y = { 0, XMVectorGetY(aabb.getSize()), 0 };
  vec3 z = { 0, 0, XMVectorGetZ(aabb.getSize()) };
  renderLine(o, o + x, color);
  renderLine(o + y, o + x + y, color);
  renderLine(o + z, o + x + z, color);
  renderLine(o + y + z, o + x + y + z, color);
  renderLine(o, o + y, color);
  renderLine(o + x, o + y + x, color);
  renderLine(o + z, o + y + z, color);
  renderLine(o + x + z, o + y + x + z, color);
  renderLine(o, o + z, color);
  renderLine(o + x, o + z + x, color);
  renderLine(o + y, o + z + y, color);
  renderLine(o + x + y, o + z + x + y, color);
}

void renderCube(mat4 transformMat, unsigned color)
{
  g_globalResources->cubes.push_back({ std::move(transformMat), float4FromColor(color) });
}

void renderRect(mat4 transformMat, unsigned color)
{
  g_globalResources->cubes.push_back({ std::move(transformMat), float4FromColor(color) });
}

void renderAxes(const Transform &transform)
{
  renderLine(transform.position, transform.position + transform.getForward(), 0xffff0000);
  renderLine(transform.position, transform.position + transform.getUp(),      0xff00ff00);
  renderLine(transform.position, transform.position + transform.getRight(),   0xff0000ff);
}

static void renderCameraOutline(const Camera &outlinedCamera, const OrthographicProjection &proj)
{
  vec3 pos = outlinedCamera.getPosition();
  vec3 I = outlinedCamera.getRight();
  vec3 J = outlinedCamera.getUp();
  vec3 F = outlinedCamera.getForward();
  float zNear = proj.zNear;
  float zFar = proj.zFar;
  vec3 p1 = pos + I * proj.width*.5f + J * proj.height*.5f;
  vec3 p2 = pos + I * proj.width*.5f - J * proj.height*.5f;
  vec3 p3 = pos - I * proj.width*.5f - J * proj.height*.5f;
  vec3 p4 = pos - I * proj.width*.5f + J * proj.height*.5f;

  unsigned int color = 0xff878282;

  renderLine(p1, p1 + F * zFar, color);
  renderLine(p2, p2 + F * zFar, color);
  renderLine(p3, p3 + F * zFar, color);
  renderLine(p4, p4 + F * zFar, color);
  for (float z = zNear; z < zFar; z += 5.f) {
    renderLine(p1 + z * F, p2 + z * F, color);
    renderLine(p2 + z * F, p3 + z * F, color);
    renderLine(p3 + z * F, p4 + z * F, color);
    renderLine(p4 + z * F, p1 + z * F, color);
  }
  mat4 tMat = DirectX::XMMatrixTranslationFromVector(pos);
  mat4 sMat = DirectX::XMMatrixScaling(.5f, .5f, .5f);
  renderCube(DirectX::XMMatrixMultiply(sMat, tMat), color);
}

static void renderCameraOutline(const Camera &outlinedCamera, const PerspectiveProjection &proj)
{
  vec3 pos = outlinedCamera.getPosition();
  vec3 I = outlinedCamera.getRight();
  vec3 J = outlinedCamera.getUp();
  vec3 F = outlinedCamera.getForward();
  float dh = tanf(proj.fovy * .5f);
  float dw = dh * proj.aspect;
  vec3 U1 = F + dh * J + dw * I;
  vec3 U2 = F - dh * J + dw * I;
  vec3 U3 = F - dh * J - dw * I;
  vec3 U4 = F + dh * J - dw * I;
  float zNear = proj.zNear;
  float zFar = proj.zFar;

  unsigned int color = 0xff878282;

  renderLine(pos, pos + U1 * zFar, color);
  renderLine(pos, pos + U2 * zFar, color);
  renderLine(pos, pos + U3 * zFar, color);
  renderLine(pos, pos + U4 * zFar, color);
  renderLine(pos + U1 * zFar, pos + U2 * zFar, color);
  renderLine(pos + U2 * zFar, pos + U3 * zFar, color);
  renderLine(pos + U3 * zFar, pos + U4 * zFar, color);
  renderLine(pos + U4 * zFar, pos + U1 * zFar, color);
  for (float z = zNear; z < zFar; z += 5.f) {
    renderLine(pos + U1 * z, pos + U2 * z, color);
    renderLine(pos + U2 * z, pos + U3 * z, color);
    renderLine(pos + U3 * z, pos + U4 * z, color);
    renderLine(pos + U4 * z, pos + U1 * z, color);
  }
  mat4 tMat = XMMatrixTranslationFromVector(pos);
  mat4 sMat = DirectX::XMMatrixScaling(.5f, .5f, .5f);
  renderCube(XMMatrixMultiply(sMat, tMat), color);
}

void renderCameraOutline(const Camera& camera)
{
  std::visit([&](const auto &proj) { renderCameraOutline(camera, proj); }, camera.getProjection());
}

void renderWorldAxes(const Camera& camera)
{
  vec3 p = camera.getPosition() + camera.getForward();
  renderLine(p, p + .1f*Camera::FORWARD, 0xffff0000);
  renderLine(p, p + .1f*Camera::UP,      0xff00ff00);
  renderLine(p, p + .1f*Camera::RIGHT,   0xff0000ff);
}

void renderCurve(const BezierCurve& bezier, unsigned int color)
{
  if (bezier.controlPoints.size() <= 1) return;
  constexpr float step = .1f;
  vec3 p1 = bezier.controlPoints[0].position;
  for(float t = step; t < static_cast<float>(bezier.controlPoints.size())-1.f; t += step) {
    vec3 p2 = bezier.samplePoint(t);
    renderLine(p1, p2, color);
    p1 = p2;
  }
  renderLine(p1, bezier.controlPoints.back().position, color);
  for (const BezierControlPoint &p : bezier.controlPoints)
  {
      mat4 tMat = XMMatrixTranslationFromVector(p.position);
      mat4 sMat = DirectX::XMMatrixScaling(.1f, .1f, .1f);
      renderCube(XMMatrixMultiply(sMat, tMat), color);
  }
}

void renderCurve(const DiscreteCurve &curve, unsigned color)
{
  constexpr vec3 s{ .1f, .1f, .1f };
  for (size_t i = 0; i+1 < curve.points.size(); i++)
    renderLine(curve.points[i], curve.points[i+1], color);
  for (size_t i = 0; i < curve.points.size(); i++)
    renderAABB(AABB{ curve.points[i]-s*.5f, s }, color);
}

void sendDebugDraws(const Camera &camera)
{
  auto &context = WindowsEngine::d3dcontext();

  { // draw lines
    auto &linesVertices = g_globalResources->linesVertices;
    linesVertices.reserve(mathf::ceilDivisibleBy(linesVertices.size(), GlobalResources::maxLinesVerticesCount));

    LinesWorldConstantBuffer worldCB;
    worldCB.matViewProj = camera.getViewProjectionMatrix();
    g_globalResources->linesWorldCB.setRawData(&worldCB);
    g_globalResources->linesEffect->bindBuffer(g_globalResources->linesWorldCB, "cbWorld");
    g_globalResources->linesEffect->bind();

    UINT stride = sizeof(LineVertex), offset = 0;
    context.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    context.IASetVertexBuffers(0, 1, &g_globalResources->linesVBO.getRawBuffer(), &stride, &offset);

    RenderProfiles::pushDepthProfile(DepthProfile::NO_DEPTH);
    for (size_t i = 0; i < linesVertices.size(); i += GlobalResources::maxLinesVerticesCount) {
      g_globalResources->linesVBO.setRawData(linesVertices.data() + i);
      context.Draw(static_cast<UINT>(std::min(GlobalResources::maxLinesVerticesCount, linesVertices.size() - i)), 0);
    }
    RenderProfiles::popDepthProfile();

    linesVertices.clear();
  }

  { // draw cubes
    auto &cubes = g_globalResources->cubes;
    CubeConstantBuffer worldCB;
    worldCB.matViewProj = camera.getViewProjectionMatrix();
    g_globalResources->cubeEffect->bindBuffer(g_globalResources->cubeCB, "cbCube");
    g_globalResources->cubeEffect->bind();
    context.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    RenderProfiles::pushBlendProfile(BlendProfile::BLEND);
    for(auto &cube : cubes) {
      worldCB.color    = cube.color;
      worldCB.transformMat = cube.transformMat;
      g_globalResources->cubeCB.setData(worldCB);
      context.Draw(36, 0);
    }
    RenderProfiles::popBlendProfile();
    cubes.clear();
  }
}

}

