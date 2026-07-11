#include "network.hpp"

#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>

Network::Network()
    : previousRx(0),
      previousTx(0)
{
    detectInterface();
}

void Network::detectInterface()
{
    std::ifstream file("/proc/net/dev");

    std::string line;

    while (std::getline(file, line))
    {
        if (line.find("lo:") != std::string::npos)
            continue;

        auto pos = line.find(':');

        if (pos == std::string::npos)
            continue;

        interfaceName = line.substr(0, pos);

        interfaceName.erase(
            0,
            interfaceName.find_first_not_of(" ")
        );

        interfaceName.erase(
            interfaceName.find_last_not_of(" ") + 1
        );

        return;
    }
}

NetworkInfo Network::getInfo()
{
    NetworkInfo info;

    info.ipAddress = getIPAddress();

    std::ifstream file("/proc/net/dev");

    if (!file.is_open())
        return info;

    std::string line;

    while (std::getline(file, line))
    {
        if (line.find(interfaceName) == std::string::npos)
            continue;

        auto pos = line.find(':');

        line = line.substr(pos + 1);

        std::stringstream ss(line);

        uint64_t rxBytes;
        uint64_t txBytes;

        ss >> rxBytes;

        for (int i = 0; i < 7; i++)
        {
            uint64_t ignore;
            ss >> ignore;
        }

        ss >> txBytes;

        if (previousRx == 0)
        {
            previousRx = rxBytes;
            previousTx = txBytes;

            std::this_thread::sleep_for(
                std::chrono::seconds(1));

            return getInfo();
        }

        info.bytesReceived = rxBytes;
        info.bytesSent = txBytes;

        info.downloadSpeed =
            static_cast<double>(rxBytes - previousRx);

        info.uploadSpeed =
            static_cast<double>(txBytes - previousTx);

        previousRx = rxBytes;
        previousTx = txBytes;

        break;
    }

    return info;
}

std::string Network::getIPAddress()
{
    return "127.0.0.1";
}