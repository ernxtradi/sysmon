#ifndef PROCESS_HPP
#define PROCESS_HPP

#include "types.hpp"
#include <vector>

class ProcessManager
{
public:
    std::vector<ProcessInfo> getProcesses();

    std::vector<ProcessInfo> topMemory(unsigned int limit = 10);

    bool killProcess(int pid);

private:
    ProcessInfo readProcess(int pid);
};

#endif