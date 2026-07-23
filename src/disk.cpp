#include "disk.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>

Disk::Disk(const std::string& path)
    : mountPoint(path)
{
    detectDevice();
}

void Disk::detectDevice()
{
    std::ifstream file("/proc/mounts");

    std::string line;

    while (std::getline(file, line))
    {
        std::istringstream ss(line);

        std::string device;
        std::string mount;

        if (!(ss >> device >> mount))
            continue;

        if (mount != mountPoint)
            continue;

        if (device.rfind("/dev/", 0) == 0)
            deviceName = device.substr(5);

        break;
    }
}

bool Disk::readIOCounters(uint64_t& sectorsRead, uint64_t& sectorsWritten)
{
    if (deviceName.empty())
        return false;

    std::ifstream file("/proc/diskstats");

    if (!file.is_open())
        return false;

    std::string line;

    while (std::getline(file, line))
    {
        std::istringstream ss(line);

        std::string major;
        std::string minor;
        std::string name;

        if (!(ss >> major >> minor >> name))
            continue;

        if (name != deviceName)
            continue;

        uint64_t readsCompleted;
        uint64_t readsMerged;
        uint64_t timeReading;
        uint64_t writesCompleted;
        uint64_t writesMerged;

        if (!(ss >> readsCompleted
                 >> readsMerged
                 >> sectorsRead
                 >> timeReading
                 >> writesCompleted
                 >> writesMerged
                 >> sectorsWritten))
        {
            return false;
        }

        return true;
    }

    return false;
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

    //-----------------------------------------------------
    // Read/write throughput
    //
    // /proc/diskstats reports cumulative sectors; each sector
    // is always 512 bytes regardless of the device's physical
    // block size.
    //-----------------------------------------------------

    uint64_t sectorsRead = 0;
    uint64_t sectorsWritten = 0;

    if (readIOCounters(sectorsRead, sectorsWritten))
    {
        if (hasPreviousIO)
        {
            if (sectorsRead >= previousSectorsRead)
            {
                info.readSpeed =
                    static_cast<double>(sectorsRead - previousSectorsRead) *
                    512.0;
            }

            if (sectorsWritten >= previousSectorsWritten)
            {
                info.writeSpeed =
                    static_cast<double>(
                        sectorsWritten - previousSectorsWritten) *
                    512.0;
            }
        }

        previousSectorsRead = sectorsRead;
        previousSectorsWritten = sectorsWritten;
        hasPreviousIO = true;
    }

    return info;
}