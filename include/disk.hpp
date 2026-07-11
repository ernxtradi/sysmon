#ifndef DISK_HPP
#define DISK_HPP

#include "types.hpp"
#include <string>

class Disk {
public:
    explicit Disk(const std::string& path = "/");

    DiskInfo getInfo();

private:
    std::string mountPoint;
};

#endif