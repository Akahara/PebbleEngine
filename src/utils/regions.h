#pragma once

#include <algorithm>
#include <cstdint>

namespace pbl
{

using anchor_t = uint8_t;

namespace Anchor
{
static constexpr anchor_t
LEFT    = 1<<0,
HMIDDLE = 1<<1, // horizontally centered
RIGHT   = 1<<2,
TOP     = 1<<3,
VMIDDLE = 1<<4, // vertically centered
BOTTOM  = 1<<5,
MASK_HORIZONTAL = LEFT|HMIDDLE|RIGHT,
MASK_VERTICAL   = TOP|VMIDDLE|BOTTOM,

// shorthands
TOP_LEFT     = TOP|LEFT,
TOP_RIGHT    = TOP|RIGHT,
BOTTOM_LEFT  = BOTTOM|LEFT,
BOTTOM_RIGHT = BOTTOM|RIGHT,
CENTER       = HMIDDLE|VMIDDLE;
};

struct ScreenPoint {
  float x = 0, y = 0;
};

struct ScreenRegion {
  static constexpr float SCREEN_HEIGHT = 1.f;
  static float SCREEN_WIDTH;

  float minX{}, minY{}, maxX{}, maxY{}; // in range 0..SCREEN_WIDTH/HEIGHT with 0:0 being the bottom left corner of the screen

  float width() const { return maxX - minX; }
  float height() const { return maxY - minY; }
  float ymiddle() const { return (minY+maxY)*.5f; }
  float xmiddle() const { return (minX+maxX)*.5f; }

  bool contains(const ScreenPoint &p) const
  {
	return minX <= p.x && maxX >= p.x && minY <= p.y && maxY >= p.y;
  }

  float xInRegion(float relativeX) const
  {
	return minX + relativeX * width();
  }

  float yInRegion(float relativeY) const
  {
	return minY + relativeY * height();
  }

  ScreenRegion regionInRegion(ScreenRegion content) const
  {
	ScreenRegion sr;
	sr.minX = minX + content.minX * width();
	sr.maxX = minX + content.maxX * width();
	sr.minY = minY + content.minY * height();
	sr.maxY = minY + content.maxY * height();
	return sr;
  }

  static ScreenRegion unionRegion(const ScreenRegion &r1, const ScreenRegion &r2)
  {
	return { std::min(r1.minX, r2.minX), std::min(r1.minY, r2.minY), std::max(r1.maxX, r2.maxX), std::max(r1.maxY, r2.maxY) };
  }

  static ScreenRegion fullscreen()
  {
	return { 0,0,SCREEN_WIDTH,SCREEN_HEIGHT };
  }

  static float xInRegion(ScreenRegion region, float relativeX)
  {
	return region.minX + relativeX * region.width();
  }

  static float yInRegion(ScreenRegion region, float relativeY)
  {
	return region.minY + relativeY * region.height();
  }

  static ScreenRegion moveRegionCorner(ScreenRegion region, float cornerX, float cornerY, anchor_t cornerLocation)
  {
	return moveRegionCorner(region.width(), region.height(), cornerX, cornerY, cornerLocation);
  }

  static ScreenRegion moveRegionCorner(float regionWidth, float regionHeight, float cornerX, float cornerY, anchor_t cornerLocation)
  {
	ScreenRegion region;
	switch (cornerLocation & Anchor::MASK_HORIZONTAL) {
	case Anchor::LEFT:    region.minX = cornerX; break;
	case Anchor::HMIDDLE: region.minX = cornerX - regionWidth/2.f; break;
	case Anchor::RIGHT:   region.minX = cornerX - regionWidth; break;
	default: break;
	}
	switch (cornerLocation & Anchor::MASK_VERTICAL) {
	case Anchor::TOP:     region.minY = cornerY - regionHeight; break;
	case Anchor::VMIDDLE: region.minY = cornerY - regionHeight/2.f; break;
	case Anchor::BOTTOM:  region.minY = cornerY; break;
	default: break;
	}
	region.maxX = region.minX + regionWidth;
	region.maxY = region.minY + regionHeight;
	return region;
  }

};

}
