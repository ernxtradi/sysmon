#ifndef CPU_HPP
#define CPU_HPP

#include "types.hpp"

class CPU {
public:
    CPU();

    CPUInfo getInfo();

private:
    long long previousIdle;
    long long previousTotal;

    double calculateUsage();
    double getTemperature();
    double getFrequency();
    unsigned int getCoreCount();
};

#endif