#pragma once

#include <memory>
#include <algorithm>

#include "scene/game/player.h"
#include "scene/game/track.h"
#include "scene/showcase/showcases.h"
#include "serial/game_serializer.h"
#include "world/terrain.h"

/*
 * For some reason, the editor showcase scene sometimes crashes uppon exiting the game.
 * The source of the crash is very misterious (crash in the standard exit(), without stack trace).
 * Until this becomes a real issue I won't investigate it further.
 */

class WorldObjectEditor {
public:
  explicit WorldObjectEditor(std::string name)
    : m_name(name), m_displayName(std::move(name)) {}
  
  virtual ~WorldObjectEditor() = default;
  virtual bool render();

  const std::string &getDisplayName() const { return m_displayName; }
  std::string getRef() const { return m_refEditionBuffer; }
  void setRef(const std::string &ref) {
    *std::copy_n(ref.begin(), std::min(std::size(m_refEditionBuffer)-1, ref.size()), m_refEditionBuffer) = '\0';
    updateDisplayName();
  }

  layer_t getLayer() const { return m_layer; }
  void setLayer(layer_t layer);

protected:
  bool renderLayerSelector(const std::shared_ptr<WorldObject> &object);

private:
  void updateDisplayName() { m_displayName = m_refEditionBuffer[0] ? m_name + "_#" + m_refEditionBuffer : m_name; }

private:
  std::string m_name;
  std::string m_displayName;
  char m_refEditionBuffer[25]{};
  layer_t m_layer;
  int m_activeLayer;
};

class EditorScene : public ShowcaseScene {
public:
  EditorScene();

  void update(double delta) override;
  void render() override;

private:
  void rebuildPhysics();
  GameSerializer getSerializer();

public:
  static GraphicalResourceRegistry &getResources()
  {
    return s_instance->m_graphicalResources;
  }

  static Transform getCameraTransform()
  {
    return s_instance->m_freecam.isActive()
      ? Transform{ s_instance->m_freecam.getCamera().getPosition(), vec3{1,1,1,0}, s_instance->m_freecam.getCamera().getRotation() }
    : s_instance->m_playerVehicle->getTransform();
  }

  template<class T> requires std::is_base_of_v<WorldObject, T>
  static std::shared_ptr<T> replaceObject(std::shared_ptr<T> old, std::shared_ptr<T> replacement=nullptr)
  {
    if(old != nullptr) std::erase(s_instance->m_objects, old);
    if(replacement != nullptr) s_instance->m_objects.push_back(replacement);
    return replacement;
  }

  static void setRespawnTransform(const Transform& transform)
  {
    s_instance->m_respawnPosition = transform;
  }
  
  std::string static getFilepathInputText() {
    return s_instance->m_filepathBuffer;
  }

private:
  inline static EditorScene *s_instance = nullptr;

  size_t                               m_objectCount = 0;
  std::shared_ptr<PlayerVehicle>       m_playerVehicle;
  std::vector<std::unique_ptr<WorldObjectEditor>> m_editors;
  Player                               m_player;
  Transform                            m_respawnPosition;
  char                                 m_filepathBuffer[128]{};
  std::string                          m_loadingError;
};