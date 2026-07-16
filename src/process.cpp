#include "process.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <pwd.h>
#include <signal.h>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;

namespace
{

bool isNumeric(const std::string& text)
{
    return !text.empty() &&
           std::all_of(
               text.begin(),
               text.end(),
               [](unsigned char c)
               {
                   return std::isdigit(c);
               });
}

std::string readFirstLine(const std::string& path)
{
    std::ifstream file(path);

    if (!file.is_open())
        return "";

    std::string line;

    std::getline(file, line);

    return line;
}

std::string trim(const std::string& text)
{
    const auto first =
        text.find_first_not_of(" \t\r\n");

    if (first == std::string::npos)
        return "";

    const auto last =
        text.find_last_not_of(" \t\r\n");

    return text.substr(
        first,
        last - first + 1);
}

std::string uidToUser(uid_t uid)
{
    passwd* pw = getpwuid(uid);

    if (pw == nullptr)
        return "unknown";

    return pw->pw_name;
}

} // anonymous namespace

ProcessInfo ProcessManager::readProcess(int pid, uint64_t currentSystemCpuTime)
{
    ProcessInfo info;

    info.pid = pid;

    const std::string base =
        "/proc/" + std::to_string(pid);

    //-----------------------------------------------------
    // Process name
    //-----------------------------------------------------

    info.name =
        trim(readFirstLine(base + "/comm"));

    if (info.name.empty())
        return {};

    //-----------------------------------------------------
    // Status (state, memory, user)
    //-----------------------------------------------------

    std::ifstream status(base + "/status");

    if (!status.is_open())
        return info;

    std::string key;

    uid_t uid = 0;

    while (status >> key)
    {
        if (key == "State:")
        {
            status >> info.state;
        }
        else if (key == "VmRSS:")
        {
            status >> info.memoryBytes;

            std::string unit;
            status >> unit;

            info.memoryBytes *= 1024;
        }
        else if (key == "Uid:")
        {
            status >> uid;

            info.user = uidToUser(uid);
        }
        else if (key == "PPid:")
        {
            status >> info.parentPid;
        }
        else if (key == "Threads:")
        {
            status >> info.threads;
        }
        else if (key == "VmSize:")
        {
            status >> info.virtualMemory;

            std::string unit;
            status >> unit;

            info.virtualMemory *= 1024;
        }

        status.ignore(
            std::numeric_limits<std::streamsize>::max(),
            '\n');
    }

    //-----------------------------------------------------
    // Command line
    //
    // Args in /proc/[pid]/cmdline are NUL-separated. Kernel
    // threads have an empty cmdline, so fall back to the
    // "[name]" convention used by ps/top.
    //-----------------------------------------------------

    {
        std::ifstream cmdline(base + "/cmdline");

        std::string raw(
            (std::istreambuf_iterator<char>(cmdline)),
            std::istreambuf_iterator<char>());

        for (char& c : raw)
            if (c == '\0')
                c = ' ';

        info.command = trim(raw);

        if (info.command.empty())
            info.command = "[" + info.name + "]";
    }

    //-----------------------------------------------------
    // CPU usage
    //
    // /proc/[pid]/stat's 2nd field (comm) is parenthesized
    // and may itself contain spaces, so fields can't be
    // located by naively splitting on whitespace. Split on
    // the last ')' instead - everything after it is
    // whitespace-delimited and starts with the state field.
    //-----------------------------------------------------

    std::string statLine = readFirstLine(base + "/stat");

    auto closeParen = statLine.find_last_of(')');

    if (closeParen != std::string::npos)
    {
        std::istringstream ss(statLine.substr(closeParen + 1));

        std::vector<std::string> fields;
        std::string field;

        while (ss >> field)
            fields.push_back(field);

        // fields[11] = utime, fields[12] = stime
        if (fields.size() > 12)
        {
            uint64_t utime = std::stoull(fields[11]);
            uint64_t stime = std::stoull(fields[12]);

            uint64_t totalProcessTicks = utime + stime;

            auto it = previousProcessTicks.find(pid);

            // A pid can be reused for an unrelated process between
            // scans, in which case its recorded ticks may exceed the
            // new process's ticks. Only compute a delta when ticks
            // moved forward, otherwise treat it as a fresh baseline.
            if (it != previousProcessTicks.end() &&
                currentSystemCpuTime > previousSystemCpuTime &&
                totalProcessTicks >= it->second)
            {
                uint64_t processDelta =
                    totalProcessTicks - it->second;

                uint64_t systemDelta =
                    currentSystemCpuTime - previousSystemCpuTime;

                info.cpuUsage =
                    (100.0 * static_cast<double>(processDelta)) /
                    static_cast<double>(systemDelta);
            }

            previousProcessTicks[pid] = totalProcessTicks;
        }
    }

    return info;
}

uint64_t ProcessManager::getTotalSystemCpuTime()
{
    std::ifstream file("/proc/stat");

    if (!file.is_open())
        return 0;

    std::string cpu;

    uint64_t user;
    uint64_t nice;
    uint64_t system;
    uint64_t idle;
    uint64_t iowait;
    uint64_t irq;
    uint64_t softirq;
    uint64_t steal;

    file >> cpu
         >> user
         >> nice
         >> system
         >> idle
         >> iowait
         >> irq
         >> softirq
         >> steal;

    return user +
           nice +
           system +
           idle +
           iowait +
           irq +
           softirq +
           steal;
}

std::vector<ProcessInfo> ProcessManager::getProcesses()
{
    std::vector<ProcessInfo> processes;

    if (!fs::exists("/proc"))
        return processes;

    uint64_t currentSystemCpuTime = getTotalSystemCpuTime();

    std::unordered_map<int, uint64_t> seenTicks;

    for (const auto& entry : fs::directory_iterator("/proc"))
    {
        if (!entry.is_directory())
            continue;

        const std::string pidString =
            entry.path().filename().string();

        if (!isNumeric(pidString))
            continue;

        int pid = 0;

        try
        {
            pid = std::stoi(pidString);
        }
        catch (...)
        {
            continue;
        }

        try
        {
            ProcessInfo process =
                readProcess(pid, currentSystemCpuTime);

            if (!process.name.empty())
            {
                processes.push_back(process);

                auto it = previousProcessTicks.find(pid);

                if (it != previousProcessTicks.end())
                    seenTicks[pid] = it->second;
            }
        }
        catch (...)
        {
            // Ignore processes that terminate while scanning.
        }
    }

    // Drop bookkeeping for processes that no longer exist so
    // the map doesn't grow without bound over a long runtime.
    previousProcessTicks = std::move(seenTicks);

    previousSystemCpuTime = currentSystemCpuTime;

    return processes;
}

std::vector<ProcessInfo> ProcessManager::topMemory(unsigned int limit)
{
    auto list = getProcesses();

    std::sort(
        list.begin(),
        list.end(),
        [](const ProcessInfo& a,
           const ProcessInfo& b)
        {
            return a.memoryBytes >
                   b.memoryBytes;
        });

    if (list.size() > limit)
        list.resize(limit);

    return list;
}

bool ProcessManager::killProcess(int pid)
{
    return kill(pid, SIGTERM) == 0;
}
