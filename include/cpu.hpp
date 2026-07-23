#ifndef CPU_HPP
#define CPU_HPP

#include "types.hpp"

#include <vector>

class CPU {
public:
    CPU();

    CPUInfo getInfo();

private:
    long long previousIdle;
    long long previousTotal;

    std::vector<long long> previousIdlePerCore;
    std::vector<long long> previousTotalPerCore;

    double calculateUsage();
    std::vector<double> calculatePerCoreUsage();
    double getTemperature();
    double getFrequency();
    unsigned int getCoreCount();
};

#endif