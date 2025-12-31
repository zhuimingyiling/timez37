#ifndef NATIVESURFACE_DRAW_H
#define NATIVESURFACE_DRAW_H
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "native_surface/ANativeWindowCreator.h"
#include "AndroidImgui.h"
#include "timer.h"
#include "TouchHelperA.h" //触摸
#include "my_imgui.h"     //字体
#include "GUI_Custom.h"//UI
extern ANativeWindow *window;
// 屏幕信息
extern android::ANativeWindowCreator::DisplayInfo displayInfo;
// 窗口信息
extern ImGuiWindow *g_window;
// 绝对屏幕X _ Y
extern int abs_ScreenX, abs_ScreenY;
extern int native_window_screen_x, native_window_screen_y;
extern std::unique_ptr<AndroidImgui> graphics;
extern ImFont *zh_font;
void 初始化();
// 上次UI位置
struct Last_ImRect
{
    float Pos_x;
    float Pos_y;
    float Size_x;
    float Size_y;
};
extern struct Last_ImRect LastCoordinate;
// 获取屏幕信息
extern void screen_config();
extern void drawBegin();
// 布局UI
extern void Layout_tick_UI(bool *main_thread_flag);
// 初始化绘制数据
extern void init_My_drawdata();
// 触摸函数
extern void AimBotAuto();
// 获取目标
extern void getaim();
// 获取目标2
extern void 音量();
// 获取目标2
extern void ShowLoginScreen();
// 获取目标2
extern void read_thread();
#endif // NATIVESURFACE_DRAW_H
