#include "system.hpp"

#include "utils.hpp"

#include <fstream>
#include <sstream>
#include <string>
#include <unistd.h>

SystemInfo System::getInfo()
{
    SystemInfo info;

    //-----------------------------
    // Hostname
    //-----------------------------

    char hostname[256];

    if (gethostname(hostname, sizeof(hostname)) == 0)
        info.hostname = hostname;
    else
        info.hostname = "Unknown";

    //-----------------------------
    // Kernel Version
    //-----------------------------

    {
        std::ifstream file("/proc/version");

        if (file.is_open())
        {
            std::getline(file, info.kernel);
        }
    }

    //-----------------------------
    // Operating System
    //-----------------------------

    {
        std::ifstream file("/etc/os-release");

        std::string line;

        while (std::getline(file, line))
        {
            if (line.rfind("PRETTY_NAME=", 0) == 0)
            {
                info.osName =
                    line.substr(12);

                if (!info.osName.empty() &&
                    info.osName.front() == '"' &&
                    info.osName.back() == '"')
                {
                    info.osName.pop_back();
                    info.osName.erase(0, 1);
                }

                break;
            }
        }
    }

    //-----------------------------
    // Uptime
    //-----------------------------

    {
        std::ifstream file("/proc/uptime");

        if (file.is_open())
        {
            double uptime;

            file >> uptime;

            info.uptime =
                Utils::secondsToTime(
                    static_cast<uint64_t>(uptime));
        }
    }

    return info;
}