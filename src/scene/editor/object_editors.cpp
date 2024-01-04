#include "object_editors.h"

#include "utils/math.h"
#include "utils/debug.h"
#include <string>

bool TransformControls::render(Transform &transform)
{
  int updates = 0;
  renderer::renderAxes(transform);
  if(m_allowTranslation)
    updates += ImGui::DragFloat3("position", reinterpret_cast<float*>(&transform.position));
  if(m_allowScaling)
    updates += ImGui::DragFloat3("scale", reinterpret_cast<float*>(&transform.scale), .5f, .1f, 1000.f);
  if (m_allowRotation && ImGui::DragFloat3("pitch/yaw/roll", m_eulerRotation)) {
    transform.rotation = XMQuaternionRotationRollPitchYaw(m_eulerRotation[0]*PI/180.f, m_eulerRotation[1]*PI/180.f, m_eulerRotation[2]*PI/180.f);
    updates++;
  }
  return updates > 0;
}

void TransformControls::loadRotation(quat rotationQuaternion)
{
  // FIX loadRotation does not conserve rotations!
  // https://gamedev.stackexchange.com/questions/35476/how-to-derive-euler-angles-from-matrix-or-quaternion
  rvec4 self; XMStoreFloat4(&self, rotationQuaternion);
  float t = self.x*self.y + self.z*self.w;
  float sqx = self.x * self.x;
  float sqy = self.y * self.y;
  float sqz = self.z * self.z;
  float heading = std::atan2(2 * self.y * self.w - 2 * self.x * self.z, 1 - 2 * sqy - 2 * sqz);
  float attitude = std::asin(2 * t);
  float bank = std::atan2(2 * self.x * self.w - 2 * self.y * self.z, 1 - 2 * sqx - 2 * sqz);
  m_eulerRotation[0] = heading*180.f/PI;
  m_eulerRotation[1] = attitude*180.f/PI;
  m_eulerRotation[2] = bank*180.f/PI;
}

bool TransformControls::renderDrag3(const char *label, vec3 *vec, float min, float max)
{
  rvec3 rvec;
  XMStoreFloat3(&rvec, *vec);
  bool updated = ImGui::DragFloat3(label, &rvec.x);
  *vec = XMLoadFloat3(&rvec);
  return updated;
}

BezierControls::BezierControls(BezierCurve *bezier)
  : m_bezier(bezier)
{
  m_controlPointsEditor.title = "Control points";
  m_controlPointsEditor.items = &bezier->controlPoints;
  m_controlPointsEditor.minItemCount = 2;
  m_controlPointsEditor.createItem = [this](int index) { return createPoint(index); };
  m_controlPointsEditor.renderItem = renderPoint;
  m_discretized = m_bezier->discretizeEvenly();
}

bool BezierControls::render()
{
  pbl::renderer::renderCurve(*m_bezier);
  if (m_controlPointsEditor.render()) {
    m_discretized = m_bezier->discretizeEvenly();
    return true;
  }
  return false;
}

void BezierControls::renderDiscretized()
{
  pbl::renderer::renderCurve(m_discretized);
}

BezierControlPoint BezierControls::createPoint(int index) const
{
  const BezierControlPoint &copyCp = m_bezier->controlPoints[index];
  if (index == m_bezier->controlPoints.size()-1) {
    BezierControlPoint newPoint;
    vec3 dir = XMVector3Normalize(copyCp.handleRight - copyCp.position)*3.f;
    newPoint.position = copyCp.position + dir * 3;
    newPoint.handleLeft = -dir + newPoint.position;
    newPoint.handleRight = dir + newPoint.position;
    return newPoint;
  } else {
    const BezierControlPoint &targetCp = m_bezier->controlPoints[index+1];
    BezierControlPoint newPoint;
    vec3 dir = XMVector3Normalize(targetCp.position - copyCp.position)*3.f;
    newPoint.position = (targetCp.position + copyCp.position)*.5f;
    newPoint.handleLeft = -dir + newPoint.position;
    newPoint.handleRight = dir + newPoint.position;
    return newPoint;
  }
}

bool BezierControls::renderPoint(BezierControlPoint &activeControlPoint) {
    Transform cubeT{};
    cubeT.position = activeControlPoint.position;
    cubeT.scale = { .1f,.1f,.1f };
  pbl::renderer::renderCube(cubeT.getWorldMatrix(), 0xffaaaaff);
  pbl::renderer::renderLine(activeControlPoint.position, activeControlPoint.handleLeft,  0xff0000ff);
  pbl::renderer::renderLine(activeControlPoint.position, activeControlPoint.handleRight, 0xff0000ff);

  int updates = 0;

  { // position slider
    rvec3 rpos;
    XMStoreFloat3(&rpos, activeControlPoint.position);
    updates += ImGui::DragFloat3("Position", &rpos.x, .05f);
    vec3 dp = activeControlPoint.position - XMLoadFloat3(&rpos);
    activeControlPoint.position    -= dp;
    activeControlPoint.handleRight -= dp;
    activeControlPoint.handleLeft  -= dp;
  }

  { // handle slider
    rvec3 handle;
    vec3 handleDir = activeControlPoint.handleRight - activeControlPoint.position;
    float handleLen = XMVectorGetX(XMVector3Length(handleDir));
    handle.x = std::asin(XMVectorGetY(handleDir) / handleLen);
    handle.y = std::atan2(XMVectorGetZ(handleDir), XMVectorGetX(handleDir));
    handle.z = handleLen;
    if (ImGui::SliderFloat("Handle pitch", &handle.x, -XM_PI *.49f, +XM_PI *.49f) +
        ImGui::SliderFloat("Handle yaw",   &handle.y, -XM_PI, +XM_PI) +
        ImGui::DragFloat("Handle length",  &handle.z, .05f, 100.f)) {
      updates++;
      vec3 newHandleDir = vec3{ std::cos(handle.x) * std::cos(handle.y), std::sin(handle.x), std::cos(handle.x) * std::sin(handle.y) } * handle.z;
      activeControlPoint.handleRight = activeControlPoint.position + newHandleDir;
      activeControlPoint.handleLeft = activeControlPoint.position - newHandleDir;
    }
  }

  return updates > 0;
}

bool GameLogicEditor::render()
{
  if(ImGui::Button("Set initial spawn here")) {
    m_playerInitialSpawnTransform = EditorScene::getCameraTransform();
    EditorScene::setRespawnTransform(m_playerInitialSpawnTransform);
  }

  TransformControls::renderDrag3("End camera offset", &m_endCameraOffset);

  if(ImGui::CollapsingHeader("Auto-Play track")) {
    m_autoPlayTrackControls.render();
    m_autoPlayTrackControls.renderDiscretized();
  }

  mat4 tMat = XMMatrixTranslationFromVector(m_playerInitialSpawnTransform.position);
  mat4 sMat = XMMatrixScaling(.1f, .1f, .1f);
  renderer::renderCube(XMMatrixMultiply(tMat, sMat), 0xffaabbcc);
  renderer::renderAxes(m_playerInitialSpawnTransform);

  return false;
}

TrackEditor::TrackEditor(const std::string &name, const Camera &activeCamera)
  : WorldObjectEditor(name)
  , m_curveControls(&m_curve)
  , m_curveFilePath("res/level/bezier_" + std::to_string(std::rand()%100000) + ".txt")
{
  vec3 dir = activeCamera.getFlatForward();
  vec3 p1 = activeCamera.getPosition() + dir - Camera::UP * 10;
  vec3 p2 = p1 + 20 * dir;

  m_curve.controlPoints.push_back({ p1, p1 - dir, p1 + dir });
  m_curve.controlPoints.push_back({ p2, p2 - dir, p2 + dir });
  updateTrack();
  initAttractionPointsEditor();
}

TrackEditor::TrackEditor(
  const std::string &name,
  std::shared_ptr<Track> track,
  BezierCurve bezier,
  std::string bezierFilePath,
  std::vector<Track::AttractionPoint> attractionPoints,
  Track::TrackProfileTemplate profileTemplate)
  : WorldObjectEditor(name)
  , m_track(std::move(track))
  , m_curve(std::move(bezier))
  , m_curveControls(&m_curve)
  , m_curveFilePath(std::move(bezierFilePath))
  , m_profileTemplate(profileTemplate)
  , m_attractionPoints(std::move(attractionPoints))
{
  initAttractionPointsEditor();
}

TrackEditor::~TrackEditor()
{
  EditorScene::replaceObject(m_track);
}

bool TrackEditor::render()
{
  int updated = 0;
  updated += renderLayerSelector(m_track);
  updated += ImGui::DragFloat("inner width", &m_profileTemplate.innerWidth, .05f, .1f, 10000.f);
  updated += ImGui::DragFloat("height", &m_profileTemplate.height, .05f, .1f, 10000.f);
  updated += ImGui::DragFloat("borders offset", &m_profileTemplate.bordersOffset, .05f, -10.f, 10.f);
  updated += ImGui::DragFloat("borders width", &m_profileTemplate.bordersWidth, .05f, .1f, 10000.f);
  updated += m_curveControls.render();
  updated += m_attractionPointsEditor.render();
  for (auto &at : m_attractionPoints)
  {
    Transform atT{};
    atT.position = at.position;
    atT.scale = { 1.4f,1.4f,1.4f };
    renderer::renderCube(atT.getWorldMatrix(), 0xff23d94e);
  }
  if (updated) updateTrack();
  return updated > 0;
}

void TrackEditor::initAttractionPointsEditor()
{
  m_attractionPointsEditor.title = "Attraction points";
  m_attractionPointsEditor.items = &m_attractionPoints;
  m_attractionPointsEditor.createItem = [](int slot) -> Track::AttractionPoint { return { vec3{ 0,0,0 }, 1.f }; };
  m_attractionPointsEditor.renderItem = [](Track::AttractionPoint &at) {
    Transform atT{};
    atT.position = at.position;
    atT.scale = { 1.5f,1.5f,1.5f };
    renderer::renderCube(atT.getWorldMatrix());
    return TransformControls::renderDrag3("position", &at.position)
      + ImGui::DragFloat("strength", &at.strength, .5f);
  };
}

void TrackEditor::updateTrack()
{
  const wchar_t *texturePath = getLayer() == Layer::BOOST_PLATE ? L"res/textures/boost.dds" : L"res/textures/track.dds";
  m_track = EditorScene::replaceObject(m_track,
    std::make_shared<Track>(m_curve, m_profileTemplate.buildProfile(), m_attractionPoints,
      EditorScene::getResources().loadEffect(L"res/shaders/miniphong.fx", BaseVertex::getShaderVertexLayout()),
      EditorScene::getResources().loadTexture(texturePath)));
}

TerrainEditor::TerrainEditor(const std::string& name, std::string heightmapFile)
  : WorldObjectEditor(name)
  , m_heightmapFile(std::move(heightmapFile))
{
  m_settings.worldHeight = 300.f;
  m_settings.uvScale     = 1.f;
  m_settings.worldWidth  = 200.f;
  m_settings.worldZScale = 20.f;
  m_terrain = EditorScene::replaceObject(m_terrain, std::make_shared<Terrain>(m_heightmapFile.c_str(), EditorScene::getResources(), m_settings));
}

TerrainEditor::TerrainEditor(
  const std::string& name,
  const std::shared_ptr<Terrain>& terrain,
  TerrainSettings terrainSettings,
  const std::string& heightmapFilePath)
  : WorldObjectEditor(name)
  , m_terrain(terrain)
  , m_settings(terrainSettings)
  , m_heightmapFile(heightmapFilePath)
  , m_transformControls{}
{
    m_transformControls.loadRotation(terrain->getTransform().rotation);
}

TerrainEditor::~TerrainEditor()
{
  EditorScene::replaceObject(m_terrain);
}

bool TerrainEditor::render()
{
  int updates = 0;
  renderLayerSelector(m_terrain);
  ImGui::Separator();
  ImGui::DragFloat2("width/height", &m_settings.worldWidth, .5f, 1.f, 10000.f);
  ImGui::DragFloat("zscale", &m_settings.worldZScale, .5f, 1.f, 10000.f);
  ImGui::DragFloat("uvscale", &m_settings.uvScale, .1f, .01f, 10000.f);
  if(ImGui::Button("update heightmap")) {
    m_terrain = EditorScene::replaceObject(m_terrain, std::make_shared<Terrain>(m_heightmapFile.c_str(), EditorScene::getResources(), m_settings));
    updates++;
  }
  if(m_transformControls.render(m_terrain->getTransform())) {
    m_terrain->updateTransform();
    updates++;
  }
  return updates > 0;
}

WorldPropEditor::WorldPropEditor(const std::string& name, const std::string& modelFilePath, const std::string& shaderFilePath)
  : WorldObjectEditor(name)
  , m_modelFilePath(modelFilePath)
  , m_effectFilePath(shaderFilePath)
  , m_effectEditionBuffer{}
{
  strcpy_s(m_effectEditionBuffer, shaderFilePath.c_str());
  m_worldProp = EditorScene::replaceObject(m_worldProp,
      std::shared_ptr{ WorldProp::makePhysicsfullObjectFromFile(EditorScene::getResources(), utils::string2widestring(modelFilePath), utils::string2widestring(shaderFilePath))});
}

WorldPropEditor::WorldPropEditor(const std::string& name, const std::shared_ptr<WorldProp> &worldProp, std::string modelFilePath, std::string shaderFilePath)
  : WorldObjectEditor(name)
  , m_worldProp(worldProp)
  , m_modelFilePath(std::move(modelFilePath))
  , m_effectFilePath(std::move(shaderFilePath))
  , m_effectEditionBuffer{}
{
  strcpy_s(m_effectEditionBuffer, m_effectFilePath.c_str());
  m_transformControls.loadRotation(worldProp->getTransform().rotation);
}

WorldPropEditor::~WorldPropEditor()
{
  EditorScene::replaceObject(m_worldProp);
}

bool WorldPropEditor::render()
{
  if (ImGui::InputText("shader path", m_effectEditionBuffer, std::size(m_effectEditionBuffer)))
      m_effectFilePath = m_effectEditionBuffer;
  else
      strcpy_s(m_effectEditionBuffer, m_effectFilePath.c_str());
  renderLayerSelector(m_worldProp);
  return WorldObjectEditor::render() + m_transformControls.render(m_worldProp->getTransform());
}

TriggerBoxEditor::TriggerBoxEditor(const std::string &name, vec3 position)
  : WorldObjectEditor(name)
  , m_transformControls(true, true, true)
{
  setLayer(Layer::TRIGGER);
  m_triggerBox = EditorScene::replaceObject(m_triggerBox, std::make_shared<TriggerBox>(Transform{ position }));
  m_triggerBox->setLayer(Layer::TRIGGER);
  replaceTriggetCallback();
}

TriggerBoxEditor::TriggerBoxEditor(const std::string &name, const std::shared_ptr<TriggerBox> &triggerBox)
  : WorldObjectEditor(name)
  , m_triggerBox(triggerBox)
{
  setLayer(triggerBox->getLayer());
  m_transformControls.loadRotation(triggerBox->getTransform().rotation);
  replaceTriggetCallback();
}

TriggerBoxEditor::~TriggerBoxEditor()
{
  EditorScene::replaceObject(m_triggerBox);
}

bool TriggerBoxEditor::render()
{
  WorldObjectEditor::render();
  WorldObjectEditor::renderLayerSelector(m_triggerBox);
  ImGui::Separator();
  Transform tRend = Transform{ m_triggerBox->getTransform() };
  tRend.scale /= 2.f;
  renderer::renderRect(tRend.getWorldMatrix(), 0xaa00ffff);
  return m_transformControls.render(m_triggerBox->getTransform());
}

void TriggerBoxEditor::replaceTriggetCallback() const
{
  m_triggerBox->setCallback([this] {
    logs::scene.logm("Hit ", getDisplayName());
  });
}

BillboardsEditor::BillboardsEditor(const std::string &name)
  : WorldObjectEditor(name)
{
  m_object = EditorScene::replaceObject(m_object, std::make_shared<BillboardsObject>(EditorScene::getResources()));
  initEditor();
}

BillboardsEditor::BillboardsEditor(const std::string &name, const std::shared_ptr<BillboardsObject> &object, std::vector<std::string> texturePaths)
  : WorldObjectEditor(name)
  , m_object(object)
  , m_billboardTextures(std::move(texturePaths))
{
  initEditor();
}

BillboardsEditor::~BillboardsEditor()
{
  EditorScene::replaceObject(m_object);
}

bool BillboardsEditor::render()
{
  return m_billboardListEditor.render();
}

void BillboardsEditor::initEditor()
{
  m_billboardListEditor.items = &m_object->getBillboards();
  m_billboardListEditor.createItem = [this](int slot) -> std::optional<BillBoard> {
    std::string texturePath = EditorScene::getFilepathInputText();
    try {
      Texture texture = EditorScene::getResources().loadTexture(utils::string2widestring(texturePath));
      m_billboardTextures.insert(m_billboardTextures.begin() + slot, texturePath);
      return BillBoard{ texture, EditorScene::getCameraTransform().position, {1,1} };
    } catch (const std::runtime_error &) {
      return std::nullopt; // could not load the texture
    }
  };
  m_billboardListEditor.deleteItem = [this](int slot, BillBoard &) {
    m_billboardTextures.erase(m_billboardTextures.begin() + slot);
  };
  m_billboardListEditor.renderItem = [](BillBoard &billboard) {
    int updates = 0;
    updates += TransformControls::renderDrag3("position", &billboard.position);
    updates += ImGui::DragFloat2("size", &billboard.scale.x, .05f, .05f, 1000.f);
    updates += ImGui::DragFloat4("UVs", &billboard.texX, .01f, 0, 1);
    return updates > 0;
  };
}

CameraRailEditor::CameraRailEditor(const std::string &name)
  : WorldObjectEditor(name)
{
  m_pointsEditor.items = &m_points;
  m_pointsEditor.createItem = [](auto) { return EditorScene::getCameraTransform().position; };
  m_pointsEditor.renderItem = [](vec3 &p) {
    Transform cube{};
    cube.position = p;
    cube.scale = { .4f,.4f,.4f };
    renderer::renderCube(cube.getWorldMatrix(), 0xffc0d63c);
    return TransformControls::renderDrag3("point position", &p);
  };
}

bool CameraRailEditor::render()
{
  for(size_t i = 0; i+1 < m_points.size(); i++)
    renderer::renderLine(m_points[i], m_points[i+1], 0xffc0d63c);
  
  return m_pointsEditor.render() + WorldObjectEditor::render();
}

CameraRail CameraRailEditor::createRail() const
{
  return { m_points };
}

TunnelEditor::TunnelEditor(const std::string &name)
  : WorldObjectEditor(name)
{
  m_tunnel = EditorScene::replaceObject(m_tunnel, std::make_shared<Tunnel>(EditorScene::getResources()));
}

TunnelEditor::TunnelEditor(const std::string& name, std::shared_ptr<Tunnel> tunnel): WorldObjectEditor(name)
  , m_tunnel(std::move(tunnel))
{
    m_transformControls.loadRotation(m_tunnel->getTransform().rotation);
}

TunnelEditor::~TunnelEditor()
{
  EditorScene::replaceObject(m_tunnel);
}

bool TunnelEditor::render()
{
  return m_transformControls.render(m_tunnel->getTransform());
}
