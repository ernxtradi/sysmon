#include "graphs.hpp"

#include <algorithm>

Graph::Graph(std::size_t maxSamples)
    : maxSamples(maxSamples)
{
}

void Graph::addSample(double value)
{
    value = std::clamp(value, 0.0, 100.0);

    history.push_back(value);

    if (history.size() > maxSamples)
        history.pop_front();
}

std::string Graph::render() const
{
    static const std::string levels[] =
    {
        "▁",
        "▂",
        "▃",
        "▄",
        "▅",
        "▆",
        "▇",
        "█"
    };

    std::string graph;

    for (double value : history)
    {
        int index = static_cast<int>((value / 100.0) * 7.0);

        index = std::clamp(index, 0, 7);

        graph += levels[index];
    }

    return graph;
}

void Graph::clear()
{
    history.clear();
}

const std::deque<double>& Graph::getHistory() const
{
    return history;
}