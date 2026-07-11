#ifndef GRAPHS_HPP
#define GRAPHS_HPP

#include <deque>
#include <string>

class Graph
{
public:
    explicit Graph(std::size_t maxSamples = 60);

    void addSample(double value);

    std::string render() const;

    void clear();

    const std::deque<double>& getHistory() const;

private:
    std::deque<double> history;

    std::size_t maxSamples;
};

#endif