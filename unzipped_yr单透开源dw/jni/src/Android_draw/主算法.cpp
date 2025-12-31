#include "库调用/调用.h"
#include "内核驱动/内核读取.h"
#include "定义文件/定义.h"
#include "类名文件/类名.h"
#include "读取文件/文件读取.h"

ImColor TouchingColor = ImColor(255, 0, 0, 150);

// 全局字体变量
ImFont *zh_font = nullptr;

// 尝试加载系统字体
bool M_Android_LoadFont(float SizePixels)
{
    ImGuiIO &io = ImGui::GetIO();

    // 1. 配置字体参数（保持与原有代码一致）
    ImFontConfig config;
    config.FontDataOwnedByAtlas = false; // 系统字体不由ImGui管理释放
    config.SizePixels = SizePixels;      // 字体大小
    config.OversampleH = 1;              // 水平过采样

    // 2. 定义系统字体可能的路径和文件名
    const char *fontDirs[] = {
        "/system/fonts",
        "/system/font",
        "/data/fonts",
        "/vendor/fonts"};

    const char *fontFiles[] = {
        "NotoSansCJK-Regular.ttc",     // 通用CJK字体
        "NotoSansSC-Regular.otf",      // 简体中文
        "NotoSansTC-Regular.otf",      // 繁体中文
        "DroidSansFallback.ttf",       // 安卓 fallback 字体
        "SourceHanSansCN-Regular.otf", // 思源黑体
        "Miui-Regular.ttf"             // 小米系统字体
    };

    // 3. 遍历路径查找可用字体
    char fontPath[256];
    for (const auto &dir : fontDirs)
    {
        // 检查目录是否可读
        if (access(dir, R_OK) != 0)
            continue;

        for (const auto &file : fontFiles)
        {
            // 构建完整路径
            snprintf(fontPath, sizeof(fontPath), "%s/%s", dir, file);

            // 检查文件是否可读
            if (access(fontPath, R_OK) != 0)
                continue;

            // 尝试加载字体（与原有代码一样加载完整中文字符集）
            zh_font = io.Fonts->AddFontFromFileTTF(
                fontPath,
                SizePixels,
                &config,
                io.Fonts->GetGlyphRangesChineseFull() // 关键：保持中文支持
            );

            if (zh_font)
            {
                // 4. 显式设置中文字体相关配置（与原有代码一致）
                io.FontDefault = zh_font;
                io.Fonts->AddFontDefault();

                // 关键步骤：重建字体纹理
                io.Fonts->Build();
                return true;
            }
        }
    }

    // 如果所有字体都加载失败，使用默认字体作为 fallback
    zh_font = io.Fonts->AddFontDefault();
    io.Fonts->Build();
    return zh_font != nullptr;
}

void init_My_drawdata()
{
    // 保持与原有代码完全相同的调用方式
    M_Android_LoadFont(25.0f); // 加载系统字体(包含中文支持)
    ImGui::GetStyle().ScaleAllSizes(1.4f);
}

void 初始化()
{
    NumIoLoad("cnmwy.json");
    if (displayInfo.width > displayInfo.height)
    {
        py = displayInfo.height / 2;
        px = displayInfo.width / 2;
    }
    else
    {
        py = displayInfo.width / 2;
        px = displayInfo.height / 2;
    }
}

void screen_config()
{
    ::displayInfo = android::ANativeWindowCreator::GetDisplayInfo();
}

void drawBegin()
{
    screen_config();
    if (::orientation != displayInfo.orientation)
    {
        ::orientation = displayInfo.orientation;
        Touch::setOrientation(displayInfo.orientation);
        if (g_window != NULL)
        {
            g_window->Pos.x = 100;
            g_window->Pos.y = 125;
        }
    }
}

int 数据()
{
    DIR *dir = opendir("/dev/input/");
    if (dir == NULL)
        return -1;
    struct dirent *ptr = NULL;
    int count = 0;
    while ((ptr = readdir(dir)) != NULL)
    {
        if (strstr(ptr->d_name, "event"))
            count++;
    }
    closedir(dir);
    return count ? count : -1;
}

void 处理输入事件(struct input_event ev)
{
    if (ev.type == EV_KEY && ev.value == 1)
    {
        if (ev.code == KEY_VOLUMEUP)
        { // 音量+
            voice = true;
            MemuSwitch = true;
        }
        else if (ev.code == KEY_VOLUMEDOWN)
        { // 音量-
            voice = false;
        }
    }
}

void 音量()
{
    int EventCount = 数据();
    if (EventCount < 0)
    {
        printf("未找到输入设备\n");
    }

    int *fdArray = (int *)malloc(EventCount * sizeof(int));
    fd_set fds;
    struct timeval tv;
    int maxfd = 0;

    for (int i = 0; i < EventCount; i++)
    {
        char temp[128];
        sprintf(temp, "/dev/input/event%d", i);
        fdArray[i] = open(temp, O_RDONLY | O_NONBLOCK);
        if (fdArray[i] > maxfd)
            maxfd = fdArray[i];
    }

    struct input_event ev;

    while (1)
    {
        FD_ZERO(&fds);
        for (int i = 0; i < EventCount; i++)
        {
            FD_SET(fdArray[i], &fds);
        }

        tv.tv_sec = 5;
        tv.tv_usec = 0;

        int ret = select(maxfd + 1, &fds, NULL, NULL, &tv);
        if (ret == -1)
        {
            perror("select error");
            break;
        }
        else if (ret == 0)
        {
            continue;
        }
        else
        {
            for (int i = 0; i < EventCount; i++)
            {
                if (FD_ISSET(fdArray[i], &fds))
                {
                    memset(&ev, 0, sizeof(ev));
                    if (read(fdArray[i], &ev, sizeof(ev)) == sizeof(ev))
                    {
                        处理输入事件(ev);
                    }
                    usleep(5000);
                }
                usleep(5000);
            }
            usleep(5000);
        }
        usleep(5000);
    }

    for (int i = 0; i < EventCount; i++)
    {
        if (fdArray[i] >= 0)
            close(fdArray[i]);
    }
    free(fdArray);
}

#include "定义文件/SO读取.h"

void read_thread()
{
    pid = get_name_pid1("dwrg");
    get_name_pid(extractedString);
    ModuleBssInfo result;
    if (strstr(extractedString, "com.netease.idv") != NULL)
    {
        libbase = getModuleBasegjf(pid, ".");
        result = get_module_bssgjf(pid, ".");
    }
    else
    {
        libbase = getModuleBase(libso);
        result = get_module_bss(pid, libso);
    }
    c = (result.taddr - result.addr) / 4096;
    long buff[512]; // 缓存指针数据
    while (MatrixOffset == 0 || ArrayaddrOffset == 0)
    {
        状态 = 1;
        for (int i = 0; i < c; i++)
        {
            vm_readv(result.addr + (i * 4096), &buff, 0x1000);

            for (int ii = 0; ii < 512; ii += 1)
            {
if (buff[ii] != 0) {
int tempMatrix = getDword(getPtr64(buff[ii] + 0x98) + 0x2a8);
if (tempMatrix == 970037390 || tempMatrix == 970061201) {
MatrixOffset = result.addr - libbase + i * 4096 + ii * 8;
}
}
if (buff[ii] == 16384) {
int tempszz = getDword(result.addr + 4096 * i + 8 * ii - 0x8);
if (tempszz == 257 && getPtr64(result.addr + i * 4096 + ii * 8 + 0x70) == (result.addr + i * 4096 + ii * 8 + 0x78)) {
ArrayaddrOffset = result.addr - libbase + i * 4096 + ii * 8 + 0x88;
}
                    int tempszz = getDword(result.addr + 4096 * i + 8 * ii + 0x28);
                    if (tempszz == 1 && getFloat(result.addr + 4096 * i + 8 * ii + 0x30) == -1)
                    {
                        自身坐标 = result.addr + 4096 * i + 8 * ii + 0x34;
                    }
                }
                }
            }
        }

        if (MatrixOffset != 0 && ArrayaddrOffset != 0)
        {
            状态 = 2;
            break;
        }
        遍历次数++;
        sleep(5);
    }
    while (true)
    {
        Arrayaddr = getPtr64(libbase + ArrayaddrOffset); // 数组

        int 指针数量 = 0;
        int 板子数量 = 0;
        int 红夫人模式 = 0;
        int 监管者数量 = 0;

        for (int ii = 0; ii < 3000; ii++)
        {
            对象 = getPtr64(Arrayaddr + 0x8 * ii); // 遍历数量次数

            if (对象 == 0)
                break;

            if (对象 <= 0x100000000 || 对象 >= 0x10000000000)
            {
                continue;
            }

            namezfcz = getPtr64(getPtr64(getPtr64(getPtr64(getPtr64(对象 + 0xe0) + 0x0) + 0x8) + 0x20) + 0x20) + 0x8;
            int len = getDword(namezfcz + 0x8);
            if (len <= 0 || len >= 256)
                continue;

            char className[1024] = "\0";
            driver->read(getPtr64(namezfcz), className, len);

            float pd1 = getFloat(对象 + 0x150);
            float pd2 = getFloat(对象 + 0x298);
            int jxpd = getDword(对象 + 0x78);
            bool duplicate = false;
            for (int i = 0; i < 指针数量; i++)
            {
                if (对象 == data[i].obj)
                {
                    duplicate = true;
                    break;
                }
            }
            if (duplicate)
            {
                continue;
            }
            if (strstr(className, "creature") != NULL)
            {
                continue; // 过滤随从
            }
            std::string s;
            if (按钮[4])
            { // 预知开始
                if (strstr(className, "burke_console") == NULL && strstr(className, "h55_joseph_camera") == NULL && strstr(className, "redqueen_e_heijin_yizi") == NULL && strstr(className, "_ghost.gim") == NULL && strstr(className, "butcher_lod") == NULL && strstr(className, "happy_dm65_weapon_saw1.gim") == NULL)
                {
                    if (strstr(className, "boss") != NULL)
                    {
                        s += getboss(className);
                        sprintf(监管者预知, "%s", s.c_str());
                    }
                }
            } // 预知结束
            if (strstr(className, "player") != NULL || strstr(className, "boss") != NULL || pd1 == 450 || strstr(className, "scene") != NULL || strstr(className, "prop") != NULL || strstr(className, "mirror") != NULL || 按钮[16])
            {
                data[指针数量].obj = 对象;
                if (strstr(className, "boss") != NULL)
                {
                    if (jxpd != 65150)
                    {
                        监管者[监管者数量].obj = 对象;
                        监管者[监管者数量].objcoor = getPtr64(对象 + 0x40);
                        监管者数量++;
                    }
                    strcpy(data[指针数量].str, getboss(className));
                    data[指针数量].阵营 = 1;
                }
                else if (strstr(className, "player") != NULL || strstr(className, "npc_deluosi_dress_ghost") != NULL || strstr(className, "h55_pendant_huojian") != NULL)
                {
                    strcpy(data[指针数量].str, getplayer(className));
                    data[指针数量].阵营 = 2;
                }
                else if (strstr(className, "scene") != NULL)
                {
                    data[指针数量].阵营 = 3;
                }
                else if (strstr(className, "prop") != NULL)
                {
                    data[指针数量].阵营 = 4;
                }
                else if (strstr(className, "redqueen") != NULL && strstr(className, "mirror") != NULL && strstr(className, "model") != NULL)
                {
                    data[指针数量].阵营 = 5;
                }
                strcpy(data[指针数量].类名, className);
                data[指针数量].objcoor = getPtr64(对象 + 0x40);
                指针数量++;
            }

            // 红夫人模式
            if (pd1 == 450)
            {
                if (红夫人模式 == 1 && strstr(className, "boss") != NULL && strstr(className, "redqueen") != NULL && strstr(className, "mirror") == NULL && getFloat(getPtr64(对象 + 0x40) + 0xE0) != 0 && getFloat(getPtr64(对象 + 0x40) + 0xE8) != 0 && ((getFloat(getPtr64(对象 + 0x40) + 0xE4) <= -300 && jxpd == 65150) || (getFloat(getPtr64(对象 + 0x40) + 0xE4) >= -300 && jxpd != 65150)))
                {
                    红夫人镜像 = 对象;
                }
                else if (strstr(className, "boss") != NULL && strstr(className, "redqueen") != NULL && strstr(className, "mirror") == NULL && getFloat(getPtr64(对象 + 0x40) + 0xE0) != 0 && getFloat(getPtr64(对象 + 0x40) + 0xE8) != 0 && getFloat(getPtr64(对象 + 0x40) + 0xE4) >= -300 && jxpd != 65150)
                {
                    红夫人 = 对象;
                    红夫人模式 = 1;
                }
            }
        }
        bosscount = 监管者数量;
        if (指针数量 > 100)
        {
            数量 = 指针数量;
        }
        else
        {
            数量 = 100;
        }
        usleep(1000 * 1000 / 2);
    }
}

void Draw_Main(ImDrawList *Draw)
{
    std::string hello = "悠惹绝唱稳定版";
    Draw->AddText(NULL, 40, {px / 4, py + (py / 3) * 2}, ImColor(255, 0, 0, 200), hello.c_str());

    Matrix = getPtr64(getPtr64(libbase + MatrixOffset) + 0x98) + 0x380; // 矩阵
    Z.X = getFloat(自身坐标);
    Z.Z = getFloat(自身坐标 + 4) - 10;
    Z.Y = getFloat(自身坐标 + 8);
    红夫人X = getFloat(getPtr64(红夫人 + 0x40) + 0x50);
    红夫人Z = getFloat(getPtr64(红夫人 + 0x40) + 0x54);
    红夫人Y = getFloat(getPtr64(红夫人 + 0x40) + 0x58);
    红夫人镜像X = getFloat(getPtr64(红夫人镜像 + 0x40) + 0x50);
    红夫人镜像Z = getFloat(getPtr64(红夫人镜像 + 0x40) + 0x54);
    红夫人镜像Y = getFloat(getPtr64(红夫人镜像 + 0x40) + 0x58);
    if ((红夫人Z >= -300 && 红夫人镜像Z >= -300 && 红夫人X != 0 && 红夫人Y != 0 && 红夫人镜像X != 0 && 红夫人镜像Y != 0))
    {
        mirror = true;
    }
    else
    {
        mirror = false;
    }

    Matrix = getPtr64(getPtr64(getPtr64(libbase + MatrixOffset) + 0x98) + 0x10) + 0x180; // 矩阵

    // 修正memset以正确初始化整个数组
    memset(matrix, 0, 16);

    // 读取过滤矩阵数据
    for (int i = 0; i < 17; ++i)
    {
        过滤矩阵[i] = getFloat(Matrix - 4 + 0x4 * i);
    }

    // 若条件满足则填充矩阵
    if (过滤矩阵[0] == 1.0f)
    {
        // 处理前4个元素（带范围检查）
        for (int i = 0; i < 4; ++i)
        {
            const float val = 过滤矩阵[i + 1];
            if (val >= -2.0f && val <= 2.0f)
            {
                matrix[i] = val;
            }
        }

        // 直接拷贝后续元素（无检查）
        memcpy(&matrix[4], &过滤矩阵[5], 12 * sizeof(float));
    }

    if (按钮[4])
    {
        auto textSize = ImGui::CalcTextSize(监管者预知, 0, 25);
        Draw->AddText({px - (textSize.x / 2), 130}, 红色, 监管者预知);
    }

    for (int i = 0; i < 数量; i++)
    {
        if (strstr(data[i].类名, "buzz") != nullptr ||
            strstr(data[i].类名, "nvyao.gim") != nullptr)
        {
            continue; // 跳过当前迭代
        }

        if (D.X == Z.X && D.Y == Z.Y)
        {
            自身 = data[i].obj;
            自身阵营 = data[i].阵营;
        }

        if (D.X == 0 || D.Y == 0)
        {
            continue; // 跳过xy0
        }

        if (D.Z <= -300)
        {
            continue; // 跳过地下
        }

        int jxpd = getDword(data[i].obj + 0x78);
        camera = matrix[3] * D.X + matrix[7] * D.Z + matrix[11] * D.Y + matrix[15];
        距离 = sqrt(pow(D.X - Z.X, 2) + pow(D.Y - Z.Y, 2) + pow(D.Z - Z.Z, 2)) / 11.886;
        r_x = px + (matrix[0] * D.X + matrix[4] * D.Z + matrix[8] * D.Y + matrix[12]) / camera * px;
        r_y = py - (matrix[1] * D.X + matrix[5] * (D.Z + 8.5) + matrix[9] * (D.Y) + matrix[13]) / camera * py;
        r_w = py - (matrix[1] * D.X + matrix[5] * (D.Z + 28.5) + matrix[9] * (D.Y) + matrix[13]) / camera * py;

        auto DrawObjectText = [&](const char *text, ImColor color)
        {
            auto textSize = ImGui::CalcTextSize(text, 0, 25);
            Draw->AddText({r_x - (textSize.x / 2), r_y}, color, text);
        };

        W = (r_y - r_w) / 2;        // 宽度
        H = r_y - r_w;              // 高度
        X1 = r_x - (r_y - r_w) / 4; // X1
        Y1 = r_y - H / 2;           // Y1
        X2 = X1 + W;                // X2
        Y2 = Y1 + H;                // Y2

        if (距离 >= 300)
        {
            continue;
        }

        if (W > 0)
        {
            if (按钮[16])
            {
                std::string test;
                snprintf(objtext, sizeof(objtext), "%lx", data[i].obj);
                test += " [";
                test += std::to_string((int)距离);
                test += " 米]  0x";
                test += objtext;
                test += " [当前动作: ";
                test += std::to_string((int)ztdzpdnb);
                test += " ]";
                test += " [类名] ";
                test += data[i].类名;
                auto textSize = ImGui::CalcTextSize(test.c_str(), 0, 25);
                Draw->AddText({r_x - (textSize.x / 2), r_y}, ImColor(255, 200, 0, 255), test.c_str());
            }

            if (data[i].阵营 == 3)
            {
                if (按钮[6] && strstr(data[i].类名, "dm65_scene_prop_30") != NULL)
                {
                    DrawObjectText("[大门]", ImColor(255, 200, 0, 255));
                }
                else if (按钮[14] && 距离 < 25 && (strstr(data[i].类名, "woodplane001") != NULL || strstr(data[i].类名, "woodplane01") != NULL))
                {
                    DrawObjectText("[板子]", ImColor(255, 200, 0, 255));
                }
                else if (按钮[7] && strstr(data[i].类名, "sender") != NULL)
                {
                    if (getDword(data[i].obj + 0xa8) == 256)
                    {
                        continue;
                    }
                    DrawObjectText("[电机]", ImColor(127, 127, 200));
                }
                else if (按钮[8] && strstr(data[i].类名, "dm65_scene_prop_01") != NULL && 距离 < 38)
                {
                    DrawObjectText("[道具箱]", ImColor(0, 0, 255, 255));
                }
                else if (按钮[10] && strstr(data[i].类名, "dm65_scene_gallow") != NULL &&
                         strstr(data[i].类名, "bashou") == NULL && 距离 < 38)
                {
                    if (getDword(data[i].obj + 0xa8) == 256)
                    {
                        continue;
                    }
                    DrawObjectText("[狂欢之椅]", ImColor(255, 0, 0, 255));
                }
                else if (按钮[9] && strstr(data[i].类名, "dm65_scene_prop_76") != NULL)
                {
                    DrawObjectText("[地窖]", ImColor(255, 0, 255, 255));
                }
            }

            if (按钮[11] && data[i].阵营 == 4)
            {
                std::string s;
                if (strstr(data[i].类名, "h55_pendant_inject") != NULL)
                {
                    s += "[镇静剂]";
                    s += std::to_string((int)距离);
                    s += " 米";
                }
                else if (strstr(data[i].类名, "h55_pendant_moshubang") != NULL)
                {
                    s += "[魔术棒]";
                    s += std::to_string((int)距离);
                    s += " 米";
                }
                else if (strstr(data[i].类名, "h55_pendant_flaregun") != NULL)
                {
                    s += "[信号枪]";
                    s += std::to_string((int)距离);
                    s += " 米";
                }
                else if (strstr(data[i].类名, "h55_pendant_huzhou") != NULL)
                {
                    s += "[护肘]";
                    s += std::to_string((int)距离);
                    s += " 米";
                }
                else if (strstr(data[i].类名, "h55_pendant_map") != NULL)
                {
                    s += "[地图]";
                    s += std::to_string((int)距离);
                    s += " 米";
                }
                else if (strstr(data[i].类名, "h55_pendant_book") != NULL)
                {
                    s += "[书]";
                    s += std::to_string((int)距离);
                    s += " 米";
                }
                else if (strstr(data[i].类名, "h55_pendant_gjx") != NULL)
                {
                    s += "[工具箱]";
                    s += std::to_string((int)距离);
                    s += " 米";
                }
                else if (strstr(data[i].类名, "h55_pendant_glim") != NULL)
                {
                    s += "[手电筒]";
                    s += std::to_string((int)距离);
                    s += " 米";
                }
                else if (strstr(data[i].类名, "h55_pendant_xiangshuiping") != NULL)
                {
                    s += "[忘忧之香]";
                    s += std::to_string((int)距离);
                    s += " 米";
                }
                else if (strstr(data[i].类名, "h55_pendant_controller") != NULL)
                {
                    s += "[遥控器]";
                    s += std::to_string((int)距离);
                    s += " 米";
                }
                else if (strstr(data[i].类名, "h55_pendant_football") != NULL)
                {
                    s += "[橄榄球]";
                    s += std::to_string((int)距离);
                    s += " 米";
                }
                else if (strstr(data[i].类名, "h55_pendant_huaibiao") != NULL)
                {
                    s += "[怀表]";
                    s += std::to_string((int)距离);
                    s += " 米";
                }
                else if (strstr(data[i].类名, "h55_pendant_puppet") != NULL)
                {
                    s += "[厂长傀儡]";
                    s += std::to_string((int)距离);
                    s += " 米";
                }
                // 插眼
                else if (strstr(data[i].类名, "h55_pendant_tower") != NULL)
                {
                    s += "[窥视者]";
                    s += std::to_string((int)距离);
                    s += " 米";
                }
                //
                else if (strstr(data[i].类名, "h55_pendant_patro") != NULL)
                {
                    s += "[巡视者]";
                    s += std::to_string((int)距离);
                    s += " 米";
                }
                auto textSize = ImGui::CalcTextSize(s.c_str(), 0, 25);
                Draw->AddText({r_x - (textSize.x / 2), r_y}, ImColor(255, 200, 0, 255), s.c_str());
            }

            int zy;
            vm_readv(data[i].obj + 0xaa, &zy, 1);

            if (getFloat(data[i].obj + 0x150) == 450)
            {
                if (strstr(data[i].类名, "h55_joseph_camera") != NULL ||             // 约瑟夫相机
                    strstr(data[i].类名, "redqueen_mirror") != NULL ||               // 红夫人镜子
                    strstr(data[i].类名, "burke_console") != NULL ||                 // 疯眼场景
                    strstr(data[i].类名, "h55_survivor_w_shangren_tiaoban") != NULL) // 商人跳板
                {
                    continue; // 跳过以上所有情况
                }
                if (自身 == data[i].obj)
                {
                    continue;
                }

                std::string s;

                if (data[i].阵营 == 1 || data[i].阵营 == 2)
                {
                    if (按钮[3])
                    {
                        s += data[i].str;
                        auto textSize = ImGui::CalcTextSize(s.c_str(), 0, 25);
                        Draw->AddText({X1 + W / 2 - (textSize.x / 2), Y1 - 45}, ImColor(253, 208, 35), s.c_str());
                    }

                    if (按钮[1])
                    {
                        if (jxpd == 65150)
                        {
                            ImGui::GetForegroundDrawList()->AddRect({X1, Y1}, {X2, Y2}, ImColor(255, 255, 255, 255), 3, 0, 1.8);
                        }
                        else if (data[i].阵营 == 1)
                        {
                            ImGui::GetForegroundDrawList()->AddRect({X1, Y1}, {X2, Y2}, ImColor(255, 0, 0, 255), 3, 0, 1.8f);
                        }
                        else if (data[i].阵营 == 2)
                        {
                            if (按钮[15])
                            {
                                ImGui::GetForegroundDrawList()->AddRect({X1, Y1}, {X2, Y2}, ImColor(0, 255, 0, 255), 3, 0, 1.8f);
                            }
                            else
                            {
                                ImGui::GetForegroundDrawList()->AddRect({X1, Y1}, {X2, Y2}, ImColor(0, 255, 0, 255), 3, 0, 1.8f);
                            }
                        }
                    }
                    if (按钮[12])
                    {
                        std::string 人物距离;
                        人物距离 += std::to_string((int)距离);
                        人物距离 += " 米";
                        auto textSize = ImGui::CalcTextSize(人物距离.c_str(), 0, 25);
                        Draw->AddText({X1 + W / 2 - (textSize.x / 2), Y2 + 10}, ImColor(255, 200, 0, 255), 人物距离.c_str());
                    }

                    if (按钮[2])
                    {
                        ImGui::GetForegroundDrawList()->AddLine({px, 160}, {X1 + W / 2, Y1}, ImColor(255, 255, 255), 2);
                    }
                }
            }
        }

        if (mirror && 按钮[5])
        {
            if (getFloat(data[i].obj + 0x150) == 450 && data[i].阵营 == 2)
            {
                std::string ss;
                // 修改后的调用逻辑
                float extended_mirrorX, extended_mirrorY;
                    calculate_mirror_reflection(红夫人X, 红夫人Y, 红夫人镜像X, 红夫人镜像Y, D.X, D.Y, &xs_prime_mirror, &ys_prime_mirror);
                    calculate_line_reflection(红夫人X, 红夫人Y, 红夫人镜像X, 红夫人镜像Y, xs_prime_mirror, ys_prime_mirror, &D.X, &D.Y);
//                }
                camera = matrix[3] * D.X + matrix[7] * D.Z + matrix[11] * D.Y + matrix[15];
                距离 = sqrt(pow(D.X - Z.X, 2) + pow(D.Y - Z.Y, 2) + pow(D.Z - Z.Z, 2)) / 11.886;
                r_x = px + (matrix[0] * D.X + matrix[4] * D.Z + matrix[8] * D.Y + matrix[12]) / camera * px;
                r_y = py - (matrix[1] * D.X + matrix[5] * (D.Z + 8.5) + matrix[9] * (D.Y) + matrix[13]) / camera * py;
                r_w = py - (matrix[1] * D.X + matrix[5] * (D.Z + 28.5) + matrix[9] * (D.Y) + matrix[13]) / camera * py;

                W = (r_y - r_w) / 2;        // 宽度
                H = r_y - r_w;              // 高度
                X1 = r_x - (r_y - r_w) / 4; // X1
                Y1 = r_y - H / 2;           // Y1
                X2 = X1 + W;                // X2
                Y2 = Y1 + H;                // Y2

                if (W > 0)
                {
                    if (按钮[3])
                    {
                        ss += data[i].str;
                        auto textSize = ImGui::CalcTextSize(ss.c_str(), 0, 25);
                        Draw->AddText({X1 + W / 2 - (textSize.x / 2), Y1 - 45}, ImColor(255, 255, 255, 255), ss.c_str());
                    }

                    if (按钮[1])
                    {
                        ImGui::GetForegroundDrawList()->AddRect({X1, Y1}, {X2, Y2}, ImColor(255, 255, 255, 255), 3, 0, 1.8f);
                    }

                    if (按钮[12])
                    {
                        std::string 镜像距离;
                        镜像距离 += std::to_string((int)距离);
                        镜像距离 += " 米";
                        auto textSize = ImGui::CalcTextSize(镜像距离.c_str(), 0, 25);
                        Draw->AddText({X1 + W / 2 - (textSize.x / 2), Y2 + 10}, ImColor(255, 255, 255, 255), 镜像距离.c_str());
                    }

                    if (按钮[2])
                    {
                        ImGui::GetForegroundDrawList()->AddLine({px, 160}, {X1 + W / 2, Y1}, ImColor(255, 255, 255), 2);
                    }
                } // 判断W
            }
        }
    }
}
#include "悬浮窗.cpp"