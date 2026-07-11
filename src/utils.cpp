#include "utils.hpp"

#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>

namespace Utils {

std::string readFile(const std::string& path)
{
    std::ifstream file(path);

    if (!file.is_open())
        return "";

    std::stringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
}

std::string trim(const std::string& text)
{
    size_t start = text.find_first_not_of(" \t\r\n");

    if (start == std::string::npos)
        return "";

    size_t end = text.find_last_not_of(" \t\r\n");

    return text.substr(start, end - start + 1);
}

std::string bytesToHuman(uint64_t bytes)
{
    constexpr double KB = 1024.0;
    constexpr double MB = KB * 1024.0;
    constexpr double GB = MB * 1024.0;
    constexpr double TB = GB * 1024.0;

    std::ostringstream out;

    out << std::fixed << std::setprecision(2);

    if (bytes >= TB)
        out << bytes / TB << " TB";

    else if (bytes >= GB)
        out << bytes / GB << " GB";

    else if (bytes >= MB)
        out << bytes / MB << " MB";

    else if (bytes >= KB)
        out << bytes / KB << " KB";

    else
        out << bytes << " B";

    return out.str();
}

std::string secondsToTime(uint64_t seconds)
{
    uint64_t days = seconds / 86400;
    seconds %= 86400;

    uint64_t hours = seconds / 3600;
    seconds %= 3600;

    uint64_t minutes = seconds / 60;
    seconds %= 60;

    std::ostringstream out;

    if (days > 0)
        out << days << "d ";

    out << std::setw(2) << std::setfill('0') << hours << ":";
    out << std::setw(2) << std::setfill('0') << minutes << ":";
    out << std::setw(2) << std::setfill('0') << seconds;

    return out.str();
}

std::string percentageBar(double value, unsigned int width)
{
    value = std::clamp(value, 0.0, 100.0);

    unsigned int filled =
        static_cast<unsigned int>((value / 100.0) * width);

    std::string bar;

    for (unsigned int i = 0; i < width; ++i)
    {
        if (i < filled)
            bar += "█";
        else
            bar += "░";
    }

    return bar;
}

}