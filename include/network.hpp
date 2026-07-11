#ifndef NETWORK_HPP
#define NETWORK_HPP

#include "types.hpp"
#include <string>

class Network {
public:
    Network();

    NetworkInfo getInfo();

private:
    uint64_t previousRx;
    uint64_t previousTx;

    std::string getIPAddress();
};

#endif