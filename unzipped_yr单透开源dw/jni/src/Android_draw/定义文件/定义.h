#define PI 3.141592653589793238 // 宏定义
std::string 类名;               // 全局变量字符串变量
char gwd1[25];                  // 全局变量字符串变量
char objtext[256];              // 全局变量字符串变量
char Name[1024];                // 全局变量字符串变量
char 监管者预知[1024];          // 全局变量字符串变量
char 当前配置[1024];            // 全局变量字符串变量
float regulation[1024];         // 全局变量字符串变量

float px, py;
float dist;
extern ImColor TouchingColor;
int TeamID;

ImColor 樱桃粉 = ImColor(255, 153, 204, 255);
ImColor 红色 = ImColor(255, 0, 0, 255);
bool 主页界面 = true; // 悬浮窗菜单定义
bool 绘制界面;        // 悬浮窗菜单定义
bool 设置界面;        // 悬浮窗菜单定义
bool 镜像分身, OB检测, 动作判断, 震慑开关;                              // 判断触摸定义
bool 按钮[100];                                                         // 菜单按钮调节定义
float 过滤矩阵[17];                                                     // 矩阵过滤限制定义
float matrix[16];                                                       // 矩阵限制定义
float camera, r_x, r_y, r_w, X1, Y1, X2, Y2, W, H;                      // 绘制所需定义
bool voice = true;                                                      // 音量键调用悬浮窗定义
bool MemuSwitch = false;                                                // 音量键调用悬浮窗定义
bool BallSwitch = false;                                                // 音量键调用悬浮窗定义
static uint32_t orientation = -1;                                       // 过录制所需定义
bool 镜子检测;
FILE *numSave = nullptr;                                   // 字体
uintptr_t obj, objcoor, libbase, Arrayaddr, Count, Matrix; // 绘制所需函数
uintptr_t 对象, 自身, 自身阵营, namezfcz, 自身坐标;        // 绘制所需函数
uintptr_t 红夫人, 红夫人镜像;                              // 绘制所需函数
int woodcount = 0;                                         // 板子指针定义
int bosscount = 0;                                         // 监管指针定义
int yizicount = 0;                                         // 椅子指针定义
int 数量;                                                  // 总数量定义
int 距离;                                                  // 人物距离定义
int dzpdnb,ztdzpdnb;
int abs_ScreenX, abs_ScreenY;                              // 绝对屏幕XY获取
int native_window_screen_x, native_window_screen_y;        // 绝对屏幕XY获取
ANativeWindow *window;                                     // 屏幕获取
android::ANativeWindowCreator::DisplayInfo displayInfo;    // 屏幕获取
ImGuiWindow *g_window;                                     // 屏幕获取
std::unique_ptr<AndroidImgui> graphics;                    // 屏幕获取
struct Last_ImRect LastCoordinate = {0, 0, 0, 0};          // 屏幕获取
// 结构体定义
typedef struct
{
    uintptr_t obj;
    uintptr_t objcoor;
    uintptr_t 动作地址;
    char str[256];
    char 类名[256];
    int 阵营;
} DataStruct;
// Vector3A定义
struct Vector3A
{
    float X;
    float Y;
    float Z;
    Vector3A()
    {
        this->X = 0;
        this->Y = 0;
        this->Z = 0;
    }
    Vector3A(float x, float y, float z)
    {
        this->X = x;
        this->Y = y;
        this->Z = z;
    }
};
// 全局数组
DataStruct data[1000];
DataStruct 板子[300], 椅子[300], 监管者[300];
// 其他
Vector3A D, Z, M;

// 红夫人定义区
bool mirror = false;
float 红夫人X, 红夫人Y, 红夫人Z;
float xs_prime_mirror, ys_prime_mirror;
float 红夫人镜像X, 红夫人镜像Y, 红夫人镜像Z;
struct hongfuren
{
    float Zx1, Zy1;           // 自己坐标
    float Zx2, Zy2, Zz2, Zt4; // 自己镜像坐标
    float Jzx, Jzy, Jzz, Jt4; // 镜子坐标
    float Dx1, Dy1;           // 敌人坐标
} 红夫人JX;                   // hzhfrjx结构体
void calculate_mirror_reflection(float x1, float y1, float x2, float y2, float xs, float ys, float *xs_prime, float *ys_prime)
{
    float xm = (x1 + x2) / 2.0;
    float ym = (y1 + y2) / 2.0;

    *xs_prime = 2.0 * xm - xs;
    *ys_prime = 2.0 * ym - ys;
}

void calculate_line_reflection(float x1, float y1, float x2, float y2, float xs, float ys, float *xs_prime, float *ys_prime)
{
    float A = y2 - y1;
    float B = x1 - x2;
    float C = x2 * y1 - x1 * y2;

    float D = A * xs + B * ys + C;
    float denom = A * A + B * B;

    *xs_prime = xs - 2.0 * A * D / denom;
    *ys_prime = ys - 2.0 * B * D / denom;
}
void extend_mirror_coordinates(
    float x1, float y1, // 红夫人原始坐标
    float x2, float y2, // 镜像原始坐标
    float *x2_extended, // 输出延长后的镜像坐标
    float *y2_extended)
{
    // 向量延长两倍
    *x2_extended = x1 + 2 * (x2 - x1);
    *y2_extended = y1 + 2 * (y2 - y1);
}
// 悬浮窗六芒星定义
void drawHexagonStar(float x, float y, float size, float rotation, ImDrawList *drawList, ImU32 color)
{
    const int numPoints = 6; // 六角星有6个顶点
    ImVec2 center(x, y);
    ImVec2 points[numPoints];
    for (int i = 0; i < numPoints; i++)
    {
        float angle = rotation + 2 * PI * i / numPoints;
        points[i] = ImVec2(center.x + size * cos(angle), center.y + size * sin(angle));
    }
    // 绘制两个大三角形
    drawList->AddLine(points[0], points[2], color, 6.0f);
    drawList->AddLine(points[2], points[4], color, 6.0f);
    drawList->AddLine(points[4], points[0], color, 6.0f);
    drawList->AddLine(points[1], points[3], color, 6.0f);
    drawList->AddLine(points[3], points[5], color, 6.0f);
    drawList->AddLine(points[5], points[1], color, 6.0f);
}

void DrawLogo(float x, float y, float size, float size2)
{
    // 添加绘制六角星并旋转的代码
    static float rotation = 0.0f; // 初始旋转角度
    rotation += 0.005f;           // 调整旋转速度
    // 获取绘图列表
    ImDrawList *drawList = ImGui::GetWindowDrawList();
    // 在指定位置绘制一个黄色六角星，大小为size，旋转角度为rotation
    drawHexagonStar(x, y, size, rotation, drawList, IM_COL32(255, 255, 0, 255));
    // 绘制包裹六角星的圆形
    // 设置文字缩放比例
}