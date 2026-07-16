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

    std::string line;

    uint64_t memFree = 0;
    uint64_t buffers = 0;
    uint64_t cached = 0;

    bool hasMemAvailable = false;

    while (std::getline(file, line))
    {
        std::istringstream ss(line);

        std::string key;
        uint64_t value;

        if (!(ss >> key >> value))
            continue;

        if (key == "MemTotal:")
            info.total = value * 1024;

        else if (key == "MemAvailable:")
        {
            info.available = value * 1024;
            hasMemAvailable = true;
        }
        else if (key == "MemFree:")
            memFree = value * 1024;

        else if (key == "Buffers:")
            buffers = value * 1024;

        else if (key == "Cached:")
            cached = value * 1024;
    }

    // Kernels older than 3.14 don't expose MemAvailable. Approximate it
    // the way tools like `free` do on those kernels.
    if (!hasMemAvailable)
        info.available = memFree + buffers + cached;

    info.used = info.total - info.available;

    if (info.total != 0)
    {
        info.usage =
            (static_cast<double>(info.used) /
             static_cast<double>(info.total)) * 100.0;
    }

    return info;
}