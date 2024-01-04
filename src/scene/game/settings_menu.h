#pragma once

#include "scene/scene.h"

#include "display/graphical_resource.h"
#include "display/ui/ui_manager.h"
#include "engine/device.h"
#include "display/ui/ui_elements.h"
#include <vector>

class SettingsMenuScene : public pbl::Scene
{
public:
	SettingsMenuScene();

	void update(double delta) override;
	void render() override;

private:
	void rebuildUI();

private:
	std::shared_ptr<pbl::ScreenResizeEventHandler> m_windowResizeEventHandler;
	pbl::GraphicalResourceRegistry m_resources;
	pbl::UIManager m_ui;
	std::vector<std::shared_ptr<pbl::CheckboxElement>> m_checkboxes;
	int m_selectedBoxIndex{ -1 };
};
