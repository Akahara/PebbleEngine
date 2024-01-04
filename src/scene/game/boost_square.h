#pragma once

#include "world/trigger_box.h"
#include "display/renderable.h"
#include "display/render_profiles.h"

namespace pbl
{

class BoostSquare : public pbl::WorldProp
{
public:
	explicit BoostSquare(Transform transform, pbl::GraphicalResourceRegistry &resources);

	void render(RenderContext &context) override {};
	void renderTransparent(RenderContext &context) override;

};

};