#include "renderer.hpp"
#include "utils.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace
{

constexpr int HEADER_ROW = 0;
constexpr int CPU_ROW = 4;
constexpr int MEMORY_ROW = 8;
constexpr int DISK_ROW = 12;
constexpr int NETWORK_ROW = 16;
constexpr int PROCESS_ROW = 20;

constexpr int PAIR_GREEN = 1;
constexpr int PAIR_YELLOW = 2;
constexpr int PAIR_RED = 3;

int usageColorPair(double usage)
{
    if (usage >= 90.0)
        return PAIR_RED;

    if (usage >= 60.0)
        return PAIR_YELLOW;

    return PAIR_GREEN;
}

void drawSeparator(int row, int width)
{
    mvhline(row, 0, ACS_HLINE, width);
}

}

Renderer::Renderer()
{
    width = 0;
    height = 0;
}

Renderer::~Renderer()
{
    end();
}

void Renderer::begin()
{
    initscr();

    noecho();

    cbreak();

    curs_set(0);

    keypad(stdscr, TRUE);

    timeout(0);

    start_color();

    use_default_colors();

    init_pair(PAIR_GREEN, COLOR_GREEN, -1);
    init_pair(PAIR_YELLOW, COLOR_YELLOW, -1);
    init_pair(PAIR_RED, COLOR_RED, -1);

    getmaxyx(stdscr, height, width);
}

void Renderer::end()
{
    endwin();
}

void Renderer::clear()
{
    // Pick up terminal resizes (SIGWINCH) each frame rather than only
    // measuring dimensions once at startup.
    getmaxyx(stdscr, height, width);

    erase();
}

void Renderer::refreshScreen()
{
    refresh();
}

void Renderer::drawHeader(const SystemInfo& system)
{
    mvprintw(
        HEADER_ROW,
        0,
        "Host: %s   OS: %s",
        system.hostname.c_str(),
        system.osName.c_str());

    mvprintw(
        HEADER_ROW + 1,
        0,
        "Kernel: %s",
        system.kernel.c_str());

    mvprintw(
        HEADER_ROW + 2,
        0,
        "Uptime: %s",
        system.uptime.c_str());

    drawSeparator(HEADER_ROW + 3, width);
}

void Renderer::drawCPU(
    const CPUInfo& cpu,
    const Graph& history)
{
    int pair = usageColorPair(cpu.usage);

    mvprintw(CPU_ROW, 0, "CPU   ");

    attron(COLOR_PAIR(pair));
    printw("%5.1f%%", cpu.usage);
    attroff(COLOR_PAIR(pair));

    printw(
        "   Temp: %5.1fC   Freq: %4.2fGHz   Cores: %u",
        cpu.temperature,
        cpu.frequency,
        cpu.cores);

    move(CPU_ROW + 1, 0);

    attron(COLOR_PAIR(pair));
    printw("%s", Utils::percentageBar(cpu.usage).c_str());
    attroff(COLOR_PAIR(pair));

    mvprintw(
        CPU_ROW + 2,
        0,
        "%s",
        history.render().c_str());

    drawSeparator(CPU_ROW + 3, width);
}

void Renderer::drawMemory(
    const MemoryInfo& memory,
    const Graph& history)
{
    int pair = usageColorPair(memory.usage);

    mvprintw(MEMORY_ROW, 0, "MEM   ");

    attron(COLOR_PAIR(pair));
    printw("%5.1f%%", memory.usage);
    attroff(COLOR_PAIR(pair));

    printw(
        "   %s / %s",
        Utils::bytesToHuman(memory.used).c_str(),
        Utils::bytesToHuman(memory.total).c_str());

    move(MEMORY_ROW + 1, 0);

    attron(COLOR_PAIR(pair));
    printw("%s", Utils::percentageBar(memory.usage).c_str());
    attroff(COLOR_PAIR(pair));

    mvprintw(
        MEMORY_ROW + 2,
        0,
        "%s",
        history.render().c_str());

    drawSeparator(MEMORY_ROW + 3, width);
}

void Renderer::drawDisk(
    const DiskInfo& disk,
    const Graph& history)
{
    int pair = usageColorPair(disk.usage);

    mvprintw(DISK_ROW, 0, "DISK  ");

    attron(COLOR_PAIR(pair));
    printw("%5.1f%%", disk.usage);
    attroff(COLOR_PAIR(pair));

    printw(
        "   %s / %s",
        Utils::bytesToHuman(disk.used).c_str(),
        Utils::bytesToHuman(disk.total).c_str());

    move(DISK_ROW + 1, 0);

    attron(COLOR_PAIR(pair));
    printw("%s", Utils::percentageBar(disk.usage).c_str());
    attroff(COLOR_PAIR(pair));

    mvprintw(
        DISK_ROW + 2,
        0,
        "%s",
        history.render().c_str());

    drawSeparator(DISK_ROW + 3, width);
}

void Renderer::drawNetwork(
    const NetworkInfo& network,
    const Graph& history)
{
    mvprintw(
        NETWORK_ROW,
        0,
        "NET   IP: %-15s  Down: %s/s   Up: %s/s",
        network.ipAddress.c_str(),
        Utils::bytesToHuman(
            static_cast<uint64_t>(network.downloadSpeed)).c_str(),
        Utils::bytesToHuman(
            static_cast<uint64_t>(network.uploadSpeed)).c_str());

    mvprintw(
        NETWORK_ROW + 1,
        0,
        "%s",
        history.render().c_str());

    drawSeparator(NETWORK_ROW + 2, width);
}

void Renderer::drawProcesses(
    const std::vector<ProcessInfo>& processes)
{
    mvprintw(
        PROCESS_ROW,
        0,
        "%-7s %-7s %-9s %-5s %4s %6s %8s %8s  %s",
        "PID",
        "PPID",
        "USER",
        "STATE",
        "THR",
        "CPU%",
        "RSS",
        "VSZ",
        "COMMAND");

    int row = PROCESS_ROW + 1;

    int maxRows = std::max(0, height - row);

    std::size_t count =
        std::min(processes.size(), static_cast<std::size_t>(maxRows));

    for (std::size_t i = 0; i < count; ++i)
    {
        const ProcessInfo& process = processes[i];

        char prefix[80];

        std::snprintf(
            prefix,
            sizeof(prefix),
            "%-7d %-7d %-9.9s %-5.5s %4u ",
            process.pid,
            process.parentPid,
            process.user.c_str(),
            process.state.c_str(),
            process.threads);

        mvprintw(row + static_cast<int>(i), 0, "%s", prefix);

        int pair = usageColorPair(process.cpuUsage);

        attron(COLOR_PAIR(pair));
        printw("%5.1f%%", process.cpuUsage);
        attroff(COLOR_PAIR(pair));

        std::string suffix =
            " " +
            Utils::bytesToHuman(process.memoryBytes) +
            " " +
            Utils::bytesToHuman(process.virtualMemory) +
            "  ";

        printw("%s", suffix.c_str());

        int usedColumns =
            static_cast<int>(std::strlen(prefix)) +
            6 + // CPU% field width
            static_cast<int>(suffix.size());

        int remaining = std::max(0, width - usedColumns);

        std::string command = process.command;

        if (static_cast<int>(command.size()) > remaining)
            command.resize(remaining);

        printw("%s", command.c_str());
    }
}