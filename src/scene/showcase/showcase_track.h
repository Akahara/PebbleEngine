#pragma once

#include <memory>
#include <algorithm>

#include "showcases.h"
#include "inputs/user_inputs.h"
#include "scene/game/player.h"
#include "scene/game/player_vehicle.h"
#include "scene/game/track.h"
#include "world/trigger_box.h"

class ShowcaseTrack : public ShowcaseScene {
public:
  ShowcaseTrack()
    : m_gameLogic{ &m_ui }
  {
    m_gameLogic.setPlayer(&m_player);

    auto vehicle = std::make_shared<PlayerVehicle>(m_graphicalResources);
    m_player.setVehicleObject(vehicle.get());
    m_physics.addBody(static_cast<std::shared_ptr<WorldObject>>(vehicle)->buildPhysicsObject());
    m_objects.push_back(std::move(vehicle));

    auto triggerTest = std::make_unique<TriggerBox>([this] {
      logs::physics.log("Checkpoint !");
      m_gameLogic.validateCheckpoint(1, Transform{ { 3*r,2,-r }});
    }, Transform{ { 0,0,5*r }, { 10,10,10 } });
    m_physics.addBody(triggerTest->buildPhysicsObject());
    m_objects.push_back(std::move(triggerTest));

    auto killerTest = std::make_unique<TriggerBox>([this] {
      logs::physics.log("You're out !"); m_gameLogic.setPlayerToCheckpoint();
    }, Transform{ { 0,-20,0 }, { 1000,10,1000 } });
    m_physics.addBody(killerTest->buildPhysicsObject());
    m_objects.push_back(std::move(killerTest));

    m_physics.addEventHandler(&m_gameLogic);

    for(int i = 0; i < t; i++) {
      float l = (i*.5f+2)*2*r;
      float x0 = i*3*r;
      BezierCurve bezier;
      bezier.isLoop = true;
      vec3 p0{ x0+0, 0,   -r  }, n0{ -k,0,0 }; bezier.controlPoints.emplace_back(p0, p0+n0,   p0-n0  );
      vec3 p1{ x0+r, 0,   0   }, n1{ 0,0,-k }; bezier.controlPoints.emplace_back(p1, p1+n1,   p1-n1*3);
      vec3 p2{ x0+r, h*i, l   }, n2{ 0,0,-k }; bezier.controlPoints.emplace_back(p2, p2+n2*3, p2-n2  );
      vec3 p3{ x0+0, h*i, l+r }, n3{ +k,0,0 }; bezier.controlPoints.emplace_back(p3, p3+n3,   p3-n3  );
      vec3 p4{ x0-r, h*i, l   }, n4{ 0,0,+k }; bezier.controlPoints.emplace_back(p4, p4+n4,   p4-n4*3);
      vec3 p5{ x0-r, 0,   0   }, n5{ 0,0,+k }; bezier.controlPoints.emplace_back(p5, p5+n5*3, p5-n5  );
      vec3 p6{ x0+0, 0,   -r  }, n6{ -k,0,0 }; bezier.controlPoints.emplace_back(p6, p6+n6,   p6-n6  );
      std::vector<Track::AttractionPoint> attractionPoints;
      attractionPoints.push_back({ vec3{ x0, h*i+r, l }, 10000.f });
      auto track = std::make_shared<Track>(bezier, Track::TrackProfileTemplate{}.buildProfile(), attractionPoints, m_graphicalResources);
      track->setLayer(Layer::STICKY_ROAD);
      m_physics.addBody(track->buildPhysicsObject());
      m_objects.push_back(std::move(track));
    }

    m_player.resetPosition({ 0,3,-r });
    m_gameLogic.setCheckpointPosition(Transform{ { 0,2,-r} });
    m_gameLogic.setLastCheckpointId(1);
    m_gameLogic.setNumberOfTurns(1);

  }

  void render() override
  {
    ShowcaseScene::render(m_player.getActiveCamera());

    m_player.render();

    logs::scene.beginWindow();
    for(int i = 0; i < t; i++) {
      if (ImGui::Button(("TP" + std::to_string(i)).c_str()))
        m_player.resetPosition({ i*3*r, 5, -r });
      if(i != t-1)
        ImGui::SameLine();
    }

    ImGui::End();
  }

  void update(double delta) override
  {
    ShowcaseScene::update(delta);

    m_player.update(delta);
  }

private:
  int   TRACK_COUNT = 5, t = TRACK_COUNT;
  float TRACK_RADIUS = 50, r = TRACK_RADIUS;
  float TRACK_HANDLES_RADIUS = r*.5f, k = TRACK_HANDLES_RADIUS;
  float TRACK_HEIGHT = 50, h = TRACK_HEIGHT;
  Player    m_player;
  GameLogic m_gameLogic;
};
