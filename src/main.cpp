#include "cpu.hpp"
#include "disk.hpp"
#include "graphs.hpp"
#include "memory.hpp"
#include "network.hpp"
#include "process.hpp"
#include "renderer.hpp"
#include "system.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <iterator>
#include <string>

namespace
{

std::vector<ProcessInfo> buildProcessView(
    const std::vector<ProcessInfo>& all,
    const ProcessListState& state)
{
    std::vector<ProcessInfo> view;

    if (state.filterText.empty())
    {
        view = all;
    }
    else
    {
        std::string needle = state.filterText;

        std::transform(
            needle.begin(),
            needle.end(),
            needle.begin(),
            [](unsigned char c) { return std::tolower(c); });

        std::copy_if(
            all.begin(),
            all.end(),
            std::back_inserter(view),
            [&](const ProcessInfo& process)
            {
                std::string name = process.name;

                std::transform(
                    name.begin(),
                    name.end(),
                    name.begin(),
                    [](unsigned char c) { return std::tolower(c); });

                return name.find(needle) != std::string::npos;
            });
    }

    std::sort(
        view.begin(),
        view.end(),
        [&](const ProcessInfo& a, const ProcessInfo& b)
        {
            if (state.sortMode == SortMode::Cpu)
                return a.cpuUsage > b.cpuUsage;

            return a.memoryBytes > b.memoryBytes;
        });

    return view;
}

void handleInput(
    int key,
    bool& running,
    ProcessListState& state,
    ProcessManager& processes,
    const std::vector<ProcessInfo>& currentView)
{
    if (key == ERR)
        return;

    if (state.confirmingKill)
    {
        if (key == 'y' || key == 'Y')
        {
            bool ok = processes.killProcess(state.killPid);

            state.statusMessage =
                (ok ? "Sent SIGTERM to " : "Failed to kill ") +
                std::to_string(state.killPid);

            state.statusTicksRemaining = 20;
            state.confirmingKill = false;
        }
        else if (key == 'n' || key == 'N' || key == 27 /* Esc */)
        {
            state.confirmingKill = false;
        }

        return;
    }

    if (state.filtering)
    {
        if (key == '\n' || key == '\r' || key == KEY_ENTER)
        {
            state.filtering = false;
        }
        else if (key == 27 /* Esc */)
        {
            state.filtering = false;
            state.filterText.clear();
        }
        else if (key == KEY_BACKSPACE || key == 127 || key == 8)
        {
            if (!state.filterText.empty())
                state.filterText.pop_back();
        }
        else if (key >= 32 && key < 127)
        {
            state.filterText += static_cast<char>(key);
        }

        return;
    }

    switch (key)
    {
        case 'q':
        case 'Q':
            running = false;
            break;

        case KEY_UP:
            if (state.selectedIndex > 0)
                state.selectedIndex--;
            break;

        case KEY_DOWN:
            state.selectedIndex++;
            break;

        case 's':
        case 'S':
            state.sortMode =
                (state.sortMode == SortMode::Memory)
                    ? SortMode::Cpu
                    : SortMode::Memory;
            break;

        case '/':
            state.filtering = true;
            break;

        case 'x':
        case 'X':
        case KEY_DC:
            if (!currentView.empty() &&
                state.selectedIndex < currentView.size())
            {
                state.confirmingKill = true;
                state.killPid = currentView[state.selectedIndex].pid;
                state.killName = currentView[state.selectedIndex].name;
            }
            break;
    }
}

} // namespace

int main()
{
    //-----------------------------------------
    // Collectors
    //-----------------------------------------

    CPU cpu;
    Memory memory;
    Disk disk("/");
    Network network;
    ProcessManager processes;
    System system;

    //-----------------------------------------
    // Graphs
    //-----------------------------------------

    Graph cpuGraph(60);
    Graph memoryGraph(60);
    Graph diskGraph(60);
    Graph networkGraph(60);

    //-----------------------------------------
    // Renderer
    //-----------------------------------------

    Renderer renderer;

    renderer.begin();

    bool running = true;

    double maxNetworkSpeed = 1.0;

    CPUInfo cpuInfo;
    MemoryInfo memoryInfo;
    DiskInfo diskInfo;
    NetworkInfo networkInfo;
    SystemInfo systemInfo;

    std::vector<ProcessInfo> allProcesses;

    ProcessListState listState;

    // Force an immediate refresh on the first iteration.
    auto lastRefresh =
        std::chrono::steady_clock::now() - std::chrono::seconds(1);

    while (running)
    {
        //-------------------------------------
        // Refresh data (~once per second) -
        // decoupled from input/render so the
        // UI stays responsive between refreshes.
        //-------------------------------------

        auto now = std::chrono::steady_clock::now();

        if (now - lastRefresh >= std::chrono::seconds(1))
        {
            cpuInfo = cpu.getInfo();
            memoryInfo = memory.getInfo();
            diskInfo = disk.getInfo();
            networkInfo = network.getInfo();
            systemInfo = system.getInfo();

            allProcesses = processes.getProcesses();

            cpuGraph.addSample(cpuInfo.usage);
            memoryGraph.addSample(memoryInfo.usage);
            diskGraph.addSample(diskInfo.usage);

            maxNetworkSpeed =
                std::max(maxNetworkSpeed, networkInfo.downloadSpeed);

            networkGraph.addSample(
                (networkInfo.downloadSpeed / maxNetworkSpeed) * 100.0);

            lastRefresh = now;
        }

        //-------------------------------------
        // Apply sort/filter for display
        //-------------------------------------

        std::vector<ProcessInfo> view =
            buildProcessView(allProcesses, listState);

        if (view.empty())
            listState.selectedIndex = 0;
        else if (listState.selectedIndex >= view.size())
            listState.selectedIndex = view.size() - 1;

        if (listState.statusTicksRemaining > 0)
        {
            listState.statusTicksRemaining--;

            if (listState.statusTicksRemaining == 0)
                listState.statusMessage.clear();
        }

        //-------------------------------------
        // Draw UI
        //-------------------------------------

        renderer.clear();

        renderer.drawHeader(systemInfo);

        renderer.drawCPU(
            cpuInfo,
            cpuGraph);

        renderer.drawMemory(
            memoryInfo,
            memoryGraph);

        renderer.drawDisk(
            diskInfo,
            diskGraph);

        renderer.drawNetwork(
            networkInfo,
            networkGraph);

        renderer.drawProcesses(
            view,
            listState);

        renderer.refreshScreen();

        //-------------------------------------
        // Keyboard (blocks up to 100ms - this
        // paces the loop, no extra sleep needed)
        //-------------------------------------

        int key = getch();

        handleInput(key, running, listState, processes, view);
    }

    renderer.end();

    return 0;
}
