#include "network.hpp"

#include <arpa/inet.h>
#include <chrono>
#include <cstring>
#include <fstream>
#include <ifaddrs.h>
#include <sstream>
#include <thread>

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

    if (interfaceName.empty())
        return info;

    std::ifstream file("/proc/net/dev");

    if (!file.is_open())
        return info;

    std::string line;

    while (std::getline(file, line))
    {
        if (line.find(interfaceName) == std::string::npos)
            continue;

        auto pos = line.find(':');

        if (pos == std::string::npos)
            continue;

        line = line.substr(pos + 1);

        std::stringstream ss(line);

        uint64_t rxBytes = 0;
        uint64_t txBytes = 0;

        ss >> rxBytes;

        for (int i = 0; i < 7; i++)
        {
            uint64_t ignore;
            ss >> ignore;
        }

        ss >> txBytes;

        if (ss.fail())
            continue;

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

        // Counters can go backwards if the interface is reset (down/up,
        // driver reload), which would otherwise underflow to a huge
        // value. Treat that as a fresh baseline instead.
        info.downloadSpeed =
            (rxBytes >= previousRx)
                ? static_cast<double>(rxBytes - previousRx)
                : 0.0;

        info.uploadSpeed =
            (txBytes >= previousTx)
                ? static_cast<double>(txBytes - previousTx)
                : 0.0;

        previousRx = rxBytes;
        previousTx = txBytes;

        break;
    }

    return info;
}

std::string Network::getIPAddress()
{
    const std::string fallback = "127.0.0.1";

    ifaddrs* addrs = nullptr;

    if (getifaddrs(&addrs) != 0)
        return fallback;

    std::string result = fallback;

    for (ifaddrs* addr = addrs; addr != nullptr; addr = addr->ifa_next)
    {
        if (addr->ifa_addr == nullptr)
            continue;

        if (addr->ifa_addr->sa_family != AF_INET)
            continue;

        if (!interfaceName.empty() &&
            interfaceName != addr->ifa_name)
            continue;

        char buffer[INET_ADDRSTRLEN];

        const auto* sin =
            reinterpret_cast<sockaddr_in*>(addr->ifa_addr);

        if (inet_ntop(AF_INET, &sin->sin_addr, buffer, sizeof(buffer)) !=
            nullptr)
        {
            result = buffer;
            break;
        }
    }

    freeifaddrs(addrs);

    return result;
}