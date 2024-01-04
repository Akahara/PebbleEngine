#include "sprite.h"

#include "directxlib.h"
#include "engine/windowsengine.h"
#include "render_profiles.h"
#include "graphical_resource.h"

namespace pbl
{

struct ScreenConstantBuffer
{
  rvec2 vScreenSize;
  float __padding[2];
};

struct SpriteVertex
{
  rvec2 position;
  rvec2 texCoord;
};

struct SpriteInstance
{
  rvec2 position;
  rvec2 scale;
  rvec4 color;
  rvec4 texCoord;
  uint32_t spriteType; // 0-default  1-distance field (used by text)
  float __padding[3];
};

struct BillboardInstance
{
  rvec3 position;
  rvec2 scale;
  rvec4 texCoord;
  float __padding[3];
};

struct BillboardConstantBuffer
{
  mat4 vMatViewProj;
  vec3 vCameraUp;
  vec3 vCameraRight;
};

const ShaderVertexLayout &SpriteRenderer::getSpriteVertexLayout()
{
  static auto layout = ShaderVertexLayout{}
    .addField<float>("INSTANCE_POSITION", 2, ShaderVertexLayout::INSTANCE)
    .addField<float>("INSTANCE_SCALE", 2, ShaderVertexLayout::INSTANCE)
    .addField<float>("INSTANCE_COLOR", 4, ShaderVertexLayout::INSTANCE)
    .addField<float>("INSTANCE_TEXCOORD", 4, ShaderVertexLayout::INSTANCE)
    .addField<uint32_t>("INSTANCE_SPRITETYPE", 1, ShaderVertexLayout::INSTANCE)
    ;
  return layout;
}

const ShaderVertexLayout &SpriteRenderer::getBillboardVertexLayout()
{
  static auto layout = ShaderVertexLayout{}
    .addField<float>("INSTANCE_POSITION", 3, ShaderVertexLayout::INSTANCE)
    .addField<float>("INSTANCE_SCALE", 2, ShaderVertexLayout::INSTANCE)
    .addField<float>("INSTANCE_TEXCOORD", 4, ShaderVertexLayout::INSTANCE)
    ;
  return layout;
}

SpriteRenderer::SpriteRenderer(GraphicalResourceRegistry &resources)
  : SpriteRenderer(
    resources.loadEffect(L"res/shaders/sprite.fx", getSpriteVertexLayout()),
    resources.loadEffect(L"res/shaders/billboard.fx", getBillboardVertexLayout()))
{
}

SpriteRenderer::SpriteRenderer(Effect *spriteEffect, Effect *billboardEffect)
  : m_spriteEffect(spriteEffect)
  , m_billboardEffect(billboardEffect)
  , m_screenConstantBuffer(sizeof(ScreenConstantBuffer), GenericBuffer::BUFFER_CONSTANT)
{
  if (m_spriteEffect) {
    m_spriteInstanceBuffer = GenericBuffer(sizeof(SpriteInstance) * DRAW_CALL_INSTANCE_CAPACITY, GenericBuffer::BUFFER_INSTANCE);
    m_spriteEffect->bindSampler(TextureManager::getSampler(SamplerState::BASIC), "spriteSampler");
  }
  if (m_billboardEffect) {
    m_billboardInstanceBuffer = GenericBuffer(sizeof(SpriteInstance) * DRAW_CALL_INSTANCE_CAPACITY, GenericBuffer::BUFFER_INSTANCE);
    m_billboardConstantBuffer = GenericBuffer(sizeof(BillboardConstantBuffer), GenericBuffer::BUFFER_CONSTANT);
    m_billboardEffect->bindSampler(TextureManager::getSampler(SamplerState::BASIC), "billboardSampler");
  }
}

void SpriteRenderer::renderSprites(RenderContext &context)
{
  if (m_sprites.empty()) return;

  ScreenConstantBuffer screenCB;
  screenCB.vScreenSize = { ScreenRegion::SCREEN_WIDTH, ScreenRegion::SCREEN_HEIGHT };
  m_screenConstantBuffer.setData(screenCB);

  UINT instanceStride = sizeof(SpriteInstance);
  UINT vboOffset = 0;
  auto &d3dcontext = WindowsEngine::d3dcontext();
  d3dcontext.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  RenderProfiles::pushBlendProfile(BlendProfile::BLEND);
  RenderProfiles::pushDepthProfile(DepthProfile::NO_DEPTH);
  RenderProfiles::pushRasterProfile(RasterizerProfile::NOCULL_RASTERIZER);

  SpriteInstance sprites[DRAW_CALL_INSTANCE_CAPACITY];

  d3dcontext.IASetVertexBuffers(1, 1, &m_spriteInstanceBuffer.getRawBuffer(), &instanceStride, &vboOffset);
  context.bindTo(*m_spriteEffect);
  m_spriteEffect->bindBuffer(m_screenConstantBuffer, "cbScreen");

  auto sendDrawCall = [&](size_t instanceCount, const Texture &texture) {
    m_spriteEffect->bindTexture(texture, "spriteTexture");
    m_spriteInstanceBuffer.setRawData(&sprites[0]);
    m_spriteEffect->bind();
    d3dcontext.DrawInstanced(6, (UINT)instanceCount, 0, 0);
  };

  Texture nextBatchTexture = m_sprites[0].texture;
  size_t nextInstancePosition = 0;

  for (Sprite &sprite : m_sprites) {
    if (sprite.texture != nextBatchTexture || nextInstancePosition >= DRAW_CALL_INSTANCE_CAPACITY) {
      sendDrawCall(nextInstancePosition, nextBatchTexture);
      nextInstancePosition = 0;
    }
    SpriteInstance &spriteInstance = sprites[nextInstancePosition++];
    spriteInstance.position     = { sprite.region.minX, sprite.region.minY };
    spriteInstance.scale        = { sprite.region.width(), sprite.region.height() };
    spriteInstance.color        = sprite.color;
    spriteInstance.texCoord     = { sprite.texX, sprite.texY, sprite.texW, sprite.texH };
    spriteInstance.spriteType   = static_cast<unsigned int>(sprite.spriteType);
    nextBatchTexture            = sprite.texture;
  }
  if (nextInstancePosition != 0)
    sendDrawCall(nextInstancePosition, nextBatchTexture);

  RenderProfiles::popDepthProfile();
  RenderProfiles::popBlendProfile();
  RenderProfiles::popRasterProfile();
}

void SpriteRenderer::renderBillboards(RenderContext &context)
{
  if (m_billboards.empty()) return;

  UINT instanceStride = sizeof(SpriteInstance);
  UINT vboOffset = 0;
  auto &d3dcontext = WindowsEngine::d3dcontext();
  d3dcontext.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  RenderProfiles::pushBlendProfile(BlendProfile::BLEND);

  BillboardInstance billboards[DRAW_CALL_INSTANCE_CAPACITY];

  BillboardConstantBuffer billboardCB;
  billboardCB.vMatViewProj = XMMatrixTranspose(context.camera.getViewProjectionMatrix());
  billboardCB.vCameraRight = context.camera.getRight();
  billboardCB.vCameraUp = context.camera.getUp();
  m_billboardConstantBuffer.setData(billboardCB);
  m_billboardEffect->bindBuffer(m_billboardConstantBuffer, "cbBillboard");
  context.bindTo(*m_billboardEffect);
  m_billboardEffect->bind();
  d3dcontext.IASetVertexBuffers(1, 1, &m_billboardInstanceBuffer.getRawBuffer(), &instanceStride, &vboOffset);

  auto sendDrawCall = [&](size_t instanceCount, const Texture &texture) {
    m_billboardEffect->bindTexture(texture, "billboardTexture");
    m_billboardInstanceBuffer.setRawData(&billboards[0]);
    m_billboardEffect->bind();
    d3dcontext.DrawInstanced(6, (UINT)instanceCount, 0, 0);
  };

  Texture nextBatchTexture = m_billboards[0].texture;
  size_t nextInstancePosition = 0;

  for (BillBoard &billboard : m_billboards) {
    if (billboard.texture != nextBatchTexture || nextInstancePosition >= DRAW_CALL_INSTANCE_CAPACITY) {
      sendDrawCall(nextInstancePosition, nextBatchTexture);
      nextInstancePosition = 0;
    }
    BillboardInstance &billboardInstance = billboards[nextInstancePosition++];
    XMStoreFloat3(&billboardInstance.position, billboard.position);
    billboardInstance.scale        = billboard.scale;
    billboardInstance.texCoord     = { billboard.texX, billboard.texY, billboard.texW, billboard.texH };
    nextBatchTexture               = billboard.texture;
  }
  if (nextInstancePosition != 0)
    sendDrawCall(nextInstancePosition, nextBatchTexture);

  RenderProfiles::popBlendProfile();
}

}