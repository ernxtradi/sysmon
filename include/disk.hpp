#ifndef DISK_HPP
#define DISK_HPP

#include "types.hpp"
#include <cstdint>
#include <string>

class Disk {
public:
    explicit Disk(const std::string& path = "/");

    DiskInfo getInfo();

private:
    std::string mountPoint;
    std::string deviceName;

    uint64_t previousSectorsRead = 0;
    uint64_t previousSectorsWritten = 0;
    bool hasPreviousIO = false;

    void detectDevice();
    bool readIOCounters(uint64_t& sectorsRead, uint64_t& sectorsWritten);
};

#endif