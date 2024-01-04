#pragma once

#include "player.h"
#include "display/frame_buffer.h"

class GameVFX {
public:
  struct GlobalSettings {
    bool enableScreenShake = true;
    bool enableChromaticAberration = true;
    bool enableVignette = true;
    bool enableBloom = true;
    bool enableDepthOfField = true;
    bool alwaysEnableTunnelVision = false;

    bool invertYAxis = true;
  };

  GameVFX(pbl::GraphicalResourceRegistry &resources, pbl::FrameBufferPipeline *pipeline);

  void setPlayer(Player *player) { m_player = player; }

  void update(float deltaTime);

  // can only be called after geometry was fully rendered to the frame buffer pipeline
  void apply();

  static GlobalSettings &getSettings() { return s_settings; }

private:
  static GlobalSettings s_settings;

  Player *m_player = nullptr;
  pbl::FrameBufferPipeline *m_pipeline;

  // ---- Common members
  
  float m_playerAirTime = 0;

  // ---- VFX-dependant members

  float m_chromaticAberrationStrength{};
  pbl::Effect *m_chromaticAberrationEffect;
  pbl::GenericBuffer m_chromaticAberrationEffectCB;

  pbl::Effect *m_vignetteEffect;

  float m_bloomIntensity{3.f};
  pbl::Effect *m_bloomEffect;
  pbl::GenericBuffer m_bloomEffectCB;

  pbl::Effect *m_depthOfFieldEffect;

  pbl::Effect *m_coloredEdgesEffect;
  pbl::GenericBuffer m_coloredEdgesEffectCB;
  float m_coloredEdgesCurrentAlpha{};
};
