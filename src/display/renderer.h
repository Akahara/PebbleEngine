#pragma once

#include "camera.h"
#include "utils/math.h"
#include "world/object.h"

struct DiscreteCurve;
struct BezierCurve;
class AABB;

/**
 * General purpose renderer, most of the rendering goes through specific render methods instead.
 */
namespace pbl::renderer
{

unsigned int colorFromVector(const vec4 &v);
rvec4 float4FromColor(unsigned int color);

void loadGlobalResources(GraphicalResourceRegistry &resources);
void unloadGlobalResources();

// a set of handy debug render methods, don't forget to call sendDebugDraws() if you use one of them!
void renderLine(rvec3 p0, rvec3 p1, unsigned int color=0xff0000ff);
void renderLine(vec3 p0, vec3 p1, unsigned int color=0xff0000ff);
void renderAABB(const AABB &aabb, unsigned int color=0xff00ffff);
void renderCube(mat4 transformMat, unsigned int color=0xaa00ffff);
void renderRect(mat4 transformMat, unsigned int color=0xaa00ffff);
void renderCurve(const BezierCurve &bezier, unsigned int color=0xffff00ff);
void renderCurve(const DiscreteCurve &curve, unsigned int color=0xff00ffff);
void renderAxes(const Transform& transform);
void renderCameraOutline(const Camera &camera);
void renderWorldAxes(const Camera& camera);
// *must* be called when one of the above renderXX() is used
void sendDebugDraws(const Camera &camera);

}
