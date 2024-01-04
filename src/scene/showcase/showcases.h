#pragma once

#include "utils/debug.h"
#include "display/renderer.h"
#include "display/camera.h"
#include "display/skybox.h"
#include "display/sprite.h"
#include "display/text.h"
#include "display/graphical_resource.h"
#include "world/object.h"
#include "scene/freecam.h"
#include "scene/scene.h"
#include "scene/scene_manager.h"
#include "scene/game/game_logic.h"
#include "physics/physics.h"

using namespace pbl;
using namespace pbx;
using namespace DirectX;

class ShowcaseScene : public Scene, protected PhysicsEventHandler
{
public:
  ShowcaseScene();

  virtual void update(double delta) override;
  virtual void render() override;

protected:
  virtual void render(const Camera &camera);

private:
  virtual CollisionResponse shouldObjectsCollide(const WorldObject &b1, const WorldObject &b2) override { return CollisionResponse::COLLIDE; }
  virtual void onObjectCollision(WorldObject *b1, WorldObject *b2) override {}

protected:
  GraphicalResourceRegistry       m_graphicalResources;
  std::vector<std::shared_ptr<WorldObject>> m_objects;
  std::shared_ptr<GenericBuffer>  m_worldConstantBuffer;
  Physics                         m_physics;
  SpriteRenderer                  m_sprites;
  TextRenderer                    m_text;
  UIManager                       m_ui;
  Freecam                         m_freecam;
  float                           m_inputCooldown = 0;
  float                           m_worldTime = 0;
  bool                            m_cursorLocked = true;
  Skybox                          m_skybox;
  bool                            m_pausePhysics = false;
};

inline ShowcaseScene::ShowcaseScene()
    : m_worldConstantBuffer(GenericBuffer::make_buffer<WorldUniforms>(GenericBuffer::BUFFER_CONSTANT))
    , m_text(fonts::defaultFont, m_graphicalResources, &m_sprites)
    , m_ui(&m_graphicalResources)
    , m_skybox(m_graphicalResources.loadCubemap(L"res/textures/skybox.dds"))
    , m_sprites(m_graphicalResources)
{
  m_freecam.setActive(true);
}

inline void ShowcaseScene::update(double delta)
{
  m_worldTime += static_cast<float>(delta);
  m_freecam.processUserInputs(delta);

  if (!m_pausePhysics)
    m_physics.step(1.f/60.f);
}

inline void ShowcaseScene::render()
{
  render(m_freecam.getCamera());
}

inline void ShowcaseScene::render(const Camera& camera)
{
  m_sprites.clearBillboards();
  m_sprites.clearSprites();

  WorldUniforms cameraUniforms{};
  cameraUniforms.vWorldMatViewProj = XMMatrixTranspose(camera.getViewProjectionMatrix());
  cameraUniforms.vWorldLightDir = XMVectorSet(0.0f, -1.0f, 1.0f, 0.0f);
  cameraUniforms.vWorldCameraPos = camera.getPosition();
  cameraUniforms.vWorldAmbiantLight = XMVectorSet(0.2f, 0.2f, 0.2f, 1.0f);
  cameraUniforms.vWorldDiffuseLight = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
  cameraUniforms.vWorldSpecularLight = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
  cameraUniforms.vWorldTime = m_worldTime;
  m_worldConstantBuffer->setData(cameraUniforms);

  RenderContext renderContext(camera);
  renderContext.constantBufferBindings.push_back({ "cbWorld", m_worldConstantBuffer.get() });
  RenderContext spriteRenderContext(camera);

  { // render world objects
    for (auto &obj : m_objects)
      obj->render(renderContext);
  }

  { // render other global objects
    if(!std::holds_alternative<OrthographicProjection>(camera.getProjection()))
      m_skybox.render(camera);
  }

  { // render transparents
    for (auto &obj : m_objects)
      obj->renderTransparent(renderContext);
    m_sprites.renderSprites(spriteRenderContext);
    m_sprites.renderBillboards(spriteRenderContext);
  }

  { // render debug objects
    renderer::sendDebugDraws(camera);
  }

  // debug windows
  logs::scene.beginWindow();
  ImGui::Checkbox("pause physics", &m_pausePhysics);
  ImGui::End();
}
