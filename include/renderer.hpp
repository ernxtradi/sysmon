#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <ncurses.h>

#include "cpu.hpp"
#include "memory.hpp"
#include "disk.hpp"
#include "network.hpp"
#include "process.hpp"
#include "system.hpp"
#include "graphs.hpp"

class Renderer
{
public:
    Renderer();
    ~Renderer();

    void begin();
    void end();

    void clear();

    void drawHeader(const SystemInfo& system);

    void drawCPU(
        const CPUInfo& cpu,
        const Graph& history);

    void drawMemory(
        const MemoryInfo& memory,
        const Graph& history);

    void drawDisk(
        const DiskInfo& disk,
        const Graph& history);

    void drawNetwork(
        const NetworkInfo& network,
        const Graph& history);

    void drawProcesses(
        const std::vector<ProcessInfo>& processes);

    void refreshScreen();

private:
    int width;
    int height;
};

#endif