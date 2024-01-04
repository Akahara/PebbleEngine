#pragma once

#include <functional>
#include <memory>

#include "scene.h"
#include "display/graphical_resource.h"
#include "display/sprite.h"
#include "display/generic_buffer.h"

namespace pbl
{

class TransitionOutScene : public Scene
{
public:
  using SceneSupplier = std::function<std::unique_ptr<Scene>()>;

  explicit TransitionOutScene(SceneSupplier &&nextSceneSupplier, SceneTransition transition=SceneTransition::NORMAL);

  void render() override;
  void update(double delta) override;

private:
  SceneSupplier                  m_followingScene;
  GraphicalResourceRegistry      m_resources;
  SpriteRenderer                 m_sprites;
  std::shared_ptr<GenericBuffer> m_effectCB;
  double                         m_time{};
  bool                           m_hasRenderedOpaqueFrame{};
};

class TransitionInScene : public Scene
{
private:
  friend TransitionOutScene;
  TransitionInScene();

public:
  void render() override;
  void update(double delta) override;
  SceneStackType getSceneStackType() const override { return SceneStackType::KEEP_UPDATE; }

private:
  GraphicalResourceRegistry      m_resources;
  SpriteRenderer                 m_sprites;
  std::shared_ptr<GenericBuffer> m_effectCB;
  double                         m_time{};
};

}
