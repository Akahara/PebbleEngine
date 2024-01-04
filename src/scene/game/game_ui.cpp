#include "game_ui.h"

#include "player.h"

REGISTER_TEXTURE_ENTRY(SpeedGaugeFilled, L"res/sprites/gauge_speed.dds");
REGISTER_TEXTURE_ENTRY(FuelGaugeFilled, L"res/sprites/gauge_fuel.dds");
REGISTER_TEXTURE_ENTRY(GaugeEmpty, L"res/sprites/gauge_empty.dds");

using namespace pbl;

static constexpr float CHECKPOINT_TIME_DISPLAY_DURATION = 2.f;

GaugeElement::GaugeElement(const Texture&emptyTexture, const Texture&filledTexture, const ScreenRegion &region)
  : m_emptyTexture(emptyTexture)
  , m_filledTexture(filledTexture)
  , m_region(region)
{}

void GaugeElement::render(ScreenRegion screenRegion)
{
  ScreenRegion filledRegion = m_region;
  filledRegion.maxX = mathf::lerp(filledRegion.minX, filledRegion.maxX, m_fillLevel);
  m_ui->getSprites().addSprite({ m_emptyTexture, m_region });
  m_ui->getSprites().addSprite({ m_filledTexture, filledRegion, 0, 0, m_fillLevel, 1 });
}

GameUI::GameUI(UIManager *ui)
  : m_ui(ui)
{
  PBL_ASSERT(ui != nullptr, "No ui provided");
  rebuildUI();
  Device::addWindowResizeEventHandler(m_screenResizeEventHandler = std::make_unique<ScreenResizeEventHandler>([this](int, int) {
    rebuildUI();
  }));
}

void GameUI::update(float gameTime, float playerVelocity, float playerFuel)
{
  bool displayCheckpointTime = m_freezeCheckpointTimeDisplay || gameTime - m_checkpointCrossTime < CHECKPOINT_TIME_DISPLAY_DURATION;
  float displayedTime = displayCheckpointTime ? m_checkpointCrossTime : gameTime;
  unsigned int time = static_cast<unsigned int>(displayedTime);
  unsigned int mm = time / 60;
  unsigned int ss = time % 60;
  unsigned int cc = static_cast<unsigned int>((displayedTime - mm * 60 - ss) * 100.f);
  m_timeText->setText(std::format("{:02}:{:02}:{:02}", mm, ss, cc));
  m_timeText->setTextColor(displayCheckpointTime ? rvec4{ 0.988f,0.706f,0.189f,1.f } : rvec4{ 1,1,1,1 });
  m_speedText->setText(std::format("{} km/h", (unsigned int)(20 * playerVelocity)));
  m_speedText->setTextColor(playerVelocity > Player::MAX_SPEED/2 ? rvec4{ 1,.3f,.3f,1.f } : rvec4{ 1,1,1,1 } );
  m_fuelText->setText(std::format("{} %", (int)(playerFuel*100)));
  m_speedGauge->setFillLevel(playerVelocity / Player::MAX_SPEED);
  m_fuelGauge->updateFillLevel(playerFuel);
}

void GameUI::render() const
{
  m_ui->render();
}

void GameUI::onNewCheckpointCrossed(float gameTime, bool isFinalCheckpoint)
{
  m_checkpointCrossTime = gameTime;
  m_freezeCheckpointTimeDisplay = isFinalCheckpoint;
}

void GameUI::rebuildUI()
{
  const float W = ScreenRegion::SCREEN_WIDTH, H = ScreenRegion::SCREEN_HEIGHT;

  Texture gaugeEmptyTexture = m_ui->registerTexture<GaugeEmpty>();

  float gaugesHeight = H*.04f;
  float gaugesWidth = gaugeEmptyTexture.getWidth() * gaugesHeight / gaugeEmptyTexture.getHeight();
  ScreenRegion speedGaugeRegion = ScreenRegion::moveRegionCorner(gaugesWidth, gaugesHeight, W*.51f, gaugesHeight*2.f, Anchor::VMIDDLE|Anchor::LEFT);
  ScreenRegion fuelGaugeRegion = ScreenRegion::moveRegionCorner(gaugesWidth, gaugesHeight, W*.49f, gaugesHeight*2.f, Anchor::VMIDDLE|Anchor::RIGHT);
  std::swap(fuelGaugeRegion.minX, fuelGaugeRegion.maxX); // fill from right to left

  m_ui->clearUIElements();
  m_ui->addUIElement(m_timeText = std::make_shared<TextElement>("00:00:00", W*.5f, gaugesHeight*4.f, gaugesHeight*1.5f, Anchor::CENTER));
  m_ui->addUIElement(m_speedGauge = std::make_shared<GaugeElement>(gaugeEmptyTexture, m_ui->registerTexture<SpeedGaugeFilled>(), speedGaugeRegion));
  m_ui->addUIElement(m_speedText = std::make_shared<TextElement>("0 km/h", speedGaugeRegion.maxX + W*.01f, speedGaugeRegion.ymiddle(), gaugesHeight*.8f, Anchor::VMIDDLE|Anchor::LEFT));
  m_ui->addUIElement(m_fuelGauge = std::make_shared<GaugeElement>(gaugeEmptyTexture, m_ui->registerTexture<FuelGaugeFilled>(), fuelGaugeRegion));
  m_ui->addUIElement(m_fuelText = std::make_shared<TextElement>("0 %", fuelGaugeRegion.maxX - W*.01f, speedGaugeRegion.ymiddle(), gaugesHeight*.8f, Anchor::VMIDDLE|Anchor::RIGHT));
}
