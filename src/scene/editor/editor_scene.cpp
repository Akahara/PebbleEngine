#include "editor_scene.h"

#include "object_editors.h"
#include "inputs/user_inputs.h"
#include "scene/game/ground.h"
#include "scene/game/player_vehicle.h"
#include "scene/game/sun.h"
#include "serial/game_serializer.h"

static constexpr const char *LEVEL_FILE = "res/level/level.json";
static constexpr const char *LAYER_NAMES[] = { "Default", "Player", "Trigger", "StickyRoad", "BoostPlate" };

bool WorldObjectEditor::render()
{
  if (ImGui::InputText("ref", m_refEditionBuffer, std::size(m_refEditionBuffer)))
    updateDisplayName();
  return false;
}

void WorldObjectEditor::setLayer(layer_t layer)
{
  m_layer = layer;
  m_activeLayer = static_cast<int>(mathf::firstBitIndex(layer));
}

bool WorldObjectEditor::renderLayerSelector(const std::shared_ptr<WorldObject>& object)
{
  if(m_layer & ~(1<<mathf::firstBitIndex(m_layer))) {
    ImGui::Text("Layer (multiple set, edit in .json file)");
  } else if (ImGui::Combo("Layer", &m_activeLayer, LAYER_NAMES, static_cast<int>(std::size(LAYER_NAMES)))) {
    m_layer = static_cast<layer_t>(1 << m_activeLayer);
    object->setLayer(m_layer);
    return true;
  }
  return false;
}

EditorScene::EditorScene()
{
  s_instance = this;
  m_playerVehicle = std::make_shared<PlayerVehicle>(m_graphicalResources);
  m_player.setVehicleObject(m_playerVehicle.get());
  m_objects.push_back(m_playerVehicle);
  m_objects.push_back(std::make_shared<Sun>(m_graphicalResources, &m_sprites));

  getSerializer().loadLevelFile(LEVEL_FILE);
  m_objectCount = m_editors.size();

  { // initialize game logic elements (there is no actual game logic, but some fields may still be used)
    for(auto &editor : m_editors) {
      auto gameLogicEditor = dynamic_cast<GameLogicEditor*>(editor.get());
      if (!gameLogicEditor) continue;
      m_respawnPosition = gameLogicEditor->m_playerInitialSpawnTransform;
    }
  }

  m_objects.push_back(std::make_shared<Ground>(m_graphicalResources));

  rebuildPhysics();
}

void EditorScene::update(double delta)
{
  m_pausePhysics = m_freecam.isActive();
  ShowcaseScene::update(delta);

  if(m_freecam.isActive()) {
    m_freecam.processUserInputs(delta);
  } else {
    m_player.update(delta);
  }

  if (pbl::UserInputs::consumeKeyPress(keys::SC_TAB))
    m_graphicalResources.reloadShaders();

  for (auto &object : m_objects)
    object->update(delta);
}

void EditorScene::rebuildPhysics()
{
  // rebuild physics
  m_physics.clearScene();
  for (auto &worldObject : m_objects) {
    PhysicsBody *physics = worldObject->buildPhysicsObject();
    if (physics) m_physics.addBody(physics);
  }
}

GameSerializer EditorScene::getSerializer()
{
  SerializerResources serializerResources;
  serializerResources.editors = &m_editors;
  serializerResources.gameLogic = nullptr; // no game logic
  serializerResources.objects = &m_objects;
  serializerResources.physics = &m_physics;
  serializerResources.resources = &m_graphicalResources;
  GameSerializer serializer{ serializerResources };
  return serializer;
}

#ifdef PBL_HASGUI
static bool ImGuiButton(const char *name, ImVec4 color) {
  ImGui::PushStyleColor(ImGuiCol_Button, color);
  bool pressed = ImGui::Button(name);
  ImGui::PopStyleColor();
  return pressed;
}

static void ImGuiTooltip(const char *fmt) {
  if (ImGui::IsItemHovered())
    ImGui::SetTooltip(fmt);
}
#else
static bool ImGuiButton(const char *name, rvec4 color) { return false; }
MOCK_IMGUI_FUNC(ImGuiTooltip);
#endif

void EditorScene::render()
{
  int editorUpdates = 0;
  logs::scene.beginWindow();

  // misc controls
  if(ImGui::Checkbox("Edit", &m_freecam.isActive())) {
    rebuildPhysics();
    m_player.resetPosition(m_respawnPosition);
  }
  if (ImGui::Button("Reset spawn here")) {
    m_respawnPosition = m_freecam.isActive()
      ? Transform{ m_freecam.getCamera().getPosition(), vec3{ 1,1,1,1 }, m_freecam.getCamera().getRotation() }
      : m_playerVehicle->getTransform();
  }

  // edit active items controls
  for (size_t i = 0; i < m_editors.size(); i++) {
    auto &editor = m_editors[i];
    ImGui::PushID(static_cast<int>(i));
    if (ImGui::CollapsingHeader((editor->getDisplayName() + "###displayname").c_str())) {
      editorUpdates += editor->render();
      if(ImGuiButton("delete", { 0.735f, 0.190f, 0.190f, 1.f }))
        m_editors.erase(m_editors.begin() + i--);
    }
    ImGui::PopID();
    ImGui::Separator();
    ImGui::Separator();
  }

  // add item buttons
  ImGui::PushStyleColor(ImGuiCol_Button, { 0.311f, 0.671f, 0.221f, 1.f });
  ImGui::InputText("File path", m_filepathBuffer, sizeof(m_filepathBuffer));
  ImGuiTooltip("Used for:\n- terrain heightmap file\n- world prop model file");
  try {
    if(ImGui::Button("+Track"))
      m_editors.push_back(std::make_unique<TrackEditor>("track_#" + std::to_string(m_objectCount++), m_freecam.getActiveCamera(m_player.getActiveCamera())));
    ImGui::SameLine();
    if (ImGui::Button("+Terrain"))
      m_editors.push_back(std::make_unique<TerrainEditor>("terrain_#" + std::to_string(m_objectCount++), m_filepathBuffer));
    ImGui::SameLine();
    if (ImGui::Button("+Prop"))
        m_editors.push_back(std::make_unique<WorldPropEditor>("prop_#" + std::to_string(m_objectCount++), m_filepathBuffer, std::string{}));
    ImGui::SameLine();
    if (ImGui::Button("+Tunnel"))
      m_editors.push_back(std::make_unique<TunnelEditor>("tunnel_#" + std::to_string(m_objectCount++)));
    if (ImGui::Button("+CameraRail"))
      m_editors.push_back(std::make_unique<CameraRailEditor>("camerarail_#" + std::to_string(m_objectCount++)));
    ImGui::SameLine();
    if (ImGui::Button("+Billboards"))
      m_editors.push_back(std::make_unique<BillboardsEditor>("billboards_#" + std::to_string(m_objectCount++)));
    ImGui::SameLine();
    if (ImGui::Button("+TriggerBox"))
      m_editors.push_back(std::make_unique<TriggerBoxEditor>("triggerbox_#" + std::to_string(m_objectCount++), m_freecam.getCamera().getPosition() + m_freecam.getCamera().getForward() * 10));
  } catch (const std::exception &loadingError) {
    m_loadingError = loadingError.what();
  }
  ImGui::PopStyleColor();

  // save level file button
  try {
    if(ImGuiButton("Save", { 0.690f, 0.274f, 0.818f, 1.f })) {
      getSerializer().writeLevelFile(LEVEL_FILE);
    }
    ImGui::SameLine();
    if(m_freecam.isActive() && ImGuiButton("Reload file", { 0.947f, 0.150f, 0.150f, 1.f })) {
      m_editors.clear();
      m_physics.clearScene();
      getSerializer().loadLevelFile(LEVEL_FILE);
    }
  } catch (const std::exception &savingError) {
    m_loadingError = savingError.what();
  }

  // error text
  ImGui::PushStyleColor(ImGuiCol_Text, { 0.735f, 0.190f, 0.190f, 1.f });
  ImGui::TextUnformatted(m_loadingError.c_str());
  ImGui::PopStyleColor();

  ImGui::End();

  const Camera &activeCamera = m_freecam.getActiveCamera(m_player.getActiveCamera());
  renderer::renderWorldAxes(activeCamera);

  ShowcaseScene::render(activeCamera);
}

