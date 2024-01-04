#pragma once

#include <functional>
#include <memory>
#include <queue>
#include <vector>

#include "scene.h"

namespace pbl
{

/*
 * Scene model:
 *
 * At any time, there can be N active scenes at the same time in a stack, all of them are rendered
 * but only the top one is rendered. To switch from one scene to another, use disposeAllScenes()
 * and pushLayeredScene(). Never leave the stack empty!
 *
 * When calling any of the methods from a Scene update() or render() function, the scene stack is
 * not updated until the end of the frame. This is to avoid scene deletion. To make this work,
 * pushLayeredScene() takes a scene *supplier*, a function that will be used to instanciate the
 * scene at the right time.
 *
 * For nice scene transitions, use transitionToScene()
 */
class SceneManager
{
public:
  using SceneSupplier = std::function<std::unique_ptr<Scene>()>;

  static SceneManager &getInstance() { return s_singleton; }

  // BEWARE! this method cannot be called from an active scene, use disposeAllScenes() and pushLayeredScene() instead
  void setActiveScene(std::unique_ptr<Scene> &&scene);

  void transitionToScene(SceneSupplier &&sceneSupplier, SceneTransition transition=SceneTransition::NORMAL);
  void pushLayeredScene(SceneSupplier &&sceneSupplier);
  void popLayeredScene();
  void disposeAllScenes();

  // creates a "scene supplier", a function that creates the scene when called
  // This is useful because scenes can be created at the right time, refer to the scene model
  template<class SceneType>
  static SceneSupplier make_supplier(auto &&...args)
  {
    return [...args = std::forward<decltype(args)>(args)]() mutable -> std::unique_ptr<Scene>
    {
      return std::make_unique<SceneType>(std::forward<decltype(args)>(args)...);
    };
  }

  // called by the engine, not by scenes! returns whether scenes were destroyed/created
  bool performTransitions();

  void update(double delta) const;
  void render() const;

private:
  static SceneManager s_singleton;
  std::vector<std::unique_ptr<Scene>> m_sceneStack;
  std::queue<std::function<void()>>   m_transitionsActionQueue;
};

}
