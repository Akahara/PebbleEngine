#include "game_vfx.h"

#include "player.h"
#include "display/graphical_resource.h"

// camera shake is triggered when the player comes back to the ground after a certain airtime
// TODO the current way of detecting airtime is janky in that if the player lands bottom-face-up they
// will still be considered airborn
static constexpr float CAMERA_SHAKE_MIN_AIRTIME = 1.f;
static constexpr float CAMERA_SHAKE_DURATION = .3f;
static constexpr float CAMERA_SHAKE_STRENGTH = .6f;

GameVFX::GlobalSettings GameVFX::s_settings;

struct ChromaCB
{
  float originPointX;
  float originPointY;
  float red;
  float blue;
  float green;
  float __padding[3];
};

struct BloomCB
{
    float Luminance;
    float fMiddleGray;
    float fWhiteCutoff;
    float __padding[1];
};

struct ColoredEdgesCB
{
    float alpha;
    float __padding[3];
};

GameVFX::GameVFX(pbl::GraphicalResourceRegistry &resources, pbl::FrameBufferPipeline *pipeline)
    : m_pipeline(pipeline)
    , m_chromaticAberrationEffect(resources.loadEffect(L"res/shaders/chromatic_aberration.fx", pbl::FrameBufferPipeline::getBlitVertexLayout()))
    , m_chromaticAberrationEffectCB(sizeof(ChromaCB), pbl::GenericBuffer::BUFFER_CONSTANT)
    , m_vignetteEffect(resources.loadEffect(L"res/shaders/vignette.fx", pbl::FrameBufferPipeline::getBlitVertexLayout()))
    , m_bloomEffect(resources.loadEffect(L"res/shaders/bloom.fx", pbl::FrameBufferPipeline::getBlitVertexLayout()))
    , m_bloomEffectCB(sizeof(BloomCB), pbl::GenericBuffer::BUFFER_CONSTANT)
    , m_depthOfFieldEffect(resources.loadEffect(L"res/shaders/depth_of_field.fx", pbl::FrameBufferPipeline::getBlitVertexLayout()))
    , m_coloredEdgesEffect(resources.loadEffect(L"res/shaders/colored_edges.fx", pbl::FrameBufferPipeline::getBlitVertexLayout()))
    , m_coloredEdgesEffectCB(sizeof(BloomCB), pbl::GenericBuffer::BUFFER_CONSTANT)
{
}

void GameVFX::update(float deltaTime)
{
  m_chromaticAberrationStrength = mathf::lerp(m_chromaticAberrationStrength, 0.f, .02f);
  m_chromaticAberrationStrength = std::max(m_chromaticAberrationStrength, m_player->getVehicle().getAngularSpeed());
  m_chromaticAberrationStrength = std::max(m_chromaticAberrationStrength, (m_player->getVehicle().getLinearSpeed()/Player::MAX_SPEED-.2f)*3.f);

  m_coloredEdgesCurrentAlpha = mathf::lerp(m_coloredEdgesCurrentAlpha, (GameVFX::getSettings().alwaysEnableTunnelVision ? 1.f : m_player->getColoredEdgesTargetAlpha()), .02f);
  
  m_bloomIntensity = mathf::lerp(m_bloomIntensity, ((XMVectorGetY(m_player->getVehicle().getTransform().position) + 70)/50.f), .07f);

  if(m_player->isFlying()) {
    m_playerAirTime += deltaTime;
  } else {
    // screen-shake is not handled as a shader pass, rather, it is handled directly by the player's camera
    if(s_settings.enableScreenShake && m_playerAirTime > CAMERA_SHAKE_MIN_AIRTIME)
      m_player->getThirdPersonCamera().addShake(CAMERA_SHAKE_DURATION, CAMERA_SHAKE_STRENGTH);
    m_playerAirTime = 0;
  }
}

void GameVFX::apply()
{
  pbl::Texture frame;

  if (s_settings.enableDepthOfField) {
    frame = m_pipeline->swap();
    m_depthOfFieldEffect->bindTexture(frame, "blitTexture");
    m_depthOfFieldEffect->bindTexture(m_pipeline->getDepthTexture(), "depthBuffer");
    m_depthOfFieldEffect->bind();
    pbl::FrameBufferPipeline::doBlitDrawCall();
  }
  
  if (m_coloredEdgesCurrentAlpha > 0.01f)
  { // colored edges
      frame = m_pipeline->swap();
      ColoredEdgesCB coloredEdgesCB;
      coloredEdgesCB.alpha = m_coloredEdgesCurrentAlpha;
      m_coloredEdgesEffectCB.setData(coloredEdgesCB);
      m_coloredEdgesEffect->bindTexture(frame, "blitTexture");
      m_coloredEdgesEffect->bindBuffer(m_coloredEdgesEffectCB, "cbColoredEdges");
      m_coloredEdgesEffect->bind();
      pbl::FrameBufferPipeline::doBlitDrawCall();
  }

  if (s_settings.enableBloom) {
    frame = m_pipeline->swap();
    BloomCB bloomCB;
    bloomCB.Luminance = 0.08f;
    bloomCB.fMiddleGray = 0.18f;
    bloomCB.fWhiteCutoff = m_bloomIntensity;
    m_bloomEffectCB.setData(bloomCB);
    m_bloomEffect->bindTexture(frame, "blitTexture");
    m_bloomEffect->bindBuffer(m_bloomEffectCB, "cbBloom");
    m_bloomEffect->bind();
    pbl::FrameBufferPipeline::doBlitDrawCall();
  }

  if (s_settings.enableChromaticAberration) {
    frame = m_pipeline->swap();
    ChromaCB chromaCB;
    chromaCB.originPointX = .5f;
    chromaCB.originPointY = .5f;
    chromaCB.red   = +0.009f * m_chromaticAberrationStrength;
    chromaCB.green = -0.006f * m_chromaticAberrationStrength;
    chromaCB.blue  = -0.009f * m_chromaticAberrationStrength;
    m_chromaticAberrationEffectCB.setData(chromaCB);
    m_chromaticAberrationEffect->bindTexture(frame, "blitTexture");
    m_chromaticAberrationEffect->bindBuffer(m_chromaticAberrationEffectCB, "cbChromatic");
    m_chromaticAberrationEffect->bind();
    pbl::FrameBufferPipeline::doBlitDrawCall();
  }

  if (s_settings.enableVignette) {
    frame = m_pipeline->swap();
    m_vignetteEffect->bindTexture(frame, "blitTexture");
    m_vignetteEffect->bind();
    pbl::FrameBufferPipeline::doBlitDrawCall();
  }
}
