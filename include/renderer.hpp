#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <ncurses.h>

#include <cstddef>
#include <string>

#include "cpu.hpp"
#include "memory.hpp"
#include "disk.hpp"
#include "network.hpp"
#include "process.hpp"
#include "system.hpp"
#include "graphs.hpp"

enum class SortMode
{
    Memory,
    Cpu
};

// UI state for the interactive process list. Owned by the caller
// (main.cpp) and mutated in place: selectedIndex/sort/filter/kill
// fields in response to input, scrollOffset by the renderer so the
// selection stays on screen.
struct ProcessListState
{
    SortMode sortMode = SortMode::Memory;

    std::size_t selectedIndex = 0;
    std::size_t scrollOffset = 0;

    bool filtering = false;
    std::string filterText;

    bool confirmingKill = false;
    int killPid = 0;
    std::string killName;

    std::string statusMessage;
    int statusTicksRemaining = 0;
};

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
        const std::vector<ProcessInfo>& processes,
        ProcessListState& state);

    void refreshScreen();

private:
    int width;
    int height;
    int nextRow;
};

#endif