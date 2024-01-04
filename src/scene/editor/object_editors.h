#pragma once

#include "editor_scene.h"
#include "scene/game/billboard_object.h"
#include "scene/game/tunnel.h"
#include "world/trigger_box.h"

namespace pbl
{
  class GameSerializer;
}

/*
 * A generic list editor, with a single selected editable item at a time.
 * The way to use this class is to create an instance and assign all the
 * fields in it, then use the render() method every frame.
 */
template<class T>
struct GUIListEditor {
public:
  GUIListEditor() = default;

  bool render();

  // the 3 main functions, they must be set for this editor to be usable
  std::function<bool(T &)> renderItem;
  std::function<std::optional<T>(int slot)> createItem;
  std::function<void(int slot, T &)> deleteItem = [](int, T&) {};

  // the actual items collection, not owned
  std::vector<T> *items;
  // optional, if a title is provided this list will be collapseable
  std::string title;
  // optional
  size_t minItemCount = 0;

private:
  int m_selectedItem = 0;
};

template <class T>
bool GUIListEditor<T>::render()
{
  if (!title.empty()) {
    if (!ImGui::CollapsingHeader(title.c_str()))
      return false;
    ImGui::PushID(title.c_str());
  }

  int updates = 0;

  if(!items->empty()) {
    ImGui::SliderInt("edited item", &m_selectedItem, 0, (int)items->size()-1);
    updates += renderItem(items->at(m_selectedItem));
  }

  if(ImGui::Button("+")) {
    std::optional<T> newItem = createItem(m_selectedItem);
    if (newItem.has_value()) {
      if(m_selectedItem == items->size()-1) {
        items->push_back(std::move(newItem.value()));
        m_selectedItem++;
      } else {
        items->insert(items->begin() + m_selectedItem, std::move(newItem.value()));
        if (m_selectedItem < items->size()-1) m_selectedItem++;
      }
      updates++;
    }
  }
  ImGui::SameLine();
  if(ImGui::Button("-") && items->size() > minItemCount) {
    deleteItem(m_selectedItem, items->at(m_selectedItem));
    items->erase(items->begin() + m_selectedItem);
    if (m_selectedItem > 0) m_selectedItem--;
    updates++;
  }

  if (!title.empty())
    ImGui::PopID();

  return updates > 0;
}

class TransformControls {
public:
  explicit TransformControls(bool allowTranslation=true, bool allowScaling=true, bool allowRotation=true)
    : m_allowTranslation(allowTranslation), m_allowScaling(allowScaling), m_allowRotation(allowRotation) {}

  bool render(Transform &transform);

  void loadRotation(quat rotationQuaternion);

  static bool renderDrag3(const char *label, vec3 *vec, float min=0, float max=0);

private:
  bool m_allowTranslation, m_allowScaling, m_allowRotation;
  float m_eulerRotation[3]{};
};

/*
* utility that renders imgui controls for live edition
* make sure to give a bezier curve with at lease 2 point
*/
struct BezierControls {
  explicit BezierControls(BezierCurve *bezier);

  bool render();
  void renderDiscretized();

private:
  BezierControlPoint createPoint(int index) const;
  static bool renderPoint(BezierControlPoint &point);

private:
  BezierCurve *m_bezier;
  DiscreteCurve m_discretized;
  GUIListEditor<BezierControlPoint> m_controlPointsEditor;
};

class GameLogicEditor : public WorldObjectEditor {
public:
  GameLogicEditor(const std::string &name, std::string autoPlayTrackFile, BezierCurve autoPlayTrack, vec3 endCameraOffset)
    : WorldObjectEditor(name)
    , m_autoPlayTrackFile(std::move(autoPlayTrackFile))
    , m_autoPlayTrack(std::move(autoPlayTrack))
    , m_autoPlayTrackControls(&m_autoPlayTrack)
    , m_endCameraOffset(endCameraOffset)
  {}

  bool render() override;

private:
  friend GameSerializer;
  friend EditorScene; // not ideal, but as the EditorScene does not have a proper GameLogic instance,
                      // it will use some fields of this class in a way that this class cannot handle
  std::string m_autoPlayTrackFile;
  BezierCurve m_autoPlayTrack;
  BezierControls m_autoPlayTrackControls;
  Transform m_playerInitialSpawnTransform;
  vec3 m_endCameraOffset;
};

class TrackEditor : public WorldObjectEditor {
public:
  TrackEditor(const std::string &name, const Camera &activeCamera);
  TrackEditor(const std::string &name, std::shared_ptr<Track> track, BezierCurve bezier, std::string bezierFilePath,
    std::vector<Track::AttractionPoint> attractionPoints, Track::TrackProfileTemplate profileTemplate);
  ~TrackEditor() override;

  bool render() override;

private:
  void initAttractionPointsEditor();
  void updateTrack();

private:
  friend class GameSerializer;
  std::shared_ptr<Track>      m_track;
  BezierCurve                 m_curve;
  BezierControls              m_curveControls;
  std::string                 m_curveFilePath;
  Track::TrackProfileTemplate m_profileTemplate;
  GUIListEditor<Track::AttractionPoint> m_attractionPointsEditor;
  std::vector<Track::AttractionPoint> m_attractionPoints;
};

class TerrainEditor : public WorldObjectEditor {
public:
  TerrainEditor(const std::string &name, std::string heightmapFile);
  TerrainEditor(const std::string &name, const std::shared_ptr<Terrain> &terrain, TerrainSettings terrainSettings, const std::string &heightmapFilePath);
  ~TerrainEditor() override;

  bool render() override;

private:
  friend class GameSerializer;
  std::shared_ptr<Terrain> m_terrain;
  std::string              m_heightmapFile;
  TerrainSettings          m_settings;
  TransformControls        m_transformControls;
};

class WorldPropEditor : public WorldObjectEditor {
public:
  WorldPropEditor(const std::string &name, const std::string &modelFilePath, const std::string &shaderFilePath);
  WorldPropEditor(const std::string &name, const std::shared_ptr<WorldProp> &worldProp, std::string modelFilePath, std::string shaderFilePath);
  ~WorldPropEditor() override;

  bool render() override;

private:
  friend class GameSerializer;
  std::shared_ptr<WorldProp> m_worldProp;
  std::string                m_modelFilePath;
  std::string                m_effectFilePath;
  char                       m_effectEditionBuffer[50]{};
  TransformControls          m_transformControls;
};

class TriggerBoxEditor : public WorldObjectEditor {
public:
  TriggerBoxEditor(const std::string &name, vec3 position);
  TriggerBoxEditor(const std::string &name, const std::shared_ptr<TriggerBox> &triggerBox);
  ~TriggerBoxEditor() override;

  bool render() override;

private:
  void replaceTriggetCallback() const;

private:
  friend class GameSerializer;
  std::shared_ptr<TriggerBox> m_triggerBox;
  TransformControls           m_transformControls;
};

class BillboardsEditor : public WorldObjectEditor {
public:
  explicit BillboardsEditor(const std::string &name);
  BillboardsEditor(const std::string &name, const std::shared_ptr<BillboardsObject> &object, std::vector<std::string> texturePaths);
  ~BillboardsEditor() override;

  bool render() override;

private:
  void initEditor();

private:
  friend class GameSerializer;
  std::shared_ptr<BillboardsObject> m_object;
  std::vector<std::string> m_billboardTextures;
  GUIListEditor<BillBoard> m_billboardListEditor;
};

class CameraRailEditor : public WorldObjectEditor {
public:
  explicit CameraRailEditor(const std::string &name);

  bool render() override;
  CameraRail createRail() const;

private:
  friend GameSerializer;
  GUIListEditor<vec3> m_pointsEditor;
  std::vector<vec3> m_points;
};

class TunnelEditor : public WorldObjectEditor {
public:
  explicit TunnelEditor(const std::string &name);
  TunnelEditor(const std::string &name, std::shared_ptr<Tunnel> tunnel);
  ~TunnelEditor() override;

  bool render() override;

private:
  friend GameSerializer;
  std::shared_ptr<Tunnel> m_tunnel;
  TransformControls m_transformControls;
};