#include "disk.hpp"

#include <filesystem>

Disk::Disk(const std::string& path)
    : mountPoint(path)
{
}

DiskInfo Disk::getInfo()
{
    DiskInfo info;

    try
    {
        auto space = std::filesystem::space(mountPoint);

        info.total = space.capacity;
        info.free = space.available;
        info.used = info.total - info.free;

        if (info.total != 0)
        {
            info.usage =
                (static_cast<double>(info.used) /
                 static_cast<double>(info.total)) * 100.0;
        }
    }
    catch (...)
    {
        // Leave info initialized to zeros.
    }

    return info;
}