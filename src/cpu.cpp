#include "cpu.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>

CPU::CPU()
    : previousIdle(0),
      previousTotal(0)
{
}

CPUInfo CPU::getInfo()
{
    CPUInfo info;

    info.usage = calculateUsage();
    info.temperature = getTemperature();
    info.frequency = getFrequency();
    info.cores = getCoreCount();
    info.perCoreUsage = calculatePerCoreUsage();

    return info;
}

double CPU::calculateUsage()
{
    std::ifstream file("/proc/stat");

    if (!file.is_open())
        return 0.0;

    std::string cpu;

    long long user;
    long long nice;
    long long system;
    long long idle;
    long long iowait;
    long long irq;
    long long softirq;
    long long steal;

    file >> cpu
         >> user
         >> nice
         >> system
         >> idle
         >> iowait
         >> irq
         >> softirq
         >> steal;

    long long idleTime = idle + iowait;

    long long totalTime =
        user +
        nice +
        system +
        idle +
        iowait +
        irq +
        softirq +
        steal;

    if (previousTotal == 0)
    {
        previousIdle = idleTime;
        previousTotal = totalTime;

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        return calculateUsage();
    }

    long long deltaIdle = idleTime - previousIdle;
    long long deltaTotal = totalTime - previousTotal;

    previousIdle = idleTime;
    previousTotal = totalTime;

    if (deltaTotal == 0)
        return 0.0;

    // /proc/stat's per-field jiffie counters aren't perfectly monotonic
    // under scheduling pressure (idle can occasionally tick backwards
    // between two close reads), which would otherwise produce a usage
    // figure outside the mathematically possible [0, 100] range.
    return std::clamp(
        (100.0 * (deltaTotal - deltaIdle)) / deltaTotal,
        0.0,
        100.0);
}

std::vector<double> CPU::calculatePerCoreUsage()
{
    std::ifstream file("/proc/stat");

    if (!file.is_open())
        return {};

    std::string line;

    // Skip the aggregate "cpu " line; per-core lines follow as
    // "cpu0", "cpu1", ... until the first non-cpu line (e.g. "intr").
    std::getline(file, line);

    std::vector<long long> idleTimes;
    std::vector<long long> totalTimes;

    while (std::getline(file, line))
    {
        if (line.rfind("cpu", 0) != 0 ||
            line.size() < 4 ||
            !std::isdigit(static_cast<unsigned char>(line[3])))
        {
            break;
        }

        std::istringstream ss(line);

        std::string label;

        long long user;
        long long nice;
        long long system;
        long long idle;
        long long iowait;
        long long irq;
        long long softirq;
        long long steal;

        ss >> label
           >> user
           >> nice
           >> system
           >> idle
           >> iowait
           >> irq
           >> softirq
           >> steal;

        idleTimes.push_back(idle + iowait);

        totalTimes.push_back(
            user + nice + system + idle + iowait + irq + softirq + steal);
    }

    // Core count changed (first call, or hotplugged CPUs) - record a
    // baseline and report 0% for this sample rather than a bogus delta.
    if (previousTotalPerCore.size() != totalTimes.size())
    {
        previousIdlePerCore = idleTimes;
        previousTotalPerCore = totalTimes;

        return std::vector<double>(totalTimes.size(), 0.0);
    }

    std::vector<double> usage(totalTimes.size(), 0.0);

    for (std::size_t i = 0; i < totalTimes.size(); ++i)
    {
        long long deltaIdle = idleTimes[i] - previousIdlePerCore[i];
        long long deltaTotal = totalTimes[i] - previousTotalPerCore[i];

        if (deltaTotal > 0)
        {
            usage[i] = std::clamp(
                (100.0 * (deltaTotal - deltaIdle)) / deltaTotal,
                0.0,
                100.0);
        }
    }

    previousIdlePerCore = idleTimes;
    previousTotalPerCore = totalTimes;

    return usage;
}

double CPU::getTemperature()
{
    std::ifstream file(
        "/sys/class/thermal/thermal_zone0/temp");

    if (!file.is_open())
        return 0.0;

    long temperature;

    file >> temperature;

    return temperature / 1000.0;
}

double CPU::getFrequency()
{
    std::ifstream file(
        "/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq");

    if (!file.is_open())
        return 0.0;

    long freq;

    file >> freq;

    return freq / 1000000.0;
}

unsigned int CPU::getCoreCount()
{
    return std::thread::hardware_concurrency();
}