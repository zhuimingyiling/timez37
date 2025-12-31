#include <iostream>
#include <fstream>
#include <filesystem>
#include <iomanip>

void NumIoSave(const char *name)
{
    if (numSave == nullptr)
    {
        std::string SaveFile = "/data/system/cmwy";
        SaveFile += "/";
        SaveFile += name;
        numSave = fopen(SaveFile.c_str(), "wb+");
    }
    fseek(numSave, 0, SEEK_SET);
    fwrite(按钮, sizeof(bool) * 50, 1, numSave);
    fflush(numSave);
    fsync(fileno(numSave));
}

void NumIoLoad(const char *name)
{
    if (numSave == nullptr)
    {
        std::string SaveFile = "/data/system/cmwy";
        SaveFile += "/";
        SaveFile += name;
        numSave = fopen(SaveFile.c_str(), "rb+");
    }
    if (numSave != nullptr)
    {
        fseek(numSave, 0, SEEK_SET);
        fread(按钮, sizeof(bool) * 50, 1, numSave);
    }
}