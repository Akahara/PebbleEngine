#include "skybox.h"

#include "shader.h"
#include "directxlib.h"
#include "graphical_resource.h"
#include "render_profiles.h"
#include "engine/windowsengine.h"

namespace pbl
{

Effect *g_skyboxEffect;

struct SkyBoxConstants
{
  mat4 matViewProj;
  vec3 cameraPosition;
};

Skybox::Skybox(Cubemap cubemap)
  : m_cubemap(cubemap), m_constantBuffer(sizeof(SkyBoxConstants), GenericBuffer::BUFFER_CONSTANT)
{}

void Skybox::render(const Camera &camera)
{
  auto &d3context = WindowsEngine::d3dcontext();

  SkyBoxConstants constants{};
  constants.cameraPosition = camera.getPosition();
  constants.matViewProj = camera.getViewProjectionMatrix();
  m_constantBuffer.setData(constants);

  RenderProfiles::pushDepthProfile(DepthProfile::TESTONLY_DEPTH);
  d3context.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  g_skyboxEffect->bindSampler(TextureManager::getSampler(SamplerState::BASIC), "cubemapSampler");
  g_skyboxEffect->bindCubemap(m_cubemap, "cubemap");
  g_skyboxEffect->bindBuffer(m_constantBuffer, "cbWorld");
  g_skyboxEffect->bind();
  d3context.Draw(36, 0);
  RenderProfiles::popDepthProfile();
}

void Skybox::loadGlobalResources(GraphicalResourceRegistry &resources)
{
  g_skyboxEffect = resources.loadEffect(L"res/shaders/skybox.fx", ShaderVertexLayout{});
}

void Skybox::unloadGlobalResources()
{
}

}