#include "render_profiles.h"

#include "directxlib.h"
#include "engine/windowsengine.h"
#include "utils/debug.h"

namespace pbl
{

std::stack<DepthProfile> RenderProfiles::s_depthProfileStack;
std::stack<BlendProfile> RenderProfiles::s_blendProfileStack;
std::stack<RasterizerProfile> RenderProfiles::s_rasterizerProfileStack;

static struct Profiles
{
  ID3D11DepthStencilState *depthTestEnabled     = nullptr;
  ID3D11DepthStencilState *depthTestDisabled    = nullptr;
  ID3D11DepthStencilState *depthTestOnly        = nullptr;

  ID3D11BlendState        *alphaBlendDisabled   = nullptr;
  ID3D11BlendState        *alphaBlendEnabled    = nullptr;

  ID3D11RasterizerState   *solidCullBackRS      = nullptr;
  ID3D11RasterizerState   *noCullBackRS         = nullptr;
} g_renderProfiles;

void RenderProfiles::pushBlendProfile(BlendProfile profile)
{
  PBL_ASSERT(s_blendProfileStack.size() < 10, "The blend profile stack is too large, did you forget to call popBlendProfile ?");
  s_blendProfileStack.push(profile);
  setActiveBlendProfile(profile);
}

void RenderProfiles::popBlendProfile()
{
  PBL_ASSERT(!s_blendProfileStack.empty(), "popBlendProfile() called one to many time");
  s_blendProfileStack.pop();
  setActiveBlendProfile(s_blendProfileStack.top());
}

void RenderProfiles::pushDepthProfile(DepthProfile profile)
{
  PBL_ASSERT(s_depthProfileStack.size() < 10, "The depth profile stack is too large, did you forget to call popDepthProfile ?");
  s_depthProfileStack.push(profile);
  setActiveDepthProfile(profile);
}

void RenderProfiles::popDepthProfile()
{
  PBL_ASSERT(!s_blendProfileStack.empty(), "popBlendProfile() called one to many time");
  s_depthProfileStack.pop();
  setActiveDepthProfile(s_depthProfileStack.top());
}

void RenderProfiles::pushRasterProfile(RasterizerProfile profile)
{
  PBL_ASSERT(s_rasterizerProfileStack.size() < 10, "The raster profile stack is too large, did you forget to call popRasterProfile ?");
  s_rasterizerProfileStack.push(profile);
  setActiveRasterProfile(profile);
}

void RenderProfiles::popRasterProfile()
{
  PBL_ASSERT(!s_blendProfileStack.empty(), "popRasterProfile() called one to many time");
  s_rasterizerProfileStack.pop();
  setActiveRasterProfile(s_rasterizerProfileStack.top());
}

static void initBlendProfiles();
static void initDepthProfiles();
static void initRasterizerProfiles();

void RenderProfiles::initProfiles()
{
  initBlendProfiles();
  initDepthProfiles();
  initRasterizerProfiles();

  pushBlendProfile(BlendProfile::NO_BLEND);
  pushDepthProfile(DepthProfile::TESTWRITE_DEPTH);
  pushRasterProfile(RasterizerProfile::CULLBACK_RASTERIZER);
}

static void initBlendProfiles()
{
  auto &device = WindowsEngine::d3ddevice();
  D3D11_BLEND_DESC blendDesc;
  ZeroMemory(&blendDesc, sizeof(D3D11_BLEND_DESC));
  blendDesc.RenderTarget[0].BlendEnable = TRUE;
  blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
  blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
  blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
  blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
  blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
  blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
  blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
  DXTry(device.CreateBlendState(&blendDesc, &g_renderProfiles.alphaBlendEnabled), "Could not create a blend state");
  blendDesc.RenderTarget[0].BlendEnable = FALSE;
  DXTry(device.CreateBlendState(&blendDesc,&g_renderProfiles.alphaBlendDisabled), "Could not create a blend state");
}

static void initDepthProfiles()
{
  auto &device = WindowsEngine::d3ddevice();

  D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
  ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));
  depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

  depthStencilDesc.DepthEnable = true;
  depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
  DXTry(device.CreateDepthStencilState(&depthStencilDesc, &g_renderProfiles.depthTestEnabled), "Could not create a depth stencil state");
  depthStencilDesc.DepthEnable = false;
  DXTry(device.CreateDepthStencilState(&depthStencilDesc, &g_renderProfiles.depthTestDisabled), "Could not create a depth stencil state");
  depthStencilDesc.DepthEnable = true;
  depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
  DXTry(device.CreateDepthStencilState(&depthStencilDesc, &g_renderProfiles.depthTestOnly), "Could not create a depth stencil state");
}

static void initRasterizerProfiles()
{
  auto &device = WindowsEngine::d3ddevice();

  D3D11_RASTERIZER_DESC rsDesc;
  ZeroMemory(&rsDesc, sizeof(D3D11_RASTERIZER_DESC));
  rsDesc.MultisampleEnable = true;

  rsDesc.FillMode = D3D11_FILL_SOLID;
  rsDesc.CullMode = D3D11_CULL_BACK;
  rsDesc.FrontCounterClockwise = false;
  DXTry(device.CreateRasterizerState(&rsDesc, &g_renderProfiles.solidCullBackRS), "Could not create a rasterizer state");
  rsDesc.CullMode = D3D11_CULL_NONE;
  DXTry(device.CreateRasterizerState(&rsDesc, &g_renderProfiles.noCullBackRS), "Could not create a rasterizer state");
}

void RenderProfiles::disposeProfiles()
{
  DXRelease(g_renderProfiles.solidCullBackRS);
  DXRelease(g_renderProfiles.noCullBackRS);
  DXRelease(g_renderProfiles.alphaBlendDisabled);
  DXRelease(g_renderProfiles.alphaBlendEnabled);
  DXRelease(g_renderProfiles.depthTestDisabled);
  DXRelease(g_renderProfiles.depthTestEnabled);
  DXRelease(g_renderProfiles.depthTestOnly);
}

void RenderProfiles::setActiveBlendProfile(BlendProfile profile)
{
  auto &context = WindowsEngine::d3dcontext();
  ID3D11BlendState *state = nullptr;
  switch (profile) {
  case BlendProfile::BLEND: state = g_renderProfiles.alphaBlendEnabled; break;
  case BlendProfile::NO_BLEND: state = g_renderProfiles.alphaBlendDisabled; break;
  }
  context.OMSetBlendState(state, nullptr, 0xffffffff);
}

void RenderProfiles::setActiveDepthProfile(DepthProfile profile)
{
  auto &context = WindowsEngine::d3dcontext();
  ID3D11DepthStencilState *state = nullptr;
  switch (profile) {
  case DepthProfile::TESTWRITE_DEPTH: state = g_renderProfiles.depthTestEnabled; break;
  case DepthProfile::TESTONLY_DEPTH: state = g_renderProfiles.depthTestOnly; break;
  case DepthProfile::NO_DEPTH: state = g_renderProfiles.depthTestDisabled; break;
  }
  context.OMSetDepthStencilState(state, 0);
}

void RenderProfiles::setActiveRasterProfile(RasterizerProfile profile)
{
  auto &context = WindowsEngine::d3dcontext();
  ID3D11RasterizerState *state = nullptr;
  switch (profile) {
  case RasterizerProfile::CULLBACK_RASTERIZER: state = g_renderProfiles.solidCullBackRS; break;
  case RasterizerProfile::NOCULL_RASTERIZER: state = g_renderProfiles.noCullBackRS; break;
  }
  context.RSSetState(state);
}

}
