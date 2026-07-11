#include "memory.hpp"

#include <fstream>
#include <sstream>
#include <string>

MemoryInfo Memory::getInfo()
{
    MemoryInfo info;

    std::ifstream file("/proc/meminfo");

    if (!file.is_open())
        return info;

    std::string key;
    uint64_t value;
    std::string unit;

    while (file >> key >> value >> unit)
    {
        if (key == "MemTotal:")
            info.total = value * 1024;

        else if (key == "MemAvailable:")
            info.available = value * 1024;
    }

    info.used = info.total - info.available;

    if (info.total != 0)
    {
        info.usage =
            (static_cast<double>(info.used) /
             static_cast<double>(info.total)) * 100.0;
    }

    return info;
}