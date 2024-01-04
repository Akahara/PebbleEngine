#include "main_menu.h"

#include "game_scene.h"
#include "ground.h"
#include "player_ribbons.h"
#include "settings_menu.h"
#include "display/ui/ui_elements.h"
#include "scene/scene_manager.h"
#include "engine/windowsengine.h"
#include "serial/game_serializer.h"
#include "inputs/user_inputs.h"

using namespace pbl;

MainMenuScene::MainMenuScene()
  : m_ui(&m_resources)
  , m_persistentUI(&m_resources)
  , m_graphicsPipeline(FrameBufferPipeline::fullscreen_t{})
  , m_graphicsVFXPass(m_resources, &m_graphicsPipeline)
  , m_worldConstantBuffer(sizeof(WorldUniforms), GenericBuffer::BUFFER_CONSTANT)
  , m_skybox(m_resources.loadCubemap(L"res/textures/skybox.dds"))
  , m_sprites(m_resources)
  , m_gameLogic(nullptr)
{
  Device::addWindowResizeEventHandler(m_windowResizeEventHandler = std::make_shared<ScreenResizeEventHandler>([this](int, int) { rebuildUI(); }));
  rebuildUI();
  loadBackgroundGame();
}

void MainMenuScene::update(double delta)
{
  m_ui.update();
  m_ui.pollEvents();
  updateBackgroundGame(static_cast<float>(delta));

  if (UserInputs::consumeDPadPress(keys::SC_DPAD_UP) || UserInputs::consumeKeyPress(keys::SC_W)) {
      if (m_selectedButtonIndex == -1) {
          m_buttons[0]->setSelected(true);
          m_selectedButtonIndex = 0;
      } else {
          m_buttons[m_selectedButtonIndex]->setSelected(false);
          m_selectedButtonIndex--;
          if (m_selectedButtonIndex == -1)
              m_selectedButtonIndex = m_buttons.size() - 1;
          m_buttons[m_selectedButtonIndex]->setSelected(true);
      }
  }
  if (UserInputs::consumeDPadPress(keys::SC_DPAD_DOWN) || UserInputs::consumeKeyPress(keys::SC_S)) {
      if (m_selectedButtonIndex == -1) {
          m_buttons[0]->setSelected(true);
          m_selectedButtonIndex = 0;
      } else {
          m_buttons[m_selectedButtonIndex]->setSelected(false);
          m_selectedButtonIndex = (m_selectedButtonIndex + 1) % m_buttons.size();
          m_buttons[m_selectedButtonIndex]->setSelected(true);
      }
  }

  if (UserInputs::consumeButtonPress(keys::SC_CROSS) || UserInputs::consumeKeyPress(keys::SC_ENTER))
      m_buttons[m_selectedButtonIndex]->triggerCallback();
}

void MainMenuScene::render()
{
  m_persistentUI.getSprites().addSprite({ renderBackgroundGame(), m_backgroundGameRegion });
  m_persistentUI.render();
  if(!m_isSubMenuActive)
    m_ui.render();
}

void MainMenuScene::rebuildUI()
{
  auto startBtn = std::make_shared<ButtonElement>("Start", .05f, .6f, Anchor::BOTTOM_LEFT);
  auto settingsBtn = std::make_shared<ButtonElement>("Settings", .05f, .5f, Anchor::BOTTOM_LEFT);
  auto quitBtn = std::make_shared<ButtonElement>("Quit", .05f, .4f, Anchor::BOTTOM_LEFT);
  startBtn->setClickHandler([] { SceneManager::getInstance().transitionToScene(SceneManager::make_supplier<GameScene>()); });
  settingsBtn->setClickHandler([&] { m_isSubMenuActive = true; SceneManager::getInstance().pushLayeredScene(SceneManager::make_supplier<SettingsMenuScene>()); });
  quitBtn->setClickHandler([] { WindowsEngine::exit(); });

  m_buttons.push_back(startBtn);
  m_buttons.push_back(settingsBtn);
  m_buttons.push_back(quitBtn);

  m_backgroundGameRegion = { ScreenRegion::SCREEN_WIDTH*.15f, 0, ScreenRegion::SCREEN_WIDTH*1.15f, ScreenRegion::SCREEN_HEIGHT*1.0f };
  m_backgroundUIRegion = { ScreenRegion::SCREEN_WIDTH*.0f, 0, ScreenRegion::SCREEN_WIDTH*0.4f, ScreenRegion::SCREEN_HEIGHT*1.0f };
  m_logoRegion = { 0.05f, 0.6f, 0.4f,1.0f };
  m_startRegion = { 0.1f, 0.5f, 0.65f,.7f };

  m_persistentUI.clearUIElements();
  m_persistentUI.addUIElement(std::make_shared<SpriteElement>(m_backgroundUIRegion, m_resources.loadTexture(L"res/textures/backgroundUI.dds"), false));
  m_persistentUI.addUIElement(std::make_shared<SpriteElement>(m_logoRegion, m_resources.loadTexture(L"res/textures/logo.dds"), false));
 // m_persistentUI.addUIElement(std::make_shared<SpriteElement>(m_startRegion, m_resources.loadTexture(L"res/sprites/button_background.dds"), false));
  m_ui.clearUIElements();
  m_ui.addUIElement(std::move(startBtn));
  m_ui.addUIElement(std::move(settingsBtn));
  m_ui.addUIElement(std::move(quitBtn));
}

void MainMenuScene::loadBackgroundGame()
{
  auto playerVehicle = std::make_shared<PlayerVehicle>(m_resources);

  { // player object
    m_player.setVehicleObject(playerVehicle.get());
    m_objects.push_back(playerVehicle);
    m_gameLogic.setPlayer(&m_player);
    m_gameLogic.setToEndGame();
    m_graphicsVFXPass.setPlayer(&m_player);
    m_shadowCastingObjects.push_back(playerVehicle);
  }

  { // other global objects
    auto ribbons = std::make_shared<PlayerRibbons>(m_resources);
    ribbons->setPlayerVehicle(playerVehicle.get());
    m_objects.push_back(ribbons);
    m_objects.push_back(std::make_shared<Ground>(m_resources));
    m_objects.push_back(m_sun = std::make_shared<Sun>(m_resources, &m_sprites));
  }

  SerializerResources serialResources{};
  serialResources.objects = &m_objects;
  serialResources.gameLogic = &m_gameLogic;
  serialResources.physics = &m_physics;
  serialResources.resources = &m_resources;
  GameSerializer serializer{ serialResources };
  serializer.loadLevelFile("res/level/main_menu_level.json");

  // create physics object
  for(auto &worldObject : m_objects) {
    if (pbx::PhysicsBody *physics = worldObject->buildPhysicsObject()) {
      if(physics)
        m_physics.addBody(physics);
    }
  }
}

void MainMenuScene::updateBackgroundGame(float delta)
{
  m_worldTime += delta;
  m_physics.step(delta);
  m_player.update(delta);
  m_gameLogic.logicUpdate(delta);
  m_graphicsVFXPass.update(delta);

  for (auto &object : m_objects)
    object->update(delta);
}

Texture MainMenuScene::renderBackgroundGame()
{
  const Camera&activeCamera = m_player.getActiveCamera();

  m_graphicsPipeline.bindGeometry();
  m_sprites.clearSprites();
  m_sprites.clearBillboards();

  Texture shadowMap = m_sun->renderDepthTexture(m_shadowCastingObjects, m_player.getVehicle().getTransform().position);

  WorldUniforms worldUniforms{};
  worldUniforms.vWorldMatViewProj = XMMatrixTranspose(activeCamera.getViewProjectionMatrix());
  worldUniforms.vWorldMatShadowViewProj = XMMatrixTranspose(m_sun->getSunViewProjectionMatrix());
  worldUniforms.vWorldLightDir = m_sun->getSunDirection();
  worldUniforms.vWorldCameraPos = activeCamera.getPosition();
  worldUniforms.vWorldAmbiantLight = { 0.2f, 0.2f, 0.2f, 1.0f };
  worldUniforms.vWorldDiffuseLight = { 1.0f, 1.0f, 1.0f, 1.0f };
  worldUniforms.vWorldSpecularLight = { 1.0f, 1.0f, 1.0f, 1.0f };
  worldUniforms.vWorldTime = m_worldTime;
  m_worldConstantBuffer.setData(worldUniforms);

  RenderContext worldRenderContext(activeCamera);
  worldRenderContext.constantBufferBindings.push_back({ "cbWorld", &m_worldConstantBuffer });
  worldRenderContext.textureBindings.push_back({ "depthMap", shadowMap });
  worldRenderContext.focusPosition = m_player.getVehicle().getTransform().position;

  RenderContext spriteRenderContext(activeCamera);

  m_sprites.renderBillboards(spriteRenderContext);

  { // render world objects
    for (auto &obj : m_objects)
      obj->render(worldRenderContext);
  }

  { // render other global objects
    m_skybox.render(activeCamera);
  }

  { // render transparents
    for (auto &obj : m_objects)
      obj->renderTransparent(worldRenderContext);
  }

  // apply screen-space effects
  m_graphicsPipeline.endGeometry();
  m_graphicsVFXPass.apply();

  { // render UI
    m_sprites.renderSprites(spriteRenderContext);
    m_gameLogic.renderUI();
  }

  return m_graphicsPipeline.endFrame();
}

void MainMenuScene::onSceneStateChange(SceneStateChange change)
{
  if (change == SceneStateChange::AQUIRED_FOCUS)
    m_isSubMenuActive = false;
}
