#include "process.hpp"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <pwd.h>
#include <signal.h>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <unistd.h>

namespace fs = std::filesystem;

namespace
{

/*
 * Returns true if a string contains only digits.
 */
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

/*
 * Reads an entire file into a string.
 */
std::string readFile(const std::string& path)
{
    std::ifstream file(path);

    if (!file.is_open())
        return "";

    std::stringstream buffer;

    buffer << file.rdbuf();

    return buffer.str();
}

/*
 * Reads the first line from a file.
 */
std::string readFirstLine(const std::string& path)
{
    std::ifstream file(path);

    if (!file.is_open())
        return "";

    std::string line;

    std::getline(file, line);

    return line;
}

/*
 * Remove leading and trailing whitespace.
 */
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

/*
 * Convert UID to username.
 */
std::string uidToUser(uid_t uid)
{
    passwd* pw = getpwuid(uid);

    if (pw == nullptr)
        return "unknown";

    return pw->pw_name;
}

/*
 * Read system uptime in seconds.
 */
long long getSystemUptime()
{
    std::ifstream file("/proc/uptime");

    if (!file.is_open())
        return 0;

    double uptime = 0;

    file >> uptime;

    return static_cast<long long>(uptime);
}

} // anonymous namespace

/*
 * Constructor
 */
ProcessManager::ProcessManager()
{
}

/*
 * Destructor
 */
ProcessManager::~ProcessManager()
{
}

/*
 * Scan the /proc filesystem and collect
 * information about every running process.
 */
std::vector<ProcessInfo> ProcessManager::getProcesses()
{
    std::vector<ProcessInfo> processes;

    if (!fs::exists("/proc"))
        return processes;

    for (const auto& entry : fs::directory_iterator("/proc"))
    {
        // Ignore non-directories
        if (!entry.is_directory())
            continue;

        // Directory name (PID)
        const std::string pidString =
            entry.path().filename().string();

        // Ignore non-numeric entries
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
                readProcess(pid);

            /*
             * If readProcess() failed,
             * it returns an empty process.
             */
            if (!process.name.empty())
                processes.push_back(process);
        }
        catch (...)
        {
            /*
             * Ignore processes that terminate
             * while we're scanning.
             */
        }
    }

    return processes;
}

struct ProcessInfo
{
    int pid = 0;
    int parentPid = 0;

    std::string name;
    std::string user;
    std::string state;

    std::string command;
    std::string executable;

    uint64_t memoryBytes = 0;
    uint64_t virtualMemory = 0;

    unsigned int threads = 0;

    double memoryUsage = 0.0;
    double cpuUsage = 0.0;
};

ProcessInfo ProcessManager::readProcess(int pid)
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
    // Command line
    //-----------------------------------------------------

    std::ifstream cmd(base + "/cmdline");

    if (cmd.is_open())
    {
        std::stringstream ss;

        char ch;

        while (cmd.get(ch))
        {
            if (ch == '\0')
                ss << ' ';
            else
                ss << ch;
        }

        info.command = trim(ss.str());
    }

    //-----------------------------------------------------
    // Executable path
    //-----------------------------------------------------

    try
    {
        info.executable =
            fs::read_symlink(base + "/exe").string();
    }
    catch (...)
    {
    }

    //-----------------------------------------------------
    // Status
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

        else if (key == "VmSize:")
        {
            status >> info.virtualMemory;

            std::string unit;

            status >> unit;

            info.virtualMemory *= 1024;
        }

        else if (key == "Threads:")
        {
            status >> info.threads;
        }

        else if (key == "PPid:")
        {
            status >> info.parentPid;
        }

        else if (key == "Uid:")
        {
            status >> uid;

            info.user =
                uidToUser(uid);
        }
        else
        {
            status.ignore(
                std::numeric_limits<
                    std::streamsize>::max(),
                '\n');
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

//-----------------------------------------------------
// CPU Usage
//-----------------------------------------------------

std::ifstream stat(base + "/stat");

if (stat.is_open())
{
    std::string line;

    std::getline(stat, line);

    std::stringstream ss(line);

    std::vector<std::string> fields;

    std::string field;

    while (ss >> field)
        fields.push_back(field);

    if (fields.size() > 15)
    {
        uint64_t utime =
            std::stoull(fields[13]);

        uint64_t stime =
            std::stoull(fields[14]);

        uint64_t totalProcessCpu =
            utime + stime;

        uint64_t totalSystemCpu =
            getTotalSystemCpuTime();

        auto it =
            cpuHistory.find(pid);

        if (it != cpuHistory.end())
        {
            uint64_t processDelta =
                totalProcessCpu -
                it->second.totalT#include "process.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <pwd.h>
#include <signal.h>
#include <sstream>
#include <string>
#include <unistd.h>

namespace fs = std::filesystem;

ProcessInfo ProcessManager::readProcess(int pid)
{
    ProcessInfo info;
    info.pid = pid;

    std::string path = "/proc/" + std::to_string(pid);

    //----------------------------
    // Process name
    //----------------------------

    std::ifstream comm(path + "/comm");

    if (!comm.is_open())
        return {};

    std::getline(comm, info.name);

    //----------------------------
    // Process status
    //----------------------------

    std::ifstream status(path + "/status");

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

            passwd* pw = getpwuid(uid);

            if (pw)
                info.user = pw->pw_name;
        }

        status.ignore(10000, '\n');
    }

    return info;
}

std::vector<ProcessInfo> ProcessManager::getProcesses()
{
    std::vector<ProcessInfo> processes;

    for (const auto& entry : fs::directory_iterator("/proc"))
    {
        if (!entry.is_directory())
            continue;

        std::string name =
            entry.path().filename().string();

        if (name.empty())
            continue;

        if (!std::isdigit(name[0]))
            continue;

        try
        {
            int pid = std::stoi(name);

            ProcessInfo process =
                readProcess(pid);

            if (!process.name.empty())
                processes.push_back(process);
        }
        catch (...)
        {
            // Ignore invalid or exited processes
        }
    }

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
}icks;

            uint64_t systemDelta =
                totalSystemCpu -
                previousSystemCpu;

            if (systemDelta > 0)
            {
                info.cpuUsage =
                    (100.0 *
                     processDelta) /
                    systemDelta;
            }
        }

        cpuHistory[pid].totalTicks =
            totalProcessCpu;

        previousSystemCpu =
            totalSystemCpu;
    }
}