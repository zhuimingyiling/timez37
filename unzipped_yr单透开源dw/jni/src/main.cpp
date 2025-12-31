#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <thread>
#include <cstdio>
#include <fcntl.h>
#include <dirent.h>
#include <pthread.h>
#include <fstream>
#include <time.h>
#include <malloc.h>
#include <iostream>
#include <ctime>
#include <signal.h>
#include <atomic>
#include <memory>
#include <vector>
#include <chrono>
#include <sys/stat.h>
#include <sys/prctl.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
using namespace std;
#include "draw.h"            // 绘制套
#include "AndroidImgui.h"    // 创建绘制套
#include "GraphicsManager.h" // 获取当前渲染模式
#include "Android_draw/timer.h"

timer DrawFPS;
float fps = 120;

// 全局标志位，用于安全退出
std::atomic<bool> g_running(true);
bool running = g_running.load(); // 获取当前值

int main(int argc, char *argv[])
{
    printf("██╗   ██╗ ██████╗ ██╗   ██╗██████╗ ███████╗\n");
    printf("╚██╗ ██╔╝██╔═══██╗██║   ██║██╔══██╗██╔════╝\n");
    printf(" ╚████╔╝ ██║   ██║██║   ██║██████╔╝█████╗ \n");
    printf("  ╚██╔╝  ██║   ██║██║   ██║██╔══██╗██╔══╝  \n");
    printf("   ██║   ╚██████╔╝╚██████╔╝██║  ██║███████╗\n");
    printf("   ╚═╝    ╚═════╝  ╚═════╝ ╚═╝  ╚═╝╚══════╝\n");

    // 捕获退出信号
    signal(SIGINT, [](int)
           { g_running = false; });
    signal(SIGTERM, [](int)
           { g_running = false; });

    if (fork() > 0)
        exit(0);
        
    ::graphics = GraphicsManager::getGraphicsInterface(GraphicsManager::VULKAN); // 绘图方式

    // 获取屏幕信息
    ::screen_config();

    ::native_window_screen_x = (::displayInfo.height > ::displayInfo.width ? ::displayInfo.height : ::displayInfo.width);
    ::native_window_screen_y = (::displayInfo.height > ::displayInfo.width ? ::displayInfo.height : ::displayInfo.width);
    ::abs_ScreenX = (::displayInfo.height > ::displayInfo.width ? ::displayInfo.height : ::displayInfo.width);
    ::abs_ScreenY = (::displayInfo.height < ::displayInfo.width ? ::displayInfo.height : ::displayInfo.width);

    ::window = android::ANativeWindowCreator::Create("1", native_window_screen_x, native_window_screen_y, false);
    graphics->Init_Render(::window, native_window_screen_x, native_window_screen_y);

    Touch::Init({(float)::abs_ScreenX, (float)::abs_ScreenY}, true);
    Touch::setOrientation(displayInfo.orientation);
    std::thread(初始化).detach();
    std::thread(read_thread).detach();
    std::thread(音量).detach();
    // 主循环
    DrawFPS.SetFps(fps);
    DrawFPS.AotuFPS_init();
    DrawFPS.setAffinity();
    ::init_My_drawdata();

    while (g_running)
    {
        drawBegin();
        graphics->NewFrame();
        Layout_tick_UI(&running);
        graphics->EndFrame();
        DrawFPS.AotuFPS();
        DrawFPS.SetFps(fps);
    }
    // 清理
    graphics->Shutdown();
    android::ANativeWindowCreator::Destroy(::window);
    return 0;
}
