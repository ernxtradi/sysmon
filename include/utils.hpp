#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <cstdint>

namespace Utils {

std::string readFile(const std::string& path);

std::string trim(const std::string& text);

std::string bytesToHuman(uint64_t bytes);

std::string secondsToTime(uint64_t seconds);

std::string percentageBar(double value, unsigned int width = 20);

}

#endif