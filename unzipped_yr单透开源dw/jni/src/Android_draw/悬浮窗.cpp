#include <ctime>
#include <cstdlib>
// 辅助函数定义
void CenterText(const char *text, const ImVec4 &color = ImGui::GetStyleColorVec4(ImGuiCol_Text))
{
    ImVec2 text_size = ImGui::CalcTextSize(text);
    ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - text_size.x) * 0.5f);
    ImGui::TextColored(color, "%s", text);
}

void ActionButton(const char *label, float height, std::function<void()> callback)
{
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 30.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);

    if (ImGui::Button(label, ImVec2(-1, height)))
    {
        callback();
    }

    ImGui::PopStyleVar(2);
    ImGui::Separator();
}

void ActionButtons(const char *label, float height, std::function<void()> callback)
{
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 30.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);

    if (ImGui::Button(label, ImVec2(170, 100)))
    {
        callback();
    }

    ImGui::PopStyleVar(1);
    ImGui::SameLine();
}

// 在全局范围内定义变量来跟踪颜色插值状态
time_t lastColorChangeTime = 0;
float lerpFactor = 0.0f;
ImVec4 currentBackColor = ImVec4(227.0f / 255.0f, 227.0f / 255.0f, 227.0f / 255.0f, 1.0f); // 当前背景颜色
ImVec4 targetColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);                                       // 初始目标颜色

// 生成随机颜色
ImVec4 GenerateRandomColor()
{
    float r = static_cast<float>(rand()) / RAND_MAX;
    float g = static_cast<float>(rand()) / RAND_MAX;
    float b = static_cast<float>(rand()) / RAND_MAX;
    return ImVec4(r, g, b, 1.0f);
}

// 更新背景颜色
void UpdateBackColor()
{
    time_t currentTime = time(0);
    if (currentTime - lastColorChangeTime >= 1)
    { // 每秒更新一次颜色
        lastColorChangeTime = currentTime;
        lerpFactor = 0.0f; // 重置插值因子
        // 随机生成一个新的目标颜色
        targetColor = GenerateRandomColor();
    }
    else
    {
        // 每帧更新插值因子，实现平滑过渡
        lerpFactor += 0.01f; // 控制插值速度，值越小过渡越慢
        if (lerpFactor >= 1.0f)
            lerpFactor = 1.0f;
        currentBackColor = ImLerp(currentBackColor, targetColor, lerpFactor);
    }
}

bool Opening = true;
bool isYuk = true;
bool showText = true;

void ShowLoadingDots(float radius, int dot_count, ImU32 color)
{
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    ImVec2 center = ImGui::GetCursorScreenPos();
    center.x += radius;
    center.y += radius;

    float time = ImGui::GetTime();
    for (int i = 0; i < dot_count; i++)
    {
        float angle = time * 2.0f + i * (2.0f * IM_PI / dot_count);
        float x = center.x + cos(angle) * radius;
        float y = center.y + sin(angle) * radius;
        float size = 4.0f + 2.0f * sin(time * 4.0f + i);

        draw_list->AddCircleFilled(ImVec2(x, y), size, color);
    }
}

void Layout_tick_UI(bool *main_thread_flag)
{
    UpdateBackColor();

    float screenWidth = static_cast<float>(displayInfo.width);
    float screenHeight = static_cast<float>(displayInfo.height);

    ImGuiIO &io = ImGui::GetIO();
    ImGuiStyle &style = ImGui::GetStyle();

    // 样式配置
    style.Colors[ImGuiCol_Text] = ImColor(0, 0, 0, 255);
    style.Colors[ImGuiCol_WindowBg] = currentBackColor; // 使用当前背景颜色
    style.Colors[ImGuiCol_PopupBg] = currentBackColor;  // 弹出窗口背景也使用当前颜色
    style.Colors[ImGuiCol_Border] = ImColor(170, 170, 170, 255);
    style.Colors[ImGuiCol_FrameBg] = ImColor(255, 255, 255, 255);
    style.Colors[ImGuiCol_FrameBgActive] = ImColor(255, 255, 255, 255);
    style.Colors[ImGuiCol_FrameBgHovered] = ImColor(255, 255, 255, 255);
    style.Colors[ImGuiCol_ScrollbarBg] = ImColor(255, 255, 255, 255);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImColor(227, 227, 227, 255);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImColor(227, 227, 227, 255);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImColor(227, 227, 255, 255);
    style.Colors[ImGuiCol_CheckMark] = ImColor(0, 120, 255, 255);
    style.Colors[ImGuiCol_SliderGrab] = ImColor(227, 227, 227, 255);
    style.Colors[ImGuiCol_SliderGrabActive] = ImColor(0, 0, 0, 150);
    style.Colors[ImGuiCol_Separator] = ImColor(227, 227, 227, 255);
    style.Colors[ImGuiCol_SeparatorActive] = ImColor(227, 227, 227, 255);
    style.Colors[ImGuiCol_SeparatorHovered] = ImColor(227, 227, 227, 255);
    style.Colors[ImGuiCol_Button] = ImColor(227, 227, 227, 255);
    style.Colors[ImGuiCol_ButtonActive] = ImColor(215, 215, 215, 255);
    style.Colors[ImGuiCol_ButtonHovered] = ImColor(215, 215, 215, 220);
    style.Colors[ImGuiCol_HeaderActive] = ImColor(0, 0, 0, 150);
    style.Colors[ImGuiCol_HeaderHovered] = ImColor(227, 227, 227, 255);
    style.Colors[ImGuiCol_TextSelectedBg] = ImColor(255, 255, 255, 255);
    style.Colors[ImGuiCol_Header] = ImColor(255, 255, 255, 255);

    Draw_Main(ImGui::GetForegroundDrawList());

    if (MemuSwitch)
    {
        const float ANIM_SPEED_FAST = 0.3f;
        const float ANIM_SPEED_SLOW = 0.7f;
        static float Velua = 0.0f;

        float targetVelua = voice ? 1.0f : 0.0f;
        float animSpeed = (Velua < targetVelua) ? ANIM_SPEED_FAST : ANIM_SPEED_SLOW;
        float deltaTimeScaled = io.DeltaTime / animSpeed;

        Velua = ImLerp(Velua, targetVelua, deltaTimeScaled);

        float width = 970 * Velua;
        float height = 830 * Velua;

        ImVec2 windowPos = ImVec2((screenWidth - width) / 2.0f, (screenHeight - height) / 2.0f - (screenHeight - height) * (1 - Velua));

        ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);

        if (width > 0 && height > 0)
        {
            if (MemuSwitch)
            {
                ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 45.0f);
                ImGui::SetNextWindowBgAlpha(0.4f);
                style.WindowRounding = 25.0f;
                if (ImGui::Begin("##MORISHIMA CHEAT", &MemuSwitch, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar))
                {
                    ImVec4 backColor;
                    backColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                    // 左侧菜单
                    if (ImGui::BeginChild("##left_menu", ImVec2(250, -1), false, ImGuiWindowFlags_NoScrollbar))
                    {
                        ImGui::CardPanel2("选择背板", ImVec2(250, 0), backColor, 20, 20, [&]()
                                          {
                            auto pos = ImGui::GetWindowPos();
                            float starX = pos.x + 125;
                            float starY = pos.y + 105;
                            float starSize = 80.0f;
                            float starSize2 = 68.0f;
                            DrawLogo(starX, starY, starSize, starSize2);
                            
                            ImGui::SetCursorPos({0, 200});
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
                            ImGui::Separator();
                            
                            // 按钮样式配置
                            ImGuiStyle 边框 = ImGui::GetStyle();
                            边框.FrameBorderSize = 2.0f;
                            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 边框.FrameBorderSize);
                            
                            ImGuiStyle 圆角 = ImGui::GetStyle();
                            圆角.FrameRounding = 30.0f;
                            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 圆角.FrameRounding);
                            
                            if (ImGui::Button("主页", ImVec2(-1,50))) {
                                主页界面=true;
                                绘制界面=false;
                                设置界面=false;
                            }
                            ImGui::Separator();
                            
                            if (ImGui::Button("绘制", ImVec2(-1,50))) {
                                主页界面=false;
                                绘制界面=true;
                                设置界面=false;
                            }
                            ImGui::Separator();
                            
                            if (ImGui::Button("设置", ImVec2(-1,50))) {
                                主页界面=false;
                                绘制界面=false;
                                设置界面=true;
                            }
                            
                            ImGui::PopStyleVar(); // 边框结束
                            ImGui::PopStyleVar(); // 圆角结束
                            
                            ImGui::Separator();
                            ImGui::PopStyleColor(5); });
                        ImGui::EndChild();
                    }
                    ImGui::SameLine();
                    // 主页内容
                    if (主页界面)
                    {
                        ImGui::CardPanel2("主页背板", ImVec2(-1, 0), backColor, 20, 20, [&]()
                                          {
                            ImVec2 text_size = ImGui::CalcTextSize("熟人不知我往事 不愿安于现状 重新开始胜过一切 By.悠惹");
                            ImGui::SetCursorPosX((ImGui::GetWindowSize().x - text_size.x) / 2);
                            ImGui::TextColored(ImVec4(0.0f, 0.7f, 0.0f, 1.0f), "熟人不知我往事 不愿安于现状 重新开始胜过一切 By.悠惹");
                            ImGui::Separator();
                            
                            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "系统信息");
        ImGui::Separator();
        ImGui::Text("渲染模式: %s", graphics->RenderName);
        ImGui::Text("GUI 版本: %s", IMGUI_VERSION);
        ImGui::Text("帧率: %.1f FPS (%.2f ms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);

        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "游戏信息");
        ImGui::Separator();
        ImGui::Text("包名: %s", extractedString);
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "内存地址");
        ImGui::Separator();
        ImGui::Text("模块基址: 0x%lx", libbase);
        ImGui::SameLine();
        ImGui::Text("对象数组: 0x%lx", Arrayaddr);
        ImGui::Text("矩阵地址: 0x%lx", Matrix);
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "偏移与统计");
        ImGui::Separator();
        ImGui::Text("矩阵偏移: 0x%lx", MatrixOffset);
        ImGui::SameLine();
        ImGui::Text("数组偏移: 0x%lx", ArrayaddrOffset);
        ImGui::Text("模块页数: %d", c);
        ImGui::SameLine();
        ImGui::Text("对象数量: %d", 数量);
                            ImGui::PopStyleColor(4);
                            ImGui::PopStyleVar();
                            ImGui::SetWindowFontScale(0.8f); });
                    }
                    ImGui::SameLine();
                    // 绘制界面
                    if (绘制界面)
                    {
                        ImGui::CardPanel("绘制背板", ImVec2(-1, 0), backColor, 20, 20, [&]()
                                         {
                                             ImGui::TextColored(ImVec4(0.0f, 0.5f, 1.0f, 1.0f), "主要绘制");
                                             ImGui::Separator2("", ImVec2(580, 15));

                                             // 按钮样式配置
                                             ImGuiStyle 边框 = ImGui::GetStyle();
                                             边框.FrameBorderSize = 2.0f;
                                             ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 边框.FrameBorderSize);

                                             ImGuiStyle 圆角 = ImGui::GetStyle();
                                             圆角.FrameRounding = 30.0f;
                                             ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 圆角.FrameRounding);

                            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.5f);
                            ImGui::SetNextItemWidth(-1);
                            
                            ImVec2 text_size3 = ImGui::CalcTextSize("当前时间");
                            ImGui::SetCursorPosX((ImGui::GetWindowSize().x - text_size3.x) / 2);
                            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "当前时间");
                            
                            auto currentTime = std::time(nullptr);
                            auto localTime = *std::localtime(&currentTime);
                            char buffer[64];
                            std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &localTime);
                            
                            ImGui::SetWindowFontScale(2.0f);
                            ImVec2 textSize = ImGui::CalcTextSize(buffer);
                            float windowWidth1 = ImGui::GetWindowSize().x;
                            float textX1 = (windowWidth1 - textSize.x) * 0.5f;
                            ImGui::SetCursorPosX(textX1);
                            ImGui::Text("%s", buffer);
                            ImGui::SetWindowFontScale(1.0f);
                            ImGui::ItemSize(ImVec2(0, 10));
                            
                            if (ImGui::Button("单透模式", {-1, 70})) {
                                Touch::Close();
                            } 
                          
                          
                                             ImGui::Checkbox("人物方框", &按钮[1]);
                                             ImGui::SameLine();
                                             ImGui::Checkbox("人物射线", &按钮[2]);
                                             ImGui::SameLine();
                                             ImGui::Checkbox("绘制名字", &按钮[3]);

                                             ImGui::Checkbox("预知监管", &按钮[4]);
                                             ImGui::SameLine();
                                             ImGui::Checkbox("夫人模式", &按钮[5]);
                                             ImGui::SameLine();
                                             ImGui::Checkbox("绘制大门", &按钮[6]);

                                             ImGui::Checkbox("绘制电机", &按钮[7]);
                                             ImGui::SameLine();
                                             ImGui::Checkbox("绘制箱子", &按钮[8]);
                                             ImGui::SameLine();
                                             ImGui::Checkbox("绘制地窖", &按钮[9]);

                                             ImGui::Checkbox("绘制椅子", &按钮[10]);
                                             ImGui::SameLine();
                                             ImGui::Checkbox("绘制道具", &按钮[11]);
                                             ImGui::SameLine();
                                             ImGui::Checkbox("绘制距离", &按钮[12]);

                                             ImGui::Checkbox("技能提示", &按钮[13]);
                                             ImGui::SameLine();
                                             ImGui::Checkbox("绘制板子", &按钮[14]);
                                             ImGui::SameLine();
                                             ImGui::Checkbox("绘制调试", &按钮[16]);

                                             ImGui::Checkbox("绘制链条", &镜子检测);
                                             ImGui::PopStyleVar(); // 边框结束
                                             ImGui::PopStyleVar(); // 圆角结束
                                         });
                    }
                    ImGui::SameLine();
                    // 设置页面内容
                    if (设置界面)
                    {
                        if (ImGui::BeginChild("##设置父背板", ImVec2(-1, -1), false))
                        {
                            constexpr ImVec4 TITLE_COLOR = {0.0f, 0.5f, 1.0f, 1.0f};
                            constexpr ImVec4 SUBTITLE_COLOR = {0.66f, 0.66f, 0.66f, 1.0f};

                            ImGui::SetWindowFontScale(1.6f);
                            CenterText("悠惹 Kernel", TITLE_COLOR);
                            ImGui::SetWindowFontScale(0.8f);
                            CenterText("成功者绝不放弃", SUBTITLE_COLOR);
                            CenterText("放弃者绝不会成功", SUBTITLE_COLOR);
                            ImGui::Spacing();

                            ImGui::CardPanel2(
                                "设置背板2",
                                ImVec2(-1, 173),
                                backColor,
                                20,
                                20,
                                [&]()
                                {
                                    ImGui::SetWindowFontScale(1.2f);
                                    ImGui::TextWrapped("本辅助仅提供学习、交流，请勿用于商业贩卖或二次倒卖\n"
                                                       "使用者的任何行为与辅助开发者无关\n"
                                                       "请在24小时内删除本应用\n"
                                                       "下载并使用本辅助即代表同意此用户协议\n");
                                    ImGui::SetWindowFontScale(1.0f);
                                });

                            ImGui::CardPanel2(
                                "设置背板3",
                                ImVec2(-1, 0),
                                backColor,
                                20,
                                20,
                                [&]()
                                {
                                    constexpr float BUTTON_HEIGHT = 80.0f;
                                    ImGui::StyleColorsDark();
                                    ImGui::Dummy(ImVec2(0, 3));
                                    ImGui::Separator();
                                    ActionButton("重置配置", BUTTON_HEIGHT, []()
                                                 { 
                                                         if (access("/data/system/cmwy/cnmwy.json", W_OK) == -1) {
                                                             // 权限不足，提示用户
                                                             ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "权限不足，无法删除配置文件");
                                                         } else {
                                                             system("rm -rf /data/system/cmwy/cnmwy.json > /dev/null 2>&1");
                                                         } });

                                    ActionButton("保存配置", BUTTON_HEIGHT, []()
                                                 { NumIoSave("cnmwy.json"); });

                                    ActionButton("安全退出", BUTTON_HEIGHT, []()
                                                 {
                                                
                Touch::Close();
                graphics->Shutdown();
                android::ANativeWindowCreator::Destroy(::window);
                exit(0); });
                                });
                            ImGui::EndChild();
                        }
                    }
                    ImGui::End();
                }
                ImGui::PopStyleVar();
            }
        }
        ImGui::Render();
    }
}