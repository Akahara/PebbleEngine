#pragma once

#include "display/texture.h"
#include "display/ui/ui_elements.h"
#include "display/ui/ui_manager.h"
#include "engine/device.h"

class GaugeElement : public pbl::UIElement {
public:
  GaugeElement(const pbl::Texture &emptyTexture, const pbl::Texture &filledTexture, const pbl::ScreenRegion &region);

  void render(pbl::ScreenRegion screenRegion) override;

  void setFillLevel(float fillLevel) { m_fillLevel = fillLevel; }
  void updateFillLevel(float fillLevel, float damping = .1f) { m_fillLevel = mathf::lerp(m_fillLevel, fillLevel, damping); }
  const pbl::ScreenRegion &getRegion() const { return m_region; }

private:
  pbl::ScreenRegion m_region;
  pbl::Texture m_emptyTexture;
  pbl::Texture m_filledTexture;
  float m_fillLevel = 0;
};

class GameUI {
public:
  explicit GameUI(pbl::UIManager *ui);

  void update(float gameTime, float playerVelocity, float playerFuel);
  void render() const;

  void onNewCheckpointCrossed(float gameTime, bool isFinalCheckpoint);

private:
  void rebuildUI();

private:
  float m_checkpointCrossTime = std::numeric_limits<float>::lowest();
  bool m_freezeCheckpointTimeDisplay = false;
  pbl::UIManager *m_ui;
  std::shared_ptr<pbl::TextElement> m_timeText;
  std::shared_ptr<pbl::TextElement> m_speedText;
  std::shared_ptr<pbl::TextElement> m_fuelText;
  std::shared_ptr<GaugeElement> m_speedGauge;
  std::shared_ptr<GaugeElement> m_fuelGauge;
  std::shared_ptr<pbl::ScreenResizeEventHandler> m_screenResizeEventHandler;
};
