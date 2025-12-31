int 状态 = 0;
int 数据获取状态 = 0;
int 遍历次数 = 0;
char extractedString[64];
long int MatrixOffset = 0, ArrayaddrOffset = 0;
typedef struct
{
    unsigned long addr;
    unsigned long taddr;
} ModuleBssInfo;

ModuleBssInfo get_module_bss(int pid, const char *module_name)
{
    FILE *fp;
    ModuleBssInfo info = {0, 0};
    char filename[64];
    char line[1024];

    // 生成文件名
    snprintf(filename, sizeof(filename), "/proc/%d/maps", pid);

    // 打开文件
    fp = fopen(filename, "r");

    bool found_module = false;

    if (fp != NULL)
    {
        while (fgets(line, sizeof(line), fp))
        {
            // 先判断是否包含模块名
            if (strstr(line, module_name) != NULL)
            {
                found_module = true;
            }

            if (found_module)
            {
                // 检查是否满足rw权限且行长度符合要求
                long addr, taddr;
                sscanf(line, "%lx-%lx", &addr, &taddr);
                if (strstr(line, "rw") != NULL && strlen(line) < 86 && (taddr - addr) / 4096 >= 2800)
                {
                    // printf("%d", (taddr-addr)/4096);

                    // 将行按空格分割成字符串数组（这里简单示意，实际可能需要更完善的分割函数）
                    char *words[10];
                    int numWords = 0;
                    char *token = strtok(line, " ");
                    while (token != NULL && numWords < 10)
                    {
                        words[numWords++] = token;
                        token = strtok(NULL, " ");
                    }

                    // 遍历分割后的字符串数组，查找地址范围并转换
                    for (int i = 0; i < numWords; i++)
                    {
                        if (sscanf(words[i], "%lx-%lx", &info.addr, &info.taddr) == 2)
                        {
                            fclose(fp);
                            return info;
                        }
                    }

                    // 如果未找到正确格式的地址范围，设置为0并返回
                    info.addr = 0;
                    info.taddr = 0;
                    fclose(fp);
                    return info;
                }
            }
        }

        fclose(fp);
    }

    return info;
}

ModuleBssInfo get_module_bssgjf(int pid, const char *module_name)
{
    FILE *fp;
    ModuleBssInfo info = {0, 0};
    long addr, taddr;
    char *pch;
    char filename[64];
    char line[1024];
    snprintf(filename, sizeof(filename), "/proc/%d/maps", pid);
    fp = fopen(filename, "r");
    bool is = false;
    if (fp != NULL)
    {
        while (fgets(line, sizeof(line), fp))
        {
            sscanf(line, "%lx-%lx", &addr, &taddr);
            if (strstr(line, module_name) && strstr(line, "r-xp") != NULL && (taddr - addr) == 114982912)
            {
                is = true;
            }
            if (is)
            {
                if (strstr(line, "rw") != NULL && !feof(fp) && (strlen(line) < 86))
                {
                    long addr, taddr;
                    sscanf(line, "%lx-%lx", &addr, &taddr);
                    if ((taddr - addr) / 4096 <= 3000)
                        continue;
                    if (sscanf(line, "%lx-%lx", &info.addr, &info.taddr) != 2)
                    {
                        // 处理转换失败的情况
                        info.addr = 0;
                        info.taddr = 0;
                        break;
                    }
                    break;
                }
            }
        }
        fclose(fp);
    }
    return info;
}
int get_name_pid1(const char *packageName)
{
    int id = -1;
    DIR *dir;
    FILE *fp;
    char filename[64];
    char cmdline[64];
    struct dirent *entry;
    dir = opendir("/proc");
    while ((entry = readdir(dir)) != NULL)
    {
        id = atoi(entry->d_name);
        if (id != 0)
        {
            sprintf(filename, "/proc/%d/cmdline", id);
            fp = fopen(filename, "r");
            if (fp)
            {
                fgets(cmdline, sizeof(cmdline), fp);
                fclose(fp);
                if ((strstr(cmdline, packageName) != NULL || strstr(cmdline, "com.netease.idv") != NULL) && strstr(cmdline, "com") != NULL && strstr(cmdline, "PushService") == NULL && strstr(cmdline, "gcsdk") == NULL)
                {
                    sprintf(extractedString, "%s", cmdline);
                    closedir(dir);
                    id = id;
                    return id;
                    break;
                }
            }
        }
    }
    closedir(dir);
    return -1;
}
long getModuleBasegjf(int pid, const char *module_name)
{
    FILE *fp;
    long addr, taddr;
    char *pch;
    char filename[64];
    char line[1024];
    snprintf(filename, sizeof(filename), "/proc/%d/maps", pid);
    fp = fopen(filename, "r");
    bool is = false;
    if (fp != NULL)
    {
        while (fgets(line, sizeof(line), fp))
        {
            if (strstr(line, "r-xp") != NULL && !feof(fp) && strstr(line, module_name))
            {
                sscanf(line, "%lx-%lx", &addr, &taddr);
                if ((taddr - addr) == 114982912)
                {
                    // 处理转换失败的情况
                    fclose(fp);
                    return addr;
                    break;
                }
                // break;
            }
        }
        fclose(fp);
    }
    return 0;
}
int c;
char libso[256] = {"libclient.so"};