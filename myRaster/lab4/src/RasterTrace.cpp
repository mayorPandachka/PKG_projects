#include "RasterTrace.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace {
std::string F(double v, int prec = 3)
{
    char buf[64];
    std::snprintf(buf, sizeof buf, "%.*f", prec, v);
    std::string s(buf);
    if (s == "-0.000" || s == "-0.00" || s == "-0.0") s.erase(0, 1);
    return s;
}
std::string I(long long v) { return std::to_string(v); }
std::string P(int x, int y) { return "(" + std::to_string(x) + "; " + std::to_string(y) + ")"; }
int R(double v) { return static_cast<int>(std::floor(v + 0.5)); }
bool Full(const TraceTable &t) { return t.rows.size() >= RasterTrace::MaxRows; }
}

TraceTable RasterTrace::Step(int x0, int y0, int x1, int y1)
{
    TraceTable t;
    const int dx = x1 - x0, dy = y1 - y0;
    if (dx == 0 && dy == 0) {
        t.header = {"i", "x", "y"};
        t.rows.push_back({"0", I(x0), I(y0)});
        t.pixels.push_back({x0, y0});
        return t;
    }
    const bool alongX = std::abs(dx) >= std::abs(dy);
    const double k = alongX ? double(dy) / dx : double(dx) / dy;
    t.header = alongX ? std::vector<std::string>{"i", "x", "y = y0 + k*(x - x0)", "y акругл.", "Піксель"}
                      : std::vector<std::string>{"i", "y", "x = x0 + k*(y - y0)", "x акругл.", "Піксель"};
    const int n = std::max(std::abs(dx), std::abs(dy));
    const int s = alongX ? (dx > 0 ? 1 : -1) : (dy > 0 ? 1 : -1);
    for (int i = 0; i <= n; ++i) {
        if (Full(t)) { t.truncated = true; break; }
        if (alongX) {
            const int x = x0 + s * i;
            const double y = y0 + k * (x - x0);
            t.rows.push_back({I(i), I(x), F(y), I(R(y)), P(x, R(y))});
            t.pixels.push_back({x, R(y)});
        } else {
            const int y = y0 + s * i;
            const double x = x0 + k * (y - y0);
            t.rows.push_back({I(i), I(y), F(x), I(R(x)), P(R(x), y)});
            t.pixels.push_back({R(x), y});
        }
    }
    return t;
}

TraceTable RasterTrace::DDA(int x0, int y0, int x1, int y1)
{
    TraceTable t;
    t.header = {"i", "x (дакладнае)", "y (дакладнае)", "Піксель"};
    const int dx = x1 - x0, dy = y1 - y0;
    const int len = std::max(std::abs(dx), std::abs(dy));
    if (len == 0) {
        t.rows.push_back({"0", I(x0), I(y0), P(x0, y0)});
        t.pixels.push_back({x0, y0});
        return t;
    }
    const double xi = double(dx) / len, yi = double(dy) / len;
    double x = x0, y = y0;
    for (int i = 0; i <= len; ++i) {
        if (Full(t)) { t.truncated = true; break; }
        t.rows.push_back({I(i), F(x), F(y), P(R(x), R(y))});
        t.pixels.push_back({R(x), R(y)});
        x += xi;
        y += yi;
    }
    return t;
}

TraceTable RasterTrace::Bresenham(int x0, int y0, int x1, int y1)
{
    TraceTable t;
    int dx = std::abs(x1 - x0), dy = std::abs(y1 - y0);
    const int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
    const bool steep = dy > dx;
    if (steep) std::swap(dx, dy);
    t.header = {"i", "Піксель", "e (да)", "e >= 0 ?", "Шаг", "e (пасля)"};
    int e = 2 * dy - dx, x = x0, y = y0;
    for (int i = 0; i <= dx; ++i) {
        if (Full(t)) { t.truncated = true; break; }
        t.pixels.push_back({x, y});
        const int before = e;
        std::string step;
        if (e >= 0) {
            if (steep) x += sx; else y += sy;
            e -= 2 * dx;
            step = steep ? "па кіроўчай Y і па X" : "па кіроўчай X і па Y";
        } else {
            step = steep ? "толькі па Y" : "толькі па X";
        }
        const int px = t.pixels.back().first, py = t.pixels.back().second;
        if (steep) y += sy; else x += sx;
        e += 2 * dy;
        t.rows.push_back({I(i), P(px, py), I(before), before >= 0 ? "так" : "не", step, I(e)});
    }
    return t;
}

TraceTable RasterTrace::Wu(double x0, double y0, double x1, double y1)
{
    TraceTable t;
    t.header = {"x", "y (дакладнае)", "ipart(y)", "Піксель 1: 1-fpart", "Піксель 2: fpart"};
    const bool steep = std::fabs(y1 - y0) > std::fabs(x1 - x0);
    if (steep) { std::swap(x0, y0); std::swap(x1, y1); }
    if (x0 > x1) { std::swap(x0, x1); std::swap(y0, y1); }
    const double dx = x1 - x0, dy = y1 - y0;
    const double g = dx == 0.0 ? 1.0 : dy / dx;
    auto px = [&](double a, double b) { return steep ? P(int(b), int(a)) : P(int(a), int(b)); };
    auto fp = [](double v) { return v - std::floor(v); };

    double xend = std::floor(x0 + 0.5), yend = y0 + g * (xend - x0), gap = 1.0 - fp(x0 + 0.5);
    double yi = std::floor(yend);
    t.rows.push_back({F(xend, 0) + " (канец 1)", F(yend), F(yi, 0),
                      px(xend, yi) + " : " + F((1 - fp(yend)) * gap), px(xend, yi + 1) + " : " + F(fp(yend) * gap)});
    double inter = yend + g;
    const double xe1 = xend;
    xend = std::floor(x1 + 0.5);
    const double yend2 = y1 + g * (xend - x1), gap2 = fp(x1 + 0.5);
    for (int x = int(xe1) + 1; x < int(xend); ++x) {
        if (Full(t)) { t.truncated = true; break; }
        yi = std::floor(inter);
        t.rows.push_back({I(x), F(inter), F(yi, 0),
                          px(x, yi) + " : " + F(1 - fp(inter)), px(x, yi + 1) + " : " + F(fp(inter))});
        inter += g;
    }
    yi = std::floor(yend2);
    t.rows.push_back({F(xend, 0) + " (канец 2)", F(yend2), F(yi, 0),
                      px(xend, yi) + " : " + F((1 - fp(yend2)) * gap2), px(xend, yi + 1) + " : " + F(fp(yend2) * gap2)});
    return t;
}

TraceTable RasterTrace::Circle(int cx, int cy, int r)
{
    TraceTable t;
    t.header = {"i", "(x; y) у актанце", "d (да)", "d < 0 ?", "Новае d", "Сіметрычныя пункты"};
    if (r < 0) return t;
    int x = 0, y = r, d = 3 - 2 * r, i = 0;
    while (x <= y) {
        if (Full(t)) { t.truncated = true; break; }
        const int bd = d;
        if (d < 0) d += 4 * x + 6;
        else { d += 4 * (x - y) + 10; }
        std::string sym = P(cx + x, cy + y) + " " + P(cx - x, cy + y) + " " + P(cx + x, cy - y) + " " + P(cx - x, cy - y)
                        + " " + P(cx + y, cy + x) + " " + P(cx - y, cy + x) + " " + P(cx + y, cy - x) + " " + P(cx - y, cy - x);
        const int pts[8][2] = {{cx + x, cy + y}, {cx - x, cy + y}, {cx + x, cy - y}, {cx - x, cy - y},
                               {cx + y, cy + x}, {cx - y, cy + x}, {cx + y, cy - x}, {cx - y, cy - x}};
        for (auto &p : pts) t.pixels.push_back({p[0], p[1]});
        t.rows.push_back({I(i), P(x, y), I(bd), bd < 0 ? "так (y не змяняецца)" : "не (y = y - 1)", I(d), sym});
        if (bd >= 0) --y;
        ++x;
        ++i;
    }
    return t;
}
