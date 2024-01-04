#pragma once

#include <chrono>
#include <vector>
#include <string>
#include <sstream>
#include <queue>
#include <stdexcept>

#include "guilib.h"

#ifdef _DEBUG
#define PBL_ISDEBUG
#define PBL_DEBUG(x) x
#define PBL_ASSERT(x, message) if(!(x)) throw std::runtime_error(std::string("Broken assertion: ") + (message))
#else
#define PBL_DEBUG(x)
#define PBL_ASSERT(x, message)
#endif

#ifdef PBL_HASGUI
class LogsWindow
{
public:
  LogsWindow(const char *name);
  ~LogsWindow();
  LogsWindow(const LogsWindow &) = delete;
  LogsWindow &operator=(const LogsWindow &) = delete;
  LogsWindow(const LogsWindow &&) = delete;
  LogsWindow &operator=(const LogsWindow &&) = delete;

  void beginWindow();

  void log(const std::string &line) {
    m_lines.push_back(line);
    if (m_lines.size() > 200) m_lines.resize(100);
  }

  void logm(auto&&... args)
  {
    std::stringstream ss;
    (ss << ... << args);
    log(ss.str());
  }

public:
  static void renderWindows();

private:
  void render();
  static std::vector<LogsWindow *> &getAllWindows();

protected:
  std::vector<std::string> m_lines;
  const char              *m_name;
  bool                     m_autoscroll = true;
};
#else
class LogsWindow
{
public:
  LogsWindow(auto &&args...) {}
  void render() {}
  void log(const std::string &line) {}
  void logm(auto &&args...) {}
  void beginWindow() {}
  static void renderWindows() {}
};
#endif

namespace logs {
extern LogsWindow general;
extern LogsWindow graphics;
extern LogsWindow scene;
extern LogsWindow physics; // there is a specialized physics window inside physics/structures.h, this one serves little
extern LogsWindow inputs;
}
