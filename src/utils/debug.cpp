#include "debug.h"

#ifdef PBL_HASGUI

LogsWindow::LogsWindow(const char* name): m_name(name)
{
  getAllWindows().push_back(this);
}

LogsWindow::~LogsWindow()
{
  auto &windows = getAllWindows();
  auto thisRef = std::ranges::find(windows, this);
  if(thisRef != windows.end()) windows.erase(thisRef);
}

void LogsWindow::beginWindow()
{
  ImGui::Begin(m_name);
}

void LogsWindow::renderWindows()
{
  std::for_each(getAllWindows().begin(), getAllWindows().end(), [](auto w) { w->render(); });
}

void LogsWindow::render()
{
  if (m_lines.empty()) return;

  ImGui::Begin(m_name);
  ImGui::Checkbox("Autoscroll", &m_autoscroll);

  constexpr ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollX | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersV;

  ImVec2 outer_size = ImVec2(0.0f, 0.f);
  if (ImGui::BeginTable(m_name, 1, flags, outer_size)) {
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn(m_name, ImGuiTableColumnFlags_NoHeaderLabel);
    ImGui::TableHeadersRow();

    for (std::string &line : m_lines) {
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::TextUnformatted(line.c_str());
    }

    if (m_autoscroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
      ImGui::SetScrollHereY(1.0f);
    ImGui::EndTable();
  }
  ImGui::End();
}

std::vector<LogsWindow*>& LogsWindow::getAllWindows()
{
  static std::vector<LogsWindow*> windows;
  return windows;
}

#endif

LogsWindow logs::general { "general"  };
LogsWindow logs::graphics{ "graphics" };
LogsWindow logs::scene   { "scene"    };
LogsWindow logs::physics { "physics"  };
LogsWindow logs::inputs  { "inputs"   };
