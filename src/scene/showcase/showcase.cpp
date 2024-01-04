#include "showcases.h"

#include "showcase_empty.h"
#include "showcase_quadtree.h"
#include "showcase_track.h"
#include "scene/editor/editor_scene.h"
#include "scene/game/game_scene.h"
#include "scene/game/main_menu.h"

void renderScenesMenu()
{
  logs::scene.beginWindow();
  if (ImGui::Button("Scene: Game    ")) SceneManager::getInstance().transitionToScene(SceneManager::make_supplier<GameScene>());
  ImGui::SameLine();
  if (ImGui::Button("Scene: Empty   ")) SceneManager::getInstance().transitionToScene(SceneManager::make_supplier<ShowcaseEmpty>());
  ImGui::SameLine();
  if (ImGui::Button("Scene: QuadTree")) SceneManager::getInstance().transitionToScene(SceneManager::make_supplier<ShowcaseQuadTree>());
  if (ImGui::Button("Scene: Track   ")) SceneManager::getInstance().transitionToScene(SceneManager::make_supplier<ShowcaseTrack>());
  ImGui::SameLine();
  if (ImGui::Button("Scene: Editor  ")) SceneManager::getInstance().transitionToScene(SceneManager::make_supplier<EditorScene>());
  ImGui::SameLine();
  if (ImGui::Button("Scene: MainMenu")) SceneManager::getInstance().transitionToScene(SceneManager::make_supplier<MainMenuScene>());
  ImGui::End();
}
