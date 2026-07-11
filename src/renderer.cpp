#include "renderer.hpp"
#include "utils.hpp"

Renderer::Renderer()
{
    width = 0;
    height = 0;
}

Renderer::~Renderer()
{
    end();
}

void Renderer::begin()
{
    initscr();

    noecho();

    cbreak();

    curs_set(0);

    keypad(stdscr, TRUE);

    timeout(0);

    start_color();

    use_default_colors();

    getmaxyx(stdscr, height, width);
}

void Renderer::end()
{
    endwin();
}

void Renderer::clear()
{
    erase();
}

void Renderer::refreshScreen()
{
    refresh();
}