#include "renderer.hpp"
#include "utils.hpp"

#include <algorithm>
#include <clocale>
#include <cstdio>
#include <cstring>

namespace
{

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

// mvprintw doesn't stop at the right edge - text longer than the
// remaining width wraps onto the next line, corrupting whatever is
// drawn there next (e.g. a long kernel version string bleeding into
// the uptime line below it). Clip first so a single call never
// crosses the edge.
void printClipped(int row, int col, int width, const std::string& text)
{
    int available = std::max(0, width - col);

    std::string clipped = text;

    if (static_cast<int>(clipped.size()) > available)
        clipped.resize(static_cast<std::size_t>(available));

    mvprintw(row, col, "%s", clipped.c_str());
}

}

Renderer::Renderer()
{
    width = 0;
    height = 0;
    nextRow = 0;
}

Renderer::~Renderer()
{
    end();
}

void Renderer::begin()
{
    // Must happen before initscr() - it's how ncursesw detects the
    // terminal's encoding is UTF-8 and switches its printw()/mvprintw()
    // cursor tracking from bytes to decoded glyphs.
    std::setlocale(LC_ALL, "");

    initscr();

    noecho();

    cbreak();

    curs_set(0);

    keypad(stdscr, TRUE);

    // Block for up to 100ms waiting for a key; this is what paces the
    // main loop now (replacing a fixed post-render sleep), so input is
    // picked up promptly instead of once per data-refresh cycle.
    timeout(100);

#ifdef NCURSES_VERSION
    // Default ESCDELAY (~1s) makes Esc feel unresponsive when using it
    // to back out of filter/kill-confirm mode.
    set_escdelay(25);
#endif

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

    nextRow = 0;
}

void Renderer::refreshScreen()
{
    refresh();
}

void Renderer::drawHeader(const SystemInfo& system)
{
    int row = nextRow;

    char hostLine[512];

    std::snprintf(
        hostLine,
        sizeof(hostLine),
        "Host: %s   OS: %s",
        system.hostname.c_str(),
        system.osName.c_str());

    char kernelLine[512];

    std::snprintf(
        kernelLine,
        sizeof(kernelLine),
        "Kernel: %s",
        system.kernel.c_str());

    printClipped(row, 0, width, hostLine);
    printClipped(row + 1, 0, width, kernelLine);

    mvprintw(
        row + 2,
        0,
        "Uptime: %s",
        system.uptime.c_str());

    drawSeparator(row + 3, width);

    nextRow = row + 4;
}

void Renderer::drawCPU(
    const CPUInfo& cpu,
    const Graph& history)
{
    int row = nextRow;

    int pair = usageColorPair(cpu.usage);

    mvprintw(row, 0, "CPU   ");

    attron(COLOR_PAIR(pair));
    printw("%5.1f%%", cpu.usage);
    attroff(COLOR_PAIR(pair));

    printw(
        "   Temp: %5.1fC   Freq: %4.2fGHz   Cores: %u",
        cpu.temperature,
        cpu.frequency,
        cpu.cores);

    move(row + 1, 0);

    attron(COLOR_PAIR(pair));
    printw("%s", Utils::percentageBar(cpu.usage).c_str());
    attroff(COLOR_PAIR(pair));

    int perCoreRow = row + 2;
    int coreLines = 0;

    if (!cpu.perCoreUsage.empty())
    {
        constexpr int columnWidth = 10; // "C00: 100% "

        int columns = std::max(1, width / columnWidth);

        for (std::size_t i = 0; i < cpu.perCoreUsage.size(); ++i)
        {
            int line = static_cast<int>(i) / columns;
            int col = (static_cast<int>(i) % columns) * columnWidth;

            int corePair = usageColorPair(cpu.perCoreUsage[i]);

            move(perCoreRow + line, col);

            attron(COLOR_PAIR(corePair));
            printw("C%-2zu%4.0f%%", i, cpu.perCoreUsage[i]);
            attroff(COLOR_PAIR(corePair));
        }

        coreLines =
            static_cast<int>((cpu.perCoreUsage.size() + columns - 1) / columns);
    }

    mvprintw(
        perCoreRow + coreLines,
        0,
        "%s",
        history.render().c_str());

    int bottom = perCoreRow + coreLines + 1;

    drawSeparator(bottom, width);

    nextRow = bottom + 1;
}

void Renderer::drawMemory(
    const MemoryInfo& memory,
    const Graph& history)
{
    int row = nextRow;

    int pair = usageColorPair(memory.usage);

    mvprintw(row, 0, "MEM   ");

    attron(COLOR_PAIR(pair));
    printw("%5.1f%%", memory.usage);
    attroff(COLOR_PAIR(pair));

    printw(
        "   %s / %s",
        Utils::bytesToHuman(memory.used).c_str(),
        Utils::bytesToHuman(memory.total).c_str());

    move(row + 1, 0);

    attron(COLOR_PAIR(pair));
    printw("%s", Utils::percentageBar(memory.usage).c_str());
    attroff(COLOR_PAIR(pair));

    mvprintw(
        row + 2,
        0,
        "%s",
        history.render().c_str());

    drawSeparator(row + 3, width);

    nextRow = row + 4;
}

void Renderer::drawDisk(
    const DiskInfo& disk,
    const Graph& history)
{
    int row = nextRow;

    int pair = usageColorPair(disk.usage);

    mvprintw(row, 0, "DISK  ");

    attron(COLOR_PAIR(pair));
    printw("%5.1f%%", disk.usage);
    attroff(COLOR_PAIR(pair));

    printw(
        "   %s / %s   R: %s/s   W: %s/s",
        Utils::bytesToHuman(disk.used).c_str(),
        Utils::bytesToHuman(disk.total).c_str(),
        Utils::bytesToHuman(static_cast<uint64_t>(disk.readSpeed)).c_str(),
        Utils::bytesToHuman(static_cast<uint64_t>(disk.writeSpeed)).c_str());

    move(row + 1, 0);

    attron(COLOR_PAIR(pair));
    printw("%s", Utils::percentageBar(disk.usage).c_str());
    attroff(COLOR_PAIR(pair));

    mvprintw(
        row + 2,
        0,
        "%s",
        history.render().c_str());

    drawSeparator(row + 3, width);

    nextRow = row + 4;
}

void Renderer::drawNetwork(
    const NetworkInfo& network,
    const Graph& history)
{
    int row = nextRow;

    mvprintw(
        row,
        0,
        "NET   IP: %-15s  Down: %s/s   Up: %s/s",
        network.ipAddress.c_str(),
        Utils::bytesToHuman(
            static_cast<uint64_t>(network.downloadSpeed)).c_str(),
        Utils::bytesToHuman(
            static_cast<uint64_t>(network.uploadSpeed)).c_str());

    mvprintw(
        row + 1,
        0,
        "%s",
        history.render().c_str());

    drawSeparator(row + 2, width);

    nextRow = row + 3;
}

void Renderer::drawProcesses(
    const std::vector<ProcessInfo>& processes,
    ProcessListState& state)
{
    int row = nextRow;

    //-------------------------------------------------------
    // Status line: kill confirmation > filter input > a
    // transient status message > the default legend.
    //-------------------------------------------------------

    if (state.confirmingKill)
    {
        attron(COLOR_PAIR(PAIR_RED));

        mvprintw(
            row,
            0,
            "Kill PID %d (%s)? [y/n]",
            state.killPid,
            state.killName.c_str());

        attroff(COLOR_PAIR(PAIR_RED));
    }
    else if (state.filtering)
    {
        mvprintw(
            row,
            0,
            "Filter: %s_",
            state.filterText.c_str());
    }
    else if (!state.statusMessage.empty())
    {
        mvprintw(row, 0, "%s", state.statusMessage.c_str());
    }
    else
    {
        mvprintw(
            row,
            0,
            "Sort: %-3s  Filter: %-15s  [up/down select  s sort  / filter  x kill  q quit]",
            state.sortMode == SortMode::Cpu ? "CPU" : "MEM",
            state.filterText.empty() ? "(none)" : state.filterText.c_str());
    }

    int headerRow = row + 1;

    mvprintw(
        headerRow,
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

    int listRow = headerRow + 1;

    int maxRows = std::max(0, height - listRow);

    // Keep the selection on screen by adjusting the scroll window.
    // selectedIndex is assumed already clamped to a valid index for
    // `processes` by the caller.
    if (maxRows > 0)
    {
        if (state.selectedIndex < state.scrollOffset)
        {
            state.scrollOffset = state.selectedIndex;
        }
        else if (state.selectedIndex >=
                 state.scrollOffset + static_cast<std::size_t>(maxRows))
        {
            state.scrollOffset =
                state.selectedIndex - static_cast<std::size_t>(maxRows) + 1;
        }
    }

    std::size_t count = 0;

    if (!processes.empty() && maxRows > 0)
    {
        count = std::min(
            processes.size() - state.scrollOffset,
            static_cast<std::size_t>(maxRows));
    }

    for (std::size_t i = 0; i < count; ++i)
    {
        std::size_t index = state.scrollOffset + i;

        const ProcessInfo& process = processes[index];

        bool selected = (index == state.selectedIndex);

        if (selected)
            attron(A_REVERSE);

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

        mvprintw(listRow + static_cast<int>(i), 0, "%s", prefix);

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
            command.resize(static_cast<std::size_t>(remaining));

        printw("%s", command.c_str());

        if (selected)
            attroff(A_REVERSE);
    }

    // Last (variable-height) section on screen - nothing else is
    // drawn below it, so there's no need to advance nextRow further.
}
