#include "RasterEngine.h"
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <utility>

namespace {
inline int RoundHalfUp(double v) { return static_cast<int>(std::floor(v + 0.5)); }
inline double IPart(double v) { return std::floor(v); }
inline double FPart(double v) { return v - std::floor(v); }
inline double RFPart(double v) { return 1.0 - FPart(v); }
}

void RasterEngine::StepLine(int x0, int y0, int x1, int y1, IRasterTarget &t)
{
    const int dx = x1 - x0;
    const int dy = y1 - y0;
    if (dx == 0 && dy == 0) {
        t.SetPixel(x0, y0, 1.0);
        return;
    }
    if (std::abs(dx) >= std::abs(dy)) {
        //Палогая простая
        const double k = static_cast<double>(dy) / dx;
        const int sx = dx > 0 ? 1 : -1;
        for (int x = x0;; x += sx) {
            const double y = y0 + k * (x - x0);
            t.SetPixel(x, RoundHalfUp(y), 1.0);
            if (x == x1) break;
        }
    } else {
        //Крутая простая
        const double k = static_cast<double>(dx) / dy;
        const int sy = dy > 0 ? 1 : -1;
        for (int y = y0;; y += sy) {
            const double x = x0 + k * (y - y0);
            t.SetPixel(RoundHalfUp(x), y, 1.0);
            if (y == y1) break;
        }
    }
}

void RasterEngine::DDALine(int x0, int y0, int x1, int y1, IRasterTarget &t)
{
    const int dx = x1 - x0;
    const int dy = y1 - y0;
    const int len = std::max(std::abs(dx), std::abs(dy));
    if (len == 0) {
        t.SetPixel(x0, y0, 1.0);
        return;
    }
    const double xInc = static_cast<double>(dx) / len;
    const double yInc = static_cast<double>(dy) / len;
    double x = x0;
    double y = y0;
    for (int i = 0; i <= len; ++i) {
        t.SetPixel(RoundHalfUp(x), RoundHalfUp(y), 1.0);
        x += xInc;
        y += yInc;
    }
}

void RasterEngine::BresenhamLine(int x0, int y0, int x1, int y1, IRasterTarget &t)
{
    int dx = std::abs(x1 - x0);
    int dy = std::abs(y1 - y0);
    const int sx = x0 < x1 ? 1 : -1;
    const int sy = y0 < y1 ? 1 : -1;

    //азначэнне кіроўчац восі
    const bool steep = dy > dx;
    if (steep) std::swap(dx, dy);

    int e = 2 * dy - dx;
    int x = x0;
    int y = y0;
    for (int i = 0; i <= dx; ++i) {
        t.SetPixel(x, y, 1.0);
        if (e >= 0) {
            if (steep) x += sx; else y += sy;
            e -= 2 * dx;
        }
        if (steep) y += sy; else x += sx;
        e += 2 * dy;
    }
}

void RasterEngine::WuLine(double x0, double y0, double x1, double y1, IRasterTarget &t)
{
    const bool steep = std::fabs(y1 - y0) > std::fabs(x1 - x0);
    if (steep) { std::swap(x0, y0); std::swap(x1, y1); }
    if (x0 > x1) { std::swap(x0, x1); std::swap(y0, y1); }

    const double dx = x1 - x0;
    const double dy = y1 - y0;
    const double gradient = (dx == 0.0) ? 1.0 : dy / dx;

    auto plot = [&](double a, double b, double c) {
        if (steep) t.SetPixel(static_cast<int>(b), static_cast<int>(a), c);
        else       t.SetPixel(static_cast<int>(a), static_cast<int>(b), c);
    };

    double xend = std::floor(x0 + 0.5);
    double yend = y0 + gradient * (xend - x0);
    double xgap = RFPart(x0 + 0.5);
    const double xpxl1 = xend;
    const double ypxl1 = IPart(yend);
    plot(xpxl1, ypxl1,     RFPart(yend) * xgap);
    plot(xpxl1, ypxl1 + 1, FPart(yend)  * xgap);
    double intery = yend + gradient;

    xend = std::floor(x1 + 0.5);
    yend = y1 + gradient * (xend - x1);
    xgap = FPart(x1 + 0.5);
    const double xpxl2 = xend;
    const double ypxl2 = IPart(yend);
    plot(xpxl2, ypxl2,     RFPart(yend) * xgap);
    plot(xpxl2, ypxl2 + 1, FPart(yend)  * xgap);

    for (int x = static_cast<int>(xpxl1) + 1; x < static_cast<int>(xpxl2); ++x) {
        const double yi = IPart(intery);
        plot(x, yi,     RFPart(intery));
        plot(x, yi + 1, FPart(intery));
        intery += gradient;
    }
}

void RasterEngine::BresenhamCircle(int cx, int cy, int r, IRasterTarget &t)
{
    if (r < 0) return;
    int x = 0;
    int y = r;
    int d = 3 - 2 * r;
    while (x <= y) {
        t.SetPixel(cx + x, cy + y, 1.0);
        t.SetPixel(cx - x, cy + y, 1.0);
        t.SetPixel(cx + x, cy - y, 1.0);
        t.SetPixel(cx - x, cy - y, 1.0);
        t.SetPixel(cx + y, cy + x, 1.0);
        t.SetPixel(cx - y, cy + x, 1.0);
        t.SetPixel(cx + y, cy - x, 1.0);
        t.SetPixel(cx - y, cy - x, 1.0);
        if (d < 0) {
            d += 4 * x + 6;
        } else {
            d += 4 * (x - y) + 10;
            --y;
        }
        ++x;
    }
}
