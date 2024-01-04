#include "tunnel.h"

#include <random>

#include "physics/physxlib.h"
#include "display/graphical_resource.h"
#include "display/renderer.h"

Tunnel::Tunnel(pbl::GraphicalResourceRegistry &resources)
  : m_lightsCBO(sizeof(LightsConstantBuffer), pbl::GenericBuffer::BUFFER_CONSTANT)
  , m_physicsBody(this)
{
  { // tunnel outer wall
    m_meshes.emplace_back(Transform{}, resources.loadMesh(L"res/models/basic_tunnel.obj"));
    physx::PxTriangleMeshGeometry meshGeometry{ pbl::WorldProp::makePhysicsMeshFromModel(*resources.loadModel(L"res/models/basic_tunnel.obj")) };
    physx::PxMaterial *material = pbx::Physics::getSdk().createMaterial(1, 1, .2f);
    m_physicsBody.addActor(PxCreateStatic(pbx::Physics::getSdk(), pbx::scene2physicsTransform(m_transform), meshGeometry, *material));
  }

  { // tunnel obstacles
    std::shared_ptr<pbl::Mesh> obstacleMesh = resources.loadMesh(L"res/models/tunnel_obstacle.obj");
    physx::PxTriangleMeshGeometry meshGeometry{ pbl::WorldProp::makePhysicsMeshFromModel(*resources.loadModel(L"res/models/tunnel_obstacle.obj")) };
    physx::PxMaterial *material = pbx::Physics::getSdk().createMaterial(1, 1, .2f);

    auto random = mathf::randomFunction();

    auto addObstacle = [&](float angle, float z) {
      Transform transform;
      transform.position = { std::cos(angle)*15.f,std::sin(angle)*10.f,z};
      transform.scale = { 1,1,1 };
      transform.rotation = XMQuaternionRotationAxis(vec3{0,0,1}, angle - PI*.5f);
      transform = m_transform.transform(transform);
      m_meshes.emplace_back(transform, obstacleMesh);
      m_physicsBody.addActor(PxCreateStatic(pbx::Physics::getSdk(), pbx::scene2physicsTransform(transform), meshGeometry, *material));
    };

    // values here were currated with a specific tunnel model in mind
    float prevAngle = 0;
    for(float z = 25.f; z < 660; z += 35.f) {
      float obstacleAngle;
      constexpr float HPI = PI*.5f;
      do {
        obstacleAngle = mathf::lerp(-PI*.4f, PI*.4f, random()) - HPI;
      } while (std::abs(obstacleAngle-prevAngle) < PI*.15f);
      prevAngle = obstacleAngle;
      addObstacle(obstacleAngle, z);
      if (std::abs(obstacleAngle + HPI) > PI*.4f && random() < .4f)
        addObstacle(2*-HPI - obstacleAngle, z);
    }
  }

  pbl::Effect *shader = resources.loadEffect(L"res/shaders/tunnel_miniphong.fx", pbl::BaseVertex::getShaderVertexLayout());
  std::ranges::for_each(m_meshes, [&](auto &m) { std::ranges::for_each(m.second->getSubmeshes(), [&](auto &submesh) { submesh.effect = shader; }); });
}

pbx::PhysicsBody *Tunnel::buildPhysicsObject()
{
  m_spotLights.clear();
  vec3 pos{ 0,14,9 };
  vec3 off{ 0,0,35 };
  vec3 dir{ 0,-1,.7f };
  rvec3 lightDir; XMStoreFloat3(&lightDir, XMVector3Normalize(m_transform.transform(dir) - m_transform.position));
  for(int i = 0; i < 20; i++) {
    rvec3 lightPos; XMStoreFloat3(&lightPos, m_transform.transform(pos + off * static_cast<float>(i)));
    Light spotlight{};
    spotlight.lightType = Light::TYPE_SPOTLIGHT;
    spotlight.sDirection = lightDir;
    spotlight.sFalloffBegin = 50.f;
    spotlight.sFalloffEnd = 55.f;
    spotlight.sInnerAngle = .6f;
    spotlight.sOuterAngle = .8f;
    spotlight.aPosition = lightPos;
    spotlight.aColor = { 1,1,1 };
    m_spotLights.emplace_back(spotlight);
  }

  for(size_t i = 0; i < m_meshes.size(); i++) {
    m_physicsBody.getActors()[i]->setGlobalPose(pbx::scene2physicsTransform(m_transform.transform(m_meshes[i].first)));
  }
  return &m_physicsBody;
}

void Tunnel::render(pbl::RenderContext &context)
{
  size_t activeSpotLights = std::min(MAXIMUM_LIGHTS_COUNT-1, m_spotLights.size());
  size_t activeLights = std::min(MAXIMUM_LIGHTS_COUNT, activeSpotLights+1);
  std::ranges::partial_sort(m_spotLights, m_spotLights.begin() + activeSpotLights, std::less{}, [&](const Light &light)
    { return XMVectorGetX(XMVector3Length(XMLoadFloat3(&light.aPosition) - context.focusPosition)); });

  LightsConstantBuffer lightsCB{};
  Light &pointLight = lightsCB.lights[0];
  lightsCB.activeLights = static_cast<uint32_t>(activeLights);
  pointLight.aColor = { 0.176f, 0.239f, 0.545f };
  pointLight.pStrength = 10.f;
  pointLight.lightType = Light::TYPE_POINTLIGHT;
  XMStoreFloat3(&pointLight.aPosition, context.focusPosition);
  std::copy_n(m_spotLights.begin(), activeSpotLights, std::begin(lightsCB.lights)+1);
  m_lightsCBO.setData(lightsCB);

  context.constantBufferBindings.push_back({ "cbObject", s_objectConstantBuffer.get() });
  context.constantBufferBindings.push_back({ "cbLights", &m_lightsCBO });
  for(auto &[transform, mesh] : m_meshes) {
    pbl::ObjectConstantData objectCBData{};
    objectCBData.matWorld = XMMatrixTranspose(m_transform.transform(transform).getWorldMatrix());
    s_objectConstantBuffer->setData(objectCBData);
    mesh->draw(context);
  }
  context.constantBufferBindings.pop_back();
  context.constantBufferBindings.pop_back();
}
