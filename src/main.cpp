#include "cpu.hpp"
#include "disk.hpp"
#include "graphs.hpp"
#include "memory.hpp"
#include "network.hpp"
#include "process.hpp"
#include "renderer.hpp"
#include "system.hpp"

#include <algorithm>
#include <chrono>
#include <thread>

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

    while (running)
    {
        //-------------------------------------
        // Read system information
        //-------------------------------------

        CPUInfo cpuInfo = cpu.getInfo();

        MemoryInfo memoryInfo =
            memory.getInfo();

        DiskInfo diskInfo =
            disk.getInfo();

        NetworkInfo networkInfo =
            network.getInfo();

        SystemInfo systemInfo =
            system.getInfo();

        auto processList =
            processes.topMemory(15);

        //-------------------------------------
        // Update graphs
        //-------------------------------------

        cpuGraph.addSample(cpuInfo.usage);

        memoryGraph.addSample(
            memoryInfo.usage);

        diskGraph.addSample(
            diskInfo.usage);

        maxNetworkSpeed = std::max(
            maxNetworkSpeed,
            networkInfo.downloadSpeed);

        networkGraph.addSample(
            (networkInfo.downloadSpeed / maxNetworkSpeed) * 100.0);

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
            processList);

        renderer.refreshScreen();

        //-------------------------------------
        // Keyboard
        //-------------------------------------

        int key = getch();

        switch(key)
        {
            case 'q':
            case 'Q':
                running = false;
                break;
        }

        //-------------------------------------
        // Refresh rate
        //-------------------------------------

        std::this_thread::sleep_for(
            std::chrono::seconds(1));
    }

    renderer.end();

    return 0;
}