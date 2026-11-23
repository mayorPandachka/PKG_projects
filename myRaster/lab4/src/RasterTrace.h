#pragma once
#include <string>
#include <utility>
#include <vector>

// Сопроводительные вычисления: таблица промежуточных значений алгоритма.
// Не используется в бенчмарке; нужна, чтобы показать ход работы на одном примере.
struct TraceTable
{
    std::vector<std::string> header;
    std::vector<std::vector<std::string>> rows;
    std::vector<std::pair<int, int>> pixels;   // закрашенные пиксели (для самопроверки)
    bool truncated = false;                    // таблица обрезана по MaxRows
};

class RasterTrace
{
public:
    static constexpr size_t MaxRows = 2000;

    static TraceTable Step(int x0, int y0, int x1, int y1);
    static TraceTable DDA(int x0, int y0, int x1, int y1);
    static TraceTable Bresenham(int x0, int y0, int x1, int y1);
    static TraceTable Wu(double x0, double y0, double x1, double y1);
    static TraceTable Circle(int cx, int cy, int r);
};
