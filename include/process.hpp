#ifndef PROCESS_HPP
#define PROCESS_HPP

#include "types.hpp"
#include <cstdint>
#include <unordered_map>
#include <vector>

class ProcessManager
{
public:
    std::vector<ProcessInfo> getProcesses();

    std::vector<ProcessInfo> topMemory(unsigned int limit = 10);

    bool killProcess(int pid);

private:
    ProcessInfo readProcess(int pid, uint64_t currentSystemCpuTime);

    uint64_t getTotalSystemCpuTime();

    std::unordered_map<int, uint64_t> previousProcessTicks;
    uint64_t previousSystemCpuTime = 0;
};

#endif