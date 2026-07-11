#include "cpu.hpp"

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

    return (100.0 * (deltaTotal - deltaIdle)) / deltaTotal;
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