#include "game_scene.h"

#include "ground.h"
#include "jump_pad.h"
#include "boost_square.h"
#include "pause_menu.h"
#include "player_ribbons.h"
#include "player_vehicle.h"
#include "sun.h"
#include "display/frame_buffer.h"
#include "display/renderer.h"
#include "inputs/user_inputs.h"
#include "scene/scene_manager.h"
#include "serial/game_serializer.h"
#include "world/trigger_box.h"

GameScene::GameScene()
  : m_graphicsPipeline(pbl::FrameBufferPipeline::fullscreen_t{})
  , m_graphicsVFXPass(m_graphicalResources, &m_graphicsPipeline)
  , m_worldConstantBuffer(pbl::GenericBuffer::make_buffer<pbl::WorldUniforms>(pbl::GenericBuffer::BUFFER_CONSTANT))
  , m_text(pbl::fonts::defaultFont, m_graphicalResources, &m_sprites)
  , m_skybox(m_graphicalResources.loadCubemap(L"res/textures/skybox.dds"))
  , m_sprites(m_graphicalResources)
  , m_ui(&m_graphicalResources)
  , m_gameLogic(&m_ui)
{
  initSceneObjects();
}

void GameScene::update(double delta)
{
  m_worldTime += delta;
  m_physics.step(delta);
  m_player.update(delta);
  m_gameLogic.logicUpdate(static_cast<float>(delta));
  m_graphicsVFXPass.update(static_cast<float>(delta));

  for (auto &object : m_objects)
    object->update(delta);

  // global key events
  if (pbl::UserInputs::consumeKeyPress(keys::SC_ESCAPE) || pbl::UserInputs::consumeButtonPress(keys::SC_START))
    pbl::SceneManager::getInstance().pushLayeredScene(pbl::SceneManager::make_supplier<PauseMenuScene>());

  if (pbl::UserInputs::consumeKeyPress(keys::SC_TAB))
    m_graphicalResources.reloadShaders();
}

void GameScene::render()
{
  const pbl::Camera &activeCamera = m_player.getActiveCamera();
  
  m_graphicsPipeline.bindGeometry();
  m_sprites.clearSprites();
  m_sprites.clearBillboards();

  pbl::Texture shadowMap = m_sun->renderDepthTexture(m_shadowCastingObjects, m_playerVehicle->getTransform().position);

  pbl::WorldUniforms worldUniforms{};
  worldUniforms.vWorldMatViewProj = XMMatrixTranspose(activeCamera.getViewProjectionMatrix());
  worldUniforms.vWorldMatShadowViewProj = XMMatrixTranspose(m_sun->getSunViewProjectionMatrix());
  worldUniforms.vWorldLightDir = m_sun->getSunDirection();
  worldUniforms.vWorldCameraPos = activeCamera.getPosition();
  worldUniforms.vWorldAmbiantLight = { 0.2f, 0.2f, 0.2f, 1.0f };
  worldUniforms.vWorldDiffuseLight = { 1.0f, 1.0f, 1.0f, 1.0f };
  worldUniforms.vWorldSpecularLight = { 1.0f, 1.0f, 1.0f, 1.0f };
  worldUniforms.vWorldTime = static_cast<float>(m_worldTime);
  m_worldConstantBuffer->setData(worldUniforms);

  pbl::RenderContext worldRenderContext(activeCamera);
  worldRenderContext.constantBufferBindings.push_back({ "cbWorld", m_worldConstantBuffer.get() });
  worldRenderContext.textureBindings.push_back({ "depthMap", shadowMap });
  worldRenderContext.focusPosition = m_playerVehicle->getTransform().position;

  pbl::RenderContext spriteRenderContext(activeCamera);

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
    m_sprites.renderBillboards(spriteRenderContext);
  }

  // apply screen-space effects
  m_graphicsPipeline.endGeometry();
  m_graphicsVFXPass.apply();

  { // render UI
    m_sprites.renderSprites(spriteRenderContext);
    m_gameLogic.renderUI();
  }
  pbl::Texture finalFrame = m_graphicsPipeline.endFrame();
  pbl::FrameBufferPipeline::doSimpleBlit(finalFrame);
}

// if you have an error here, check if your checkpoint ref follows the format : checkpoint_[checkpoint number]([non-used identifier])
// i.e : checkpoint_01a is valid
GameLogic::checkpointid_t idFromRef(std::string &ref)
{
    try {
        return std::stoi(ref.substr(11, ref.size()-11));
    } catch (size_t) {
        return std::stoi(ref.substr(11, ref.size()-12));
    }
}

// if you have an error here, check if your jump ref follows the format : jump_[id in two characters]_[jumpForce]
float forceFromRef(std::string &ref)
{
    try {
        return std::stof(ref.substr(8, ref.size()-8));
    } catch (size_t) {
        return std::stof(ref.substr(8, ref.size()-9));
    }
}

// if you have an error here, check if your jump ref follows the format : boost_square_[id in two characters]_[boostValue (in %)]
float boostFromRef(std::string &ref)
{
    try {
        return std::stof(ref.substr(16, ref.size()-16))/100.f;
    } catch (size_t) {
        return std::stof(ref.substr(16, ref.size()-17))/100.f;
    }
}

// if you have an error here, check if your jump ref follows the format : tunnel_vision_[id in two characters]_[alpha (in %)]
float alphaFromRef(std::string &ref)
{
    try {
        return std::stof(ref.substr(17, ref.size()-17))/100.f;
    } catch (size_t) {
        return std::stof(ref.substr(17, ref.size()-18))/100.f;
    }
}

void GameScene::initSceneObjects()
{
  pbl::SerializerResources serializerResources;
  serializerResources.editors = nullptr; // no editors
  serializerResources.gameLogic = &m_gameLogic;
  serializerResources.objects = &m_objects;
  serializerResources.physics = &m_physics;
  serializerResources.resources = &m_graphicalResources;
  pbl::GameSerializer serializer{ serializerResources };

  { // player object
    m_playerVehicle = std::make_shared<PlayerVehicle>(m_graphicalResources);
    m_player.setVehicleObject(m_playerVehicle.get());
    m_objects.push_back(m_playerVehicle);
    m_gameLogic.setPlayer(&m_player);
    m_graphicsVFXPass.setPlayer(&m_player);
    m_physics.addEventHandler(&m_gameLogic);
    m_shadowCastingObjects.push_back(m_playerVehicle);
  }

  // most of the scene's objects
  serializer.loadLevelFile("res/level/level.json");

  { // bind checkpoints events (TODO not yet fully serialized)
    std::vector<std::pair<std::string,std::shared_ptr<pbl::WorldObject>>> checkpoints = serializer.getReferencedObjects("checkpoint");
    for (size_t i = 0; i < checkpoints.size(); i++) {
      std::shared_ptr<pbl::TriggerBox> checkpoint = std::static_pointer_cast<pbl::TriggerBox>(checkpoints[i].second);
      GameLogic::checkpointid_t id = idFromRef(checkpoints[i].first);
      checkpoint->setCallback([id, this](pbl::TriggerBox &self) {
        m_gameLogic.validateCheckpoint(id, self.getTransform());
        logs::scene.logm("Crossed checkpoint ", id);
      });
    }
    if (!checkpoints.empty())
      m_gameLogic.setLastCheckpointId(idFromRef(checkpoints[checkpoints.size()-1].first));
  }

  { // bind killzone events (TODO not yet fully serialized)
    std::vector<std::pair<std::string, std::shared_ptr<pbl::WorldObject>>> killzones = serializer.getReferencedObjects("killzone");
    for (auto &rawKillzone : killzones) {
      std::shared_ptr<pbl::TriggerBox> checkpoint = std::static_pointer_cast<pbl::TriggerBox>(rawKillzone.second);
      checkpoint->setCallback([this](pbl::TriggerBox &self) {
        m_gameLogic.setPlayerToCheckpoint();
        logs::scene.logm("Went out of bounds!");
      });
    }
  }

  { // bind jump events
      std::vector<std::pair<std::string, std::shared_ptr<pbl::WorldObject>>> jumps = serializer.getReferencedObjects("jump");
      for (auto &rawJump : jumps) {
          std::shared_ptr<pbl::TriggerBox> checkpoint = std::static_pointer_cast<pbl::TriggerBox>(rawJump.second);
          Transform visiblePadTransform = checkpoint->getTransform();
          visiblePadTransform.position -= vec3{0, XMVectorGetY(visiblePadTransform.scale)/2.f + 0.01f, 0};
          visiblePadTransform.scale /= 2.f;
          m_objects.push_back(std::make_shared<pbl::JumpPad>(visiblePadTransform, m_graphicalResources));
          float force = forceFromRef(rawJump.first);
          checkpoint->setCallback([this,force](pbl::TriggerBox &self) {
              m_player.jump(force);
              logs::scene.logm("Jumped at force ", std::to_string(force), "!");
          });
      }
  }

  { // bind boost squares events
      std::vector<std::pair<std::string, std::shared_ptr<pbl::WorldObject>>> squares = serializer.getReferencedObjects("boost_square");
      for (auto &rawSquare : squares) {
          std::shared_ptr<pbl::TriggerBox> checkpoint = std::static_pointer_cast<pbl::TriggerBox>(rawSquare.second);
          Transform visibleSquareTransform = checkpoint->getTransform();
          visibleSquareTransform.scale /= 2.f;
          m_objects.push_back(std::make_shared<pbl::BoostSquare>(visibleSquareTransform, m_graphicalResources));
          float boostValue = boostFromRef(rawSquare.first);
          checkpoint->setCallback([this, boostValue](pbl::TriggerBox &self) {
              m_player.setRemainingFuel(m_player.getRemainingFuel() + boostValue);
              logs::scene.logm("Gained ", std::to_string((int)(boostValue * 100)), "% boost !");
          });
      }
  }

  { // bind tunnel vision events
      std::vector<std::pair<std::string, std::shared_ptr<pbl::WorldObject>>> visions = serializer.getReferencedObjects("tunnel_vision");
      for (auto &rawVision : visions) {
          std::shared_ptr<pbl::TriggerBox> visionBox = std::static_pointer_cast<pbl::TriggerBox>(rawVision.second);
          float alpha = alphaFromRef(rawVision.first);
          visionBox->setCallback([this, alpha](pbl::TriggerBox &self) {
              m_player.setColoredEdgesTargetAlpha(alpha);
              logs::scene.logm("Tunnel vision set to ", std::to_string(alpha), " !");
          });
      }
  }

  { // other global objects
    auto ribbons = std::make_shared<PlayerRibbons>(m_graphicalResources);
    ribbons->setPlayerVehicle(m_playerVehicle.get());
    m_objects.push_back(ribbons);
    m_objects.push_back(std::make_shared<Ground>(m_graphicalResources));
    m_objects.push_back(m_sun = std::make_shared<Sun>(m_graphicalResources, &m_sprites));
  }

  m_objects.push_back(std::make_shared<Ground>(m_graphicalResources));
  m_sun = std::make_shared<Sun>(m_graphicalResources, &m_sprites);
  m_objects.push_back(m_sun);

  // create physics object
  for(auto &worldObject : m_objects) {
    if (pbx::PhysicsBody *physics = worldObject->buildPhysicsObject()) {
      if(physics)
        m_physics.addBody(physics);
    }
  }
}

void GameScene::onSceneStateChange(pbl::SceneStateChange change)
{
#ifndef PBL_ISDEBUG
  switch(change) {
  case pbl::SceneStateChange::AQUIRED_FOCUS:
    pbl::UserInputs::setCursorLocked(true);
    break;
  case pbl::SceneStateChange::LOST_FOCUS:
    pbl::UserInputs::setCursorLocked(false);
    break;
  }
#endif
}
