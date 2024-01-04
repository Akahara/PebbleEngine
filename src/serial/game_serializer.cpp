#include "game_serializer.h"

#include <fstream>

#include "json.h"
#include "scene/editor/object_editors.h"
#include "scene/game/track.h"
#include "world/terrain.h"

namespace pbl
{

void GameSerializer::loadLevelFile(const std::filesystem::path &file)
{
  try {
    JsonValue serializedLevel = json::parseFile(file);
    const JsonObject &serializedLevelObject = serializedLevel.asObject();

    for(const JsonValue &serializedObject : serializedLevelObject.getArray("objects")) {
      const JsonObject &serializedObjectObject = serializedObject.asObject();
      std::shared_ptr<WorldObject> object = m_objectParsers[serializedObjectObject.getString("type")].parse(serializedObjectObject);
      if (object == nullptr) continue;
      if (serializedObjectObject.hasField("ref"))
        m_referencedObjects.emplace(serializedObjectObject.getString("ref"), object);
      m_resources.objects->push_back(std::move(object));
    }
  } catch (const std::bad_variant_access &bva) {
    throw std::runtime_error("Could not read level file: " + std::string(bva.what()));
  } catch (const std::out_of_range &oor) {
    throw std::runtime_error("Could not read level file: " + std::string(oor.what()));
  }
}

void GameSerializer::writeLevelFile(const std::filesystem::path &file) const
{
  JsonObject serializedLevel;
  JsonValue::array_type serializedLevelObjects;
  for(const std::unique_ptr<WorldObjectEditor> &editor : *m_resources.editors) {
    bool serializedSuccessfully = false;
    for(auto &[type, serializer] : m_objectParsers) {
      if (std::optional<JsonObject> serializedObject = serializer.serialize(editor.get()); serializedObject.has_value()) {
        serializedSuccessfully = true;
        serializedObject.value().set("type", type);
        if (!editor->getRef().empty())
          serializedObject.value().set("ref", editor->getRef());
        serializedLevelObjects.emplace_back(std::move(serializedObject.value()));
        break;
      }
    }
    PBL_ASSERT(serializedSuccessfully, "No serializer set for an editor");
  }
  serializedLevel.set("objects", JsonValue{ std::move(serializedLevelObjects) });

  std::ofstream{ file } << json::pretty_format << JsonValue{ std::move(serializedLevel) };
}

std::vector<std::pair<std::string, std::shared_ptr<WorldObject>>> GameSerializer::getReferencedObjects(const std::string& refStartsWith) const
{
  std::vector<std::pair<std::string, std::shared_ptr<WorldObject>>> objects;
  for (auto &[ref, object] : m_referencedObjects)
    if (ref.starts_with(refStartsWith))
      objects.emplace_back(ref, object);
  std::ranges::sort(objects, std::less{});
  return objects;
}

std::shared_ptr<WorldObject> GameSerializer::getReferencedObject(const std::string& exactRef) const
{
  return m_referencedObjects.contains(exactRef) ? m_referencedObjects.at(exactRef) : nullptr;
}

void GameSerializer::fillInParsers()
{
  addParser("track",
    [this](const JsonObject &object) {
      std::string bezierFilePath = object.getString("bezier_file");
      BezierCurve bezier = BezierCurve::loadFromFile(bezierFilePath);
      std::vector<Track::AttractionPoint> attractionPoints;
      for(const JsonValue &atValue : object.getArray("attraction_points")) {
        const JsonObject &atObject = atValue.asObject();
        attractionPoints.emplace_back(parseVec3(atObject), atObject.getFloat("strength"));
      }
      Track::TrackProfileTemplate profileTemplate;
      const JsonObject profileObject = object.getObject("profile");
      profileTemplate.innerWidth    = profileObject.getFloat("inner_width");
      profileTemplate.height        = profileObject.getFloat("height");
      profileTemplate.bordersOffset = profileObject.getFloat("borders_offset");
      profileTemplate.bordersWidth  = profileObject.getFloat("borders_width");
      layer_t layer = parseLayer(object);
      // dupplicated code! see TrackEditor#updateTrack
      const wchar_t *texturePath = layer == Layer::BOOST_PLATE ? L"res/textures/boost.dds" : L"res/textures/track.dds";
      auto track = std::make_shared<Track>(bezier, profileTemplate.buildProfile(), attractionPoints, 
        m_resources.resources->loadEffect(L"res/shaders/miniphong.fx", BaseVertex::getShaderVertexLayout()),
        m_resources.resources->loadTexture(texturePath));
      if (m_resources.editors) {
        m_resources.editors->push_back(loadEditor(
          std::make_unique<TrackEditor>(
            nextEditorName("track"),
            track,
            std::move(bezier),
            std::move(bezierFilePath),
            std::move(attractionPoints),
            profileTemplate),
          object));
      }
      loadLayer(track, object);
      return track;
    },
    [this](const WorldObjectEditor *rawEditor) -> std::optional<JsonObject> {
      const auto *editor = dynamic_cast<const TrackEditor*>(rawEditor);
      if (!editor) return std::nullopt;
      JsonObject object;
      object.set("layer", rawEditor->getLayer());
      JsonObject profileObject;
      json::array_type attractionPointsArray;
      for(const Track::AttractionPoint &at : editor->m_attractionPoints) {
        JsonObject atObject = serializeVec3(at.position);
        atObject.set("strength", at.strength);
        attractionPointsArray.emplace_back(std::move(atObject));
      }
      profileObject.set("inner_width",    editor->m_profileTemplate.innerWidth   );
      profileObject.set("height",         editor->m_profileTemplate.height       );
      profileObject.set("borders_offset", editor->m_profileTemplate.bordersOffset);
      profileObject.set("borders_width",  editor->m_profileTemplate.bordersWidth );
      object.set("profile", std::move(profileObject));
      object.set("bezier_file", editor->m_curveFilePath);
      object.set("attraction_points", std::move(attractionPointsArray));
      BezierCurve::writeToFile(editor->m_curveFilePath, editor->m_curve);
      return object;
    }
  );
  addParser("terrain",
    [this](const JsonObject &object) {
      TerrainSettings settings{};
      const JsonObject &settingsObject = object.getObject("settings");
      settings.worldWidth  = settingsObject.getFloat("width");
      settings.worldHeight = settingsObject.getFloat("height");
      settings.worldZScale = settingsObject.getFloat("zscale");
      settings.uvScale     = settingsObject.getFloat("uvscale");
      std::string heightmapFilePath = object.getString("heightmap_file");
      auto terrain = std::make_shared<Terrain>(heightmapFilePath.c_str(), *m_resources.resources, settings);
      if(loadTransform(terrain, object)) terrain->updateTransform();
      loadLayer(terrain, object);
      if (m_resources.editors) m_resources.editors->push_back(
        loadEditor(std::make_unique<TerrainEditor>(nextEditorName("terrain"), terrain, settings, heightmapFilePath), object));
      return terrain;
    },
    [this](const WorldObjectEditor *rawEditor) -> std::optional<JsonObject> {
      const auto *editor = dynamic_cast<const TerrainEditor*>(rawEditor);
      if (!editor) return std::nullopt;
      JsonObject object;
      object.set("layer", rawEditor->getLayer());
      JsonObject settingsObject;
      settingsObject.set("width",   editor->m_settings.worldWidth );
      settingsObject.set("height",  editor->m_settings.worldHeight);
      settingsObject.set("zscale",  editor->m_settings.worldZScale);
      settingsObject.set("uvscale", editor->m_settings.uvScale    );
      object.set("settings", std::move(settingsObject));
      object.set("transform", serializeTransform(editor->m_terrain->getTransform()));
      object.set("heightmap_file", editor->m_heightmapFile);
      return object;
    }
  );
  addParser("prop",
    [this](const JsonObject &object) {
      std::string shaderFile = object.getString("shader_file");
      std::string modelFile = object.getString("model_file");
      std::shared_ptr worldProp = WorldProp::makePhysicsfullObjectFromFile(*m_resources.resources, utils::string2widestring(modelFile), utils::string2widestring(shaderFile));
      loadTransform(worldProp, object);
      loadLayer(worldProp, object);
      if (m_resources.editors) m_resources.editors->push_back(loadEditor(std::make_unique<WorldPropEditor>(nextEditorName("prop"), worldProp, modelFile, shaderFile), object));
      return worldProp;
    },
    [this](const WorldObjectEditor *rawEditor) -> std::optional<JsonObject> {
      const auto *editor = dynamic_cast<const WorldPropEditor *>(rawEditor);
      if (!editor) return std::nullopt;
      JsonObject object;
      object.set("layer", rawEditor->getLayer());
      object.set("model_file", editor->m_modelFilePath);
      object.set("shader_file", editor->m_effectFilePath);
      object.set("transform", serializeTransform(editor->m_worldProp->getTransform()));
      return object;
    }
  );
  addParser("triggerbox",
    [this](const JsonObject &object) {
      auto triggerBox = std::make_shared<TriggerBox>(parseTransform(object.getObject("transform")));
      loadLayer(triggerBox, object);
      if (m_resources.editors) m_resources.editors->push_back(loadEditor(std::make_unique<TriggerBoxEditor>(nextEditorName("triggerbox"), triggerBox), object));
      return triggerBox;
    },
    [this](const WorldObjectEditor *rawEditor) -> std::optional<JsonObject> {
      const auto *editor = dynamic_cast<const TriggerBoxEditor *>(rawEditor);
      if (!editor) return std::nullopt;
      JsonObject object;
      object.set("layer", rawEditor->getLayer());
      object.set("transform", serializeTransform(editor->m_triggerBox->getTransform()));
      return object;
    }
  );
  addParser("gamelogic",
    [this](const JsonObject &object) {
      Transform playerSpawnTransform = parseTransform(object.getObject("player_spawn"));
      std::string postEndTrackFilePath = object.getString("post_end_track_file");
      BezierCurve postEndTrack = BezierCurve::loadFromFile(postEndTrackFilePath);
      vec3 endCameraOffset = parseVec3(object.getObject("end_camera_offset"));

      if(m_resources.gameLogic) {
        m_resources.gameLogic->setCheckpointPosition(playerSpawnTransform);
        m_resources.gameLogic->setPostEndTrack(postEndTrack.discretizeEvenly());
        m_resources.gameLogic->getPlayer()->getFixedCamera().setOffset(endCameraOffset);
        m_resources.gameLogic->getPlayer()->resetPosition(playerSpawnTransform);
      }

      if(m_resources.editors) {
        auto editor = std::make_unique<GameLogicEditor>(nextEditorName("gamelogic"), std::move(postEndTrackFilePath), std::move(postEndTrack), endCameraOffset);
        editor->m_playerInitialSpawnTransform = playerSpawnTransform;
        m_resources.editors->emplace_back(std::move(editor));
      }

      return nullptr;
    },
    [this](const WorldObjectEditor *rawEditor) -> std::optional<JsonObject> {
      const auto *editor = dynamic_cast<const GameLogicEditor *>(rawEditor);
      if (!editor) return std::nullopt;
      JsonObject object;
      BezierCurve::writeToFile(editor->m_autoPlayTrackFile, editor->m_autoPlayTrack);
      object.set("player_spawn", serializeTransform(editor->m_playerInitialSpawnTransform));
      object.set("post_end_track_file", editor->m_autoPlayTrackFile);
      object.set("end_camera_offset", serializeVec3(editor->m_endCameraOffset));
      return object;
    }
  );
  addParser("billboard",
    [this](const JsonObject &object) {
      auto billboardObject = std::make_shared<BillboardsObject>(*m_resources.resources);
      std::vector<std::string> texturePaths;
      for(const JsonValue &bb : object.getArray("billboards")) {
        const JsonObject &bbObject = bb.asObject();
        const std::string &texturePath = bbObject.getString("texture");
        texturePaths.push_back(texturePath);
        billboardObject->getBillboards().push_back(BillBoard{
          m_resources.resources->loadTexture(utils::string2widestring(texturePath)),
          parseVec3(bbObject),
          rvec2{ bbObject.getFloat("w"), bbObject.getFloat("h") },
          bbObject.getFloat("tx"),
          bbObject.getFloat("ty"),
          bbObject.getFloat("tw"),
          bbObject.getFloat("th"),
        });
      }
      if(m_resources.editors) {
        m_resources.editors->push_back(loadEditor(
          std::make_unique<BillboardsEditor>(nextEditorName("billboards"), billboardObject, std::move(texturePaths)), object));
      }
      return billboardObject;
    },
    [this](const WorldObjectEditor *rawEditor) -> std::optional<JsonObject> {
      const BillboardsEditor *editor = dynamic_cast<const BillboardsEditor *>(rawEditor);
      if (!editor) return std::nullopt;
      JsonObject object;
      json::array_type billboards;
      for(size_t i = 0; i < editor->m_billboardTextures.size(); i++) {
        const BillBoard &bb = editor->m_object->getBillboards()[i];
        JsonObject bbObject = serializeVec3(bb.position);
        bbObject.set("w", bb.scale.x);
        bbObject.set("h", bb.scale.y);
        bbObject.set("tx", bb.texX);
        bbObject.set("ty", bb.texY);
        bbObject.set("tw", bb.texW);
        bbObject.set("th", bb.texH);
        bbObject.set("texture", editor->m_billboardTextures[i]);
        billboards.emplace_back(std::move(bbObject));
      }
      object.set("billboards", std::move(billboards));
      return object;
    }
  );
  addParser("camerarail",
    [this](const JsonObject &object) {
      std::vector<vec3> points;
      std::ranges::transform(object.getArray("points"), std::back_inserter(points), [](const JsonValue &p) { return parseVec3(p.asObject()); });

      if(m_resources.gameLogic) {
        m_resources.gameLogic->getPlayer()->getFixedCamera().addRail({ points });
      }

      if(m_resources.editors) {
        auto editor = std::make_unique<CameraRailEditor>(nextEditorName("camerarail"));
        editor->m_points = points;
        m_resources.editors->emplace_back(std::move(editor));
      }

      return nullptr;
    },
    [this](const WorldObjectEditor *rawEditor) -> std::optional<JsonObject> {
      const auto *editor = dynamic_cast<const CameraRailEditor *>(rawEditor);
      if (!editor) return std::nullopt;
      json::array_type points;
      std::ranges::transform(editor->m_points, std::back_inserter(points), serializeVec3);
      JsonObject object;
      object.set("points", std::move(points));
      return object;
    }
  );
  addParser("tunnel",
    [this](const JsonObject &object) {
      auto tunnel = std::make_shared<Tunnel>(*m_resources.resources);
      loadTransform(tunnel, object);

      if(m_resources.editors) {
        auto editor = std::make_unique<TunnelEditor>(nextEditorName("tunnel"), tunnel);
        m_resources.editors->emplace_back(std::move(editor));
      }

      return tunnel;
    },
    [this](const WorldObjectEditor *rawEditor) -> std::optional<JsonObject> {
      const auto *editor = dynamic_cast<const TunnelEditor *>(rawEditor);
      if (!editor) return std::nullopt;
      JsonObject object;
      object.set("transform", serializeTransform(editor->m_tunnel->getTransform()));
      return object;
    }
  );
}

void GameSerializer::addParser(const object_type_type &name, object_parser_type &&parser, object_serializer_type &&serializer)
{
  PBL_ASSERT(!m_objectParsers.contains(name), "Already registered parser type: " + name);
  m_objectParsers.emplace(name, ObjectSerializer{ std::move(parser), std::move(serializer) });
}

std::string GameSerializer::nextEditorName(const std::string &objectType) const
{
  return objectType + "_#" + std::to_string(m_resources.editors->size());
}

bool GameSerializer::loadTransform(const std::shared_ptr<WorldObject> &worldObject, const JsonObject &object)
{
  if(object.hasField("transform")) {
    worldObject->getTransform() = parseTransform(object.getObject("transform"));
    return true;
  } else {
    return false;
  }
}

std::optional<layer_t> GameSerializer::loadLayer(const std::shared_ptr<WorldObject> &worldObject, const JsonObject &object)
{
  if(object.hasField("layer")) {
    layer_t layer = static_cast<layer_t>(object.getInt("layer"));
    worldObject->setLayer(layer);
    return layer;
  } else {
    return std::nullopt;
  }
}

std::unique_ptr<WorldObjectEditor> GameSerializer::loadEditor(std::unique_ptr<WorldObjectEditor> &&editor, const JsonObject &object)
{
  if (object.hasField("ref")) {
    editor->setRef(object.getString("ref"));
  }
  if (object.hasField("layer")) {
    layer_t layer = static_cast<layer_t>(object.getInt("layer"));
    editor->setLayer(layer);
  }
  return std::move(editor);
}

Transform GameSerializer::parseTransform(const JsonObject &object)
{
  rvec4 p{ 0,0,0,1 };
  rvec4 s{ 1,1,1,0 };
  rvec4 q{ 0,0,0,1 };
  if (object.hasField("x"))  p.x = object.getFloat("x");
  if (object.hasField("y"))  p.y = object.getFloat("y");
  if (object.hasField("z"))  p.z = object.getFloat("z");
  if (object.hasField("w"))  s.x = object.getFloat("w");
  if (object.hasField("h"))  s.y = object.getFloat("h");
  if (object.hasField("d"))  s.z = object.getFloat("d");
  if (object.hasField("qx")) q.x = object.getFloat("qx");
  if (object.hasField("qy")) q.y = object.getFloat("qy");
  if (object.hasField("qz")) q.z = object.getFloat("qz");
  if (object.hasField("qw")) q.w = object.getFloat("qw");
  return Transform{ XMLoadFloat4(&p), XMLoadFloat4(&s), XMLoadFloat4(&q) };
}

JsonObject GameSerializer::serializeTransform(const Transform &transform)
{
  rvec3 p{};
  rvec3 s{};
  rvec4 q{};
  XMStoreFloat3(&p, transform.position);
  XMStoreFloat3(&s, transform.scale);
  XMStoreFloat4(&q, transform.rotation);
  JsonObject object;
  if (p.x != 0.f) object.set("x", p.x);
  if (p.y != 0.f) object.set("y", p.y);
  if (p.z != 0.f) object.set("z", p.z);
  if (s.x != 1.f) object.set("w", s.x);
  if (s.y != 1.f) object.set("h", s.y);
  if (s.z != 1.f) object.set("d", s.z);
  if (q.x != 0.f) object.set("qx", q.x);
  if (q.y != 0.f) object.set("qy", q.y);
  if (q.z != 0.f) object.set("qz", q.z);
  if (q.w != 1.f) object.set("qw", q.w);
  return object;
}

vec3 GameSerializer::parseVec3(const JsonObject& object)
{
  return {
    object.getFloat("x"),
    object.getFloat("y"),
    object.getFloat("z"),
  };
}

JsonObject GameSerializer::serializeVec3(const vec3& vector)
{
  JsonObject object;
  object.set("x", XMVectorGetX(vector));
  object.set("y", XMVectorGetY(vector));
  object.set("z", XMVectorGetZ(vector));
  return object;
}

layer_t GameSerializer::parseLayer(const JsonObject& object)
{
  return static_cast<layer_t>(object.getInt("layer"));
}

}
