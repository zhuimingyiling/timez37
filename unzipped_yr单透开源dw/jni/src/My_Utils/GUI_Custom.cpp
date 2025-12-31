#include "GUI_Custom.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <malloc.h>
#include <fstream>
#include <iostream>
#include <ctime>
#include <ctgmath>
#include <imgui_internal.h>
#include <stb_image.h>
#include <unordered_map>
#include <vector>
#include <thread>

void ImGui::CardPanel(const char* title, const ImVec2& size, const ImVec4& bgColor, float cornerRadius,float padding, const std::function<void()>& contentFunction) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, bgColor);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, cornerRadius);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.2f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding, padding));
    ImGui::BeginChild(title, size, true);
    if (contentFunction) {
        contentFunction();
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

void ImGui::CardPanel2(const char* title, const ImVec2& size, const ImVec4& bgColor, float cornerRadius,float padding, const std::function<void()>& contentFunction) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, bgColor);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, cornerRadius);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.1f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding, padding));
    ImGui::BeginChild(title, size, true, ImGuiWindowFlags_NoScrollbar);
    if (contentFunction) {
        contentFunction();
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

void ImGui::Separator2(const char* text, ImVec2 size) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return;
    const ImGuiStyle& style = ImGui::GetStyle();
    const ImGuiID id = window->GetID(text);
    const ImVec2 contentMin = window->DC.CursorPos;
    const ImVec2 contentMax = ImVec2(contentMin.x + size.x, contentMin.y + size.y);
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImU32 shadowColor2 = IM_COL32(0, 0, 0, 128);
    float shadowStartY = contentMin.y + size.y - 25;
    float shadowEndY = shadowStartY + 5;
    drawList->AddRectFilledMultiColor(
        ImVec2(contentMin.x, shadowStartY),
        ImVec2(contentMin.x+size.x, shadowEndY),
        shadowColor2, shadowColor2, shadowColor2, shadowColor2
    );
}

const float sliderBarHeight = 20.0f; // 滑块条的高度
const float sliderKnobSize = 30.0f; // 滑块的大小
const float sliderKnobOffset = 5.0f; // 滑块位置的偏移
float knobSize = 40.0f;  // 方形滑块的大小
float cornerRadius = 50.0f;  // 圆角的半径
#include <map>
struct RenderCustomSliderState {
    bool isDragging = false;
    float dragOffset = 0.0f;
    std::string label;
};
bool isShowText=true;
static std::map<std::string, RenderCustomSliderState> sliderStates;

void ImGui::RenderCustomSlider(const char* label, float* value, float min, float max, float width) {
    // 滑块状态管理
    static std::map<std::string, RenderCustomSliderState> sliderStates;
    if (sliderStates.find(label) == sliderStates.end()) {
        sliderStates[label] = {false, 0.0f};
    }
    RenderCustomSliderState& state = sliderStates[label];

    // 滑块参数
    const float barHeight = 20.0f;
    const float knobRadius = 10.0f;
    const float textPadding = 15.0f;

    // 颜色定义
    const ImU32 backgroundColor = ImGui::GetColorU32(ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    const ImU32 foregroundColor = ImGui::GetColorU32(ImVec4(0.0f, 0.6f, 1.0f, 1.0f));
    const ImU32 textColor = ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    const ImU32 textBgColor = ImGui::GetColorU32(ImVec4(0.1f, 0.1f, 0.1f, 0.8f));

    // 滑块位置和尺寸
    ImVec2 startPos = ImGui::GetCursorScreenPos();
    ImVec2 endPos = ImVec2(startPos.x + width, startPos.y + barHeight);

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // 绘制滑块背景
    drawList->AddRectFilled(
        ImVec2(startPos.x, startPos.y + barHeight * 0.5f - 2.0f), 
        ImVec2(endPos.x, startPos.y + barHeight * 0.5f + 2.0f), 
        backgroundColor, 
        3.0f
    );

    // 计算滑块当前位置
    float normalizedValue = (*value - min) / (max - min);
    float knobCenterX = startPos.x + normalizedValue * width;

    // 绘制滑块填充
    drawList->AddRectFilled(
        ImVec2(startPos.x, startPos.y + barHeight * 0.5f - 2.0f), 
        ImVec2(knobCenterX, startPos.y + barHeight * 0.5f + 2.0f), 
        foregroundColor, 
        3.0f
    );

    // 绘制滑块拖动钮
    drawList->AddCircleFilled(
        ImVec2(knobCenterX, startPos.y + barHeight * 0.5f), 
        knobRadius, 
        foregroundColor
    );

    // 绘制滑块值文本
    char valueText[32];
    sprintf(valueText, "%.0f", *value);

    // 设置字体大小
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]); // 假设第二个字体是更大的字体
    ImVec2 textSize = ImGui::CalcTextSize(valueText);

    // 计算文本位置
    ImVec2 textPos = ImVec2(
        knobCenterX - textSize.x * 0.5f, 
        startPos.y - textSize.y - textPadding
    );

    // 绘制文本背景
    drawList->AddRectFilled(
        ImVec2(textPos.x - 5.0f, textPos.y - 5.0f), 
        ImVec2(textPos.x + textSize.x + 5.0f, textPos.y + textSize.y + 5.0f), 
        textBgColor, 
        5.0f
    );

    // 绘制文本
    drawList->AddText(textPos, textColor, valueText);
    ImGui::PopFont();

    // 滑块交互逻辑
    ImGui::InvisibleButton(label, ImVec2(width, barHeight));
    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
        ImVec2 mousePos = ImGui::GetMousePos();
        state.isDragging = true;
        state.dragOffset = mousePos.x - knobCenterX;
    }

    if (state.isDragging && ImGui::IsMouseDragging(0)) {
        ImVec2 mousePos = ImGui::GetMousePos();
        float dx = mousePos.x - startPos.x;
        float normalizedPos = ImClamp(dx / width, 0.0f, 1.0f);
        *value = min + normalizedPos * (max - min);
    }

    if (ImGui::IsMouseReleased(0)) {
        state.isDragging = false;
    }
}

void ImGui::XSRenderCustomSlider(const char* label, float* value, float sliderMinValue, float sliderMaxValue, float sliderBarWidth)
{
    // 滑块状态
    static std::map<std::string, RenderCustomSliderState> sliderStates;
    if (sliderStates.find(label) == sliderStates.end()) {
        sliderStates[label] = {false, 0.0f};
    }
    RenderCustomSliderState& sliderState = sliderStates[label];

    // 滑块参数
    const float sliderBarHeight = 10.0f; // 滑动条高度
    const float sliderKnobSize = 20.0f; // 滑块大小
    const float sliderKnobOffset = 5.0f; // 滑块偏移量
    const float knobSize = 15.0f; // 滑块显示大小
    const float cornerRadius = 5.0f; // 圆角半径
    const ImU32 c_16b777 = IM_COL32(22, 183, 119, 255); // 滑块颜色
    const ImU32 c_545D6D = IM_COL32(84, 93, 109, 255); // 滑动条颜色
    const ImU32 c_fafafa = IM_COL32(250, 250, 250, 255); // 文本颜色
    const float sensitivity = 0.1f; // 滑块拖动灵敏度
    const bool isShowText = true; // 是否显示滑块值

    // 获取当前光标位置
    ImVec2 startPos = ImGui::GetCursorScreenPos();
    ImVec2 endPos = ImVec2(startPos.x + sliderBarWidth, startPos.y + sliderBarHeight);

    // 绘制滑动条
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(ImVec2(startPos.x + 20, startPos.y + sliderBarHeight - 3), ImVec2(endPos.x, startPos.y + sliderBarHeight + 3), c_545D6D, 20.0f);

    // 计算滑块位置
    float normalizedValue = (*value - sliderMinValue) / (sliderMaxValue - sliderMinValue);
    normalizedValue = std::clamp(normalizedValue, 0.0f, 1.0f);
    float knobX = startPos.x + 20 + normalizedValue * (sliderBarWidth - sliderKnobSize);
    ImVec2 knobPos = ImVec2(knobX - sliderKnobSize * 0.5f, startPos.y - sliderKnobOffset);

    // 绘制滑块左侧填充区域
    drawList->AddRectFilled(ImVec2(startPos.x + 20, startPos.y + sliderBarHeight - 3), ImVec2(knobX, startPos.y + sliderBarHeight + 3), c_16b777, 20.0f);

    // 绘制滑块
    ImVec2 knobTopLeft = ImVec2(knobPos.x - knobSize * 0.5f + knobSize / 2, startPos.y);
    ImVec2 borderMin = ImVec2(knobTopLeft.x - 10.0f, knobTopLeft.y - 10.0f);
    ImVec2 borderMax = ImVec2(knobTopLeft.x + knobSize + 10.0f, knobTopLeft.y + knobSize + 10.0f);
    drawList->AddRectFilled(knobTopLeft, ImVec2(knobTopLeft.x + knobSize, knobTopLeft.y + knobSize), c_16b777, cornerRadius);

    // 显示滑块值
    char valueText[32];
    sprintf(valueText, "%.1f", *value);
    ImVec2 textSize = ImGui::CalcTextSize(valueText);
    ImVec2 textPos = ImVec2((knobPos.x - textSize.x * 0.5f), knobTopLeft.y - 4.0f);
    ImVec2 backgroundPos = textPos;
    ImVec2 backgroundSize = ImVec2(textSize.x + 10.0f, textSize.y + 10.0f);

    if (isShowText) {
        drawList->AddRectFilled(backgroundPos, ImVec2(backgroundPos.x + backgroundSize.x, backgroundPos.y + backgroundSize.y), c_16b777, 7);
        drawList->AddText(ImVec2(backgroundPos.x + (backgroundSize.x - textSize.x) * 0.5f, backgroundPos.y + (backgroundSize.y - textSize.y) * 0.5f), c_fafafa, valueText);
    }

    // 创建不可见按钮以捕获鼠标事件
    ImGui::InvisibleButton(label, ImVec2(sliderBarWidth, sliderBarHeight * 2));

    // 滑块拖动逻辑
    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
        ImVec2 mousePos = ImGui::GetMousePos();
        // 增大滑块的交互区域
        if (mousePos.x >= knobPos.x - 5.0f && mousePos.x <= knobPos.x + sliderKnobSize + 5.0f) {
            sliderState.isDragging = true;
            sliderState.dragOffset = mousePos.x - knobPos.x;
        }
    }

    if (sliderState.isDragging && ImGui::IsMouseDragging(0)) {
        ImVec2 mousePos = ImGui::GetMousePos();
        float dx = mousePos.x - (knobPos.x + sliderState.dragOffset);
        float knobWidth = endPos.x - startPos.x - sliderKnobSize;
        float valueDelta = dx / knobWidth * (sliderMaxValue - sliderMinValue);
        *value = std::clamp<float>(*value + valueDelta * sensitivity, sliderMinValue, sliderMaxValue);
    }

    if (ImGui::IsMouseReleased(0)) {
        sliderState.isDragging = false;
    }

    // 在滑块的两端显示范围标记
    char minText[32];
    sprintf(minText, "%.1f", sliderMinValue);
    ImVec2 minTextSize = ImGui::CalcTextSize(minText);
    ImVec2 minTextPos = ImVec2(startPos.x + 5.0f, startPos.y + sliderBarHeight + 5.0f);
    drawList->AddText(minTextPos, c_545D6D, minText);

    char maxText[32];
    sprintf(maxText, "%.1f", sliderMaxValue);
    ImVec2 maxTextSize = ImGui::CalcTextSize(maxText);
    ImVec2 maxTextPos = ImVec2(endPos.x - maxTextSize.x - 5.0f, startPos.y + sliderBarHeight + 5.0f);
    drawList->AddText(maxTextPos, c_545D6D, maxText);
}
