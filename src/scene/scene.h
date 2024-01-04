#pragma once

namespace pbl
{

enum class SceneStateChange {
  AQUIRED_FOCUS, // when this scene becomes the top of the stack
  LOST_FOCUS,    // when another scene is pushed on top of the stack or before the scene is destroyed
};

enum class SceneStackType {
  FREEZE,      // freeze other scenes further down the stack but still render them
  KEEP_UPDATE, // keep updating other scenes further down the stack
};

enum class SceneTransition {
  NORMAL,
  INSTANT_OUT,
};

class Scene
{
public:
  virtual ~Scene() = default;
  
  virtual void update(double delta) = 0;
  virtual void render() = 0;
  virtual void onSceneStateChange(SceneStateChange change) {}
  virtual SceneStackType getSceneStackType() const { return SceneStackType::FREEZE; }
};

}