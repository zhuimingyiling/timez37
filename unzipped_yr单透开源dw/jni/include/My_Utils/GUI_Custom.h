#ifndef IMGUI_CUSTOMIZE_H
#define IMGUI_CUSTOMIZE_H
#include <fstream>
#include "imgui.h"
#include "imgui_internal.h"


namespace ImGui {
    //自定义::(控件)小组件
    IMGUI_API void CardPanel(const char* title, const ImVec2& size, const ImVec4& bgColor, float cornerRadius,float padding, const std::function<void()>& contentFunction);
    IMGUI_API void CardPanel2(const char* title, const ImVec2& size, const ImVec4& bgColor, float cornerRadius,float padding, const std::function<void()>& contentFunction);
    IMGUI_API void RenderCustomSlider(const char* label,float* value, float sliderMinValue, float sliderMaxValue, float sliderBarWidth);
    IMGUI_API void XSRenderCustomSlider(const char* label,float* value, float sliderMinValue, float sliderMaxValue, float sliderBarWidth);
    IMGUI_API void Separator2(const char* text, ImVec2 size);
}

#endif //IMGUI_CUSTOMIZE_H
