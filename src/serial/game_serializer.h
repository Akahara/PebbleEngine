#pragma once

#include <filesystem>
#include <functional>
#include <unordered_map>

#include "json.h"
#include "physics/physics.h"
#include "scene/game/game_logic.h"
#include "world/object.h"

class WorldObjectEditor;

namespace pbl
{

struct SerializerResources {
  GraphicalResourceRegistry *resources = nullptr;
  pbx::Physics *physics = nullptr;
  std::vector<std::shared_ptr<WorldObject>> *objects = nullptr;
  GameLogic *gameLogic = nullptr; // optional
  std::vector<std::unique_ptr<WorldObjectEditor>> *editors = nullptr; // optional
};

class GameSerializer
{
private:
  using ref_type = std::string;
  using object_type_type = std::string;
  using object_parser_type = std::function<std::shared_ptr<WorldObject>(const JsonObject &)>;
  using object_serializer_type = std::function<std::optional<JsonObject>(const WorldObjectEditor *)>;

  struct ObjectSerializer {
    object_parser_type parse;
    object_serializer_type serialize;
  };

public:
    GameSerializer(const GameSerializer &) = delete;
    GameSerializer &operator=(const GameSerializer &) = delete;
    GameSerializer &operator=(GameSerializer &) = delete;
    GameSerializer(GameSerializer &&moved) noexcept :
        m_resources(std::move(moved.m_resources)),
        m_referencedObjects(std::move(moved.m_referencedObjects))
    {
        fillInParsers();
    }
  explicit GameSerializer(const SerializerResources &resources)
    : m_resources(resources)
  {
    fillInParsers();
  }

  void loadLevelFile(const std::filesystem::path &file);
  void writeLevelFile(const std::filesystem::path &file) const;
  const std::unordered_map<ref_type, std::shared_ptr<WorldObject>> &getReferencedObjects() const { return m_referencedObjects; }
  std::vector<std::pair<std::string, std::shared_ptr<WorldObject>>> getReferencedObjects(const std::string &refStartsWith) const;
  std::shared_ptr<WorldObject> getReferencedObject(const std::string &exactRef) const;

private:
  void fillInParsers();
  void addParser(const object_type_type &name, object_parser_type &&parser, object_serializer_type &&serializer);
  std::string nextEditorName(const std::string &objectType) const;

  static bool loadTransform(const std::shared_ptr<WorldObject> &worldObject, const JsonObject &object);
  static std::optional<layer_t> loadLayer(const std::shared_ptr<WorldObject> &worldObject, const JsonObject &object);
  static std::unique_ptr<WorldObjectEditor> loadEditor(std::unique_ptr<WorldObjectEditor> &&editor, const JsonObject& object);
  static Transform parseTransform(const JsonObject &object);
  static JsonObject serializeTransform(const Transform &transform);
  static vec3 parseVec3(const JsonObject &object);
  static JsonObject serializeVec3(const vec3 &vector);
  static layer_t parseLayer(const JsonObject &object);

private:
  std::unordered_map<object_type_type, ObjectSerializer> m_objectParsers;

  SerializerResources m_resources;

  std::unordered_map<ref_type, std::shared_ptr<WorldObject>> m_referencedObjects;
};

}
