//Shared Data Structures used throughout the project
#ifndef TYPES_HPP
#define TYPES_HPP

#include <string>
#include <vector>
#include <cstdint>

struct CPUInfo {
    double usage = 0.0;
    double temperature = 0.0;
    double frequency = 0.0;
    unsigned int cores = 0;
    std::vector<double> perCoreUsage;
};

struct MemoryInfo {
    uint64_t total = 0;
    uint64_t available = 0;
    uint64_t used = 0;
    double usage = 0.0;
};

struct DiskInfo {
    uint64_t total = 0;
    uint64_t free = 0;
    uint64_t used = 0;
    double usage = 0.0;
    double readSpeed = 0.0;
    double writeSpeed = 0.0;
};

struct NetworkInfo {
    uint64_t bytesReceived = 0;
    uint64_t bytesSent = 0;
    double downloadSpeed = 0.0;
    double uploadSpeed = 0.0;
    std::string ipAddress;
};

struct ProcessInfo
{
    int pid = 0;
    int parentPid = 0;

    std::string name;
    std::string user;
    std::string state;
    std::string command;

    uint64_t memoryBytes = 0;
    uint64_t virtualMemory = 0;

    unsigned int threads = 0;

    double cpuUsage = 0.0;
};

struct SystemInfo {
    std::string hostname;
    std::string kernel;
    std::string uptime;
    std::string osName;
};

#endif