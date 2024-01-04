#include "device.h"

#include "utils/regions.h"

namespace pbl
{
  
uint32_t Device::s_winWidth{};
uint32_t Device::s_winHeight{};
std::vector<std::weak_ptr<ScreenResizeEventHandler>> Device::s_screenResizeEventHandler;

void Device::addWindowResizeEventHandler(std::weak_ptr<ScreenResizeEventHandler> resizeHandler)
{
  s_screenResizeEventHandler.push_back(std::move(resizeHandler));
}

void Device::performWindowResize(int newWidth, int newHeight)
{
  resizeWindow(newWidth, newHeight);

  s_winWidth = newWidth;
  s_winHeight = newHeight;

  ScreenRegion::SCREEN_WIDTH = ScreenRegion::SCREEN_HEIGHT / newHeight * newWidth;

  for(size_t i = s_screenResizeEventHandler.size(); i > 0; i--) {
    auto handler = s_screenResizeEventHandler[i-1];
    if (auto lock = handler.lock())
      (*lock)(newWidth, newHeight);
    else
      s_screenResizeEventHandler.erase(s_screenResizeEventHandler.begin() + i++ - 1);
  }
}

}
