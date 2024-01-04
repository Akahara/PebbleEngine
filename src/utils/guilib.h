#pragma once

// Macro definitions
#ifdef _DEBUG
#define PBL_HASGUI
#endif


#ifndef PBL_GUILIBFROMIMCONFIG

// Code to extend/mock ImGui
#ifdef PBL_HASGUI
#include "imgui/imgui.h"

#define PBL_IMGUI_PROFILE(name, instructions) {\
  auto __start = std::chrono::system_clock::now();\
  instructions;\
  auto __elapsed = std::chrono::system_clock::now() - __start;\
  ImGui::BeginDisabled();\
    static float __delta;\
    __delta = std::chrono::duration_cast<std::chrono::duration<float>>(__elapsed).count()*1000.f;\
    ImGui::DragFloat(name, &__delta);\
  ImGui::EndDisabled();\
}

#else
#define PBL_IMGUI_PROFILE(name, instructions) { instructions; }

/*
* In release mode, the ImGui namespace may be used but all functions will be
* noops. The arguments will still be evaluated so large computations that are
* not necessary should be discarded altogether using #if(n)def PBL_HASGUI
*/
namespace ImGui
{
#define MOCK_IMGUI_FUNC(name) inline void name(auto &&...Args) {}
#define MOCK_IMGUI_FUNCRET(name, retVal) inline auto name(auto &&...Args) { return retVal; }
#define MOCK_IMGUI_VAR(name) inline constexpr auto name = 0;

struct ImVec4 { float x, y, z, w; };

MOCK_IMGUI_FUNC(Begin)
MOCK_IMGUI_FUNCRET(DragFloat, false)
MOCK_IMGUI_FUNCRET(DragFloat2, false)
MOCK_IMGUI_FUNCRET(DragFloat3, false)
MOCK_IMGUI_FUNCRET(DragFloat4, false)
MOCK_IMGUI_FUNCRET(SliderFloat, false)
MOCK_IMGUI_FUNCRET(Checkbox, false)
MOCK_IMGUI_FUNCRET(CollapsingHeader, false)
MOCK_IMGUI_FUNCRET(SliderInt, false)
MOCK_IMGUI_FUNCRET(Button, false)
MOCK_IMGUI_FUNCRET(DragFloatRange2, false)
MOCK_IMGUI_FUNCRET(InputText, false)
MOCK_IMGUI_FUNCRET(Combo, false)
MOCK_IMGUI_FUNC(TextUnformatted)
MOCK_IMGUI_FUNC(Text)
MOCK_IMGUI_FUNC(BeginDisabled)
MOCK_IMGUI_FUNC(EndDisabled)
MOCK_IMGUI_FUNC(PushID)
MOCK_IMGUI_FUNC(PopID)
inline void PushStyleColor(auto, ImVec4) {}
MOCK_IMGUI_FUNC(PopStyleColor)
MOCK_IMGUI_FUNC(SetTooltip)
MOCK_IMGUI_FUNCRET(IsItemHovered, false)
MOCK_IMGUI_FUNC(SameLine)
MOCK_IMGUI_FUNC(Separator)
MOCK_IMGUI_FUNC(End)

}
MOCK_IMGUI_VAR(ImGuiCol_Button)
MOCK_IMGUI_VAR(ImGuiCol_Text)

#endif // _PBL_GUILIBFROMIMCONFIG

#endif // PBL_HASGUI