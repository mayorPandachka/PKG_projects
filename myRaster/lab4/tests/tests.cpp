// Автотесты ядра: проверка корректности во всех 8 октантах и на осях.
#include "../src/RasterEngine.h"
#include "../src/RasterTrace.h"
#include "../src/VirtualGrid.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <set>
#include <vector>
#include <algorithm>
#include <utility>

static int g_fail = 0, g_checks = 0;
#define CHECK(cond, ...) do { ++g_checks; if (!(cond)) { ++g_fail; std::printf("FAIL: " __VA_ARGS__); std::printf("  [%s:%d]\n", __FILE__, __LINE__);} } while (0)

using PixSet = std::set<std::pair<int, int>>;

struct Collect : IRasterTarget {
    PixSet px;
    std::vector<std::pair<std::pair<int,int>, double>> raw;
    void SetPixel(int x, int y, double i) override { if (i > 0) px.insert({x, y}); raw.push_back({{x, y}, i}); }
};

typedef void (*LineFn)(int, int, int, int, IRasterTarget &);

// Отрезок: число пикселей, концы, отклонение от идеальной прямой <= 0.5 по ведомой оси, связность
static void CheckLine(const char *name, LineFn fn, int x0, int y0, int x1, int y1)
{
    Collect c;
    fn(x0, y0, x1, y1, c);
    const int dx = x1 - x0, dy = y1 - y0;
    const int n = std::max(std::abs(dx), std::abs(dy)) + 1;
    CHECK((int)c.px.size() == n, "%s (%d,%d)->(%d,%d): пикселей %zu, ожидалось %d", name, x0, y0, x1, y1, c.px.size(), n);
    CHECK(c.px.count({x0, y0}) && c.px.count({x1, y1}), "%s (%d,%d)->(%d,%d): концы не закрашены", name, x0, y0, x1, y1);
    const bool alongX = std::abs(dx) >= std::abs(dy);
    if (dx == 0 && dy == 0) return;
    for (auto &p : c.px) {
        double dev;
        if (alongX) { double ideal = y0 + (double)dy * (p.first - x0) / dx; dev = std::fabs(p.second - ideal); }
        else        { double ideal = x0 + (double)dx * (p.second - y0) / dy; dev = std::fabs(p.first - ideal); }
        CHECK(dev <= 0.5 + 1e-9, "%s (%d,%d)->(%d,%d): отклонение %.3f в (%d,%d)", name, x0, y0, x1, y1, dev, p.first, p.second);
    }
    // по одному пикселю на каждый шаг ведущей оси
    std::set<int> major;
    for (auto &p : c.px) major.insert(alongX ? p.first : p.second);
    CHECK((int)major.size() == n, "%s: не по одному пикселю на шаг ведущей оси", name);
}

int main()
{
    // 1. Все 8 октантов + оси + вырожденные случаи, с ненулевым началом
    struct { int dx, dy; } dirs[] = {
        {10, 3}, {3, 10}, {-3, 10}, {-10, 3}, {-10, -3}, {-3, -10}, {3, -10}, {10, -3},   // по одному в каждом октанте
        {10, 0}, {0, 10}, {-10, 0}, {0, -10}, {7, 7}, {-7, 7}, {-7, -7}, {7, -7}, {0, 0}, {1, 0}, {1, 1}, {100, 1}, {1, 100}, {-97, 41}, {41, -97}};
    for (auto d : dirs) {
        CheckLine("Bresenham", RasterEngine::BresenhamLine, 5, -4, 5 + d.dx, -4 + d.dy);
        CheckLine("DDA",       RasterEngine::DDALine,       5, -4, 5 + d.dx, -4 + d.dy);
        CheckLine("Step",      RasterEngine::StepLine,      5, -4, 5 + d.dx, -4 + d.dy);
    }
    // 2. Полный перебор: все направления в квадрате 17x17 - Брезенхем
    for (int dx = -8; dx <= 8; ++dx)
        for (int dy = -8; dy <= 8; ++dy)
            CheckLine("Bresenham", RasterEngine::BresenhamLine, 0, 0, dx, dy);

    // 3. Согласованность с трассировкой (таблицы вычислений)
    for (auto d : dirs) {
        int X = 5 + d.dx, Y = -4 + d.dy;
        { Collect c; RasterEngine::BresenhamLine(5, -4, X, Y, c); auto t = RasterTrace::Bresenham(5, -4, X, Y);
          CHECK(t.pixels.size() == c.raw.size(), "trace Bresenham size");
          for (size_t i = 0; i < c.raw.size() && i < t.pixels.size(); ++i) CHECK(c.raw[i].first == t.pixels[i], "trace Bresenham pixel %zu", i); }
        { Collect c; RasterEngine::DDALine(5, -4, X, Y, c); auto t = RasterTrace::DDA(5, -4, X, Y);
          CHECK(t.pixels.size() == c.raw.size(), "trace DDA size");
          for (size_t i = 0; i < c.raw.size() && i < t.pixels.size(); ++i) CHECK(c.raw[i].first == t.pixels[i], "trace DDA pixel %zu", i); }
        { Collect c; RasterEngine::StepLine(5, -4, X, Y, c); auto t = RasterTrace::Step(5, -4, X, Y);
          CHECK(t.pixels.size() == c.raw.size(), "trace Step size");
          for (size_t i = 0; i < c.raw.size() && i < t.pixels.size(); ++i) CHECK(c.raw[i].first == t.pixels[i], "trace Step pixel %zu", i); }
    }

    // 4. ЦДА и Брезенхем должны отличаться не более чем на 1 пиксель
    int diffTotal = 0, cases = 0;
    for (int dx = -30; dx <= 30; ++dx)
        for (int dy = -30; dy <= 30; ++dy) {
            Collect a, b; RasterEngine::DDALine(0, 0, dx, dy, a); RasterEngine::BresenhamLine(0, 0, dx, dy, b);
            ++cases;
            for (auto &p : a.px) if (!b.px.count(p)) { ++diffTotal; bool near = false; for (int ox = -1; ox <= 1; ++ox) for (int oy = -1; oy <= 1; ++oy) if (b.px.count({p.first + ox, p.second + oy})) near = true; CHECK(near, "DDA vs Bresenham"); }
        }
    std::printf("ЦДА vs Брезенхем: %d отрезков, отличающихся пикселей всего %d\n", cases, diffTotal);

    // 5. Окружность
    for (int r = 0; r <= 60; ++r) {
        Collect c; RasterEngine::BresenhamCircle(3, -2, r, c);
        for (auto &p : c.px) {
            double dist = std::hypot(p.first - 3, p.second + 2);
            CHECK(std::fabs(dist - r) <= 0.75, "circle r=%d: расстояние %.3f в (%d,%d)", r, dist, p.first, p.second);
            // симметрия
            CHECK(c.px.count({3 + (p.first - 3), -2 - (p.second + 2)}) && c.px.count({3 - (p.first - 3), -2 + (p.second + 2)}) &&
                  c.px.count({3 + (p.second + 2), -2 + (p.first - 3)}), "circle r=%d: нет симметрии", r);
        }
        CHECK(c.px.count({3 + r, -2}) && c.px.count({3 - r, -2}) && c.px.count({3, -2 + r}) && c.px.count({3, -2 - r}), "circle r=%d: нет крайних точек", r);
        auto t = RasterTrace::Circle(3, -2, r);
        PixSet ts(t.pixels.begin(), t.pixels.end());
        CHECK(ts == c.px, "trace circle r=%d", r);
    }

    // 6. Ву: сумма весов двух соседних пикселей в каждом столбце (строке) = 1
    for (int k = 0; k < 360; k += 7) {
        double a = k * 3.14159265358979 / 180.0, L = 40.0;
        double x1 = 3.3 + L * std::cos(a), y1 = -2.6 + L * std::sin(a);
        Collect c; RasterEngine::WuLine(3.3, -2.6, x1, y1, c);
        bool steep = std::fabs(y1 + 2.6) > std::fabs(x1 - 3.3);
        std::map<int, double> sum;
        for (auto &r : c.raw) { CHECK(r.second >= -1e-12 && r.second <= 1.0 + 1e-12, "Wu: интенсивность вне [0,1]: %f", r.second); sum[steep ? r.first.second : r.first.first] += r.second; }
        int lo = (int)std::floor(std::min(steep ? -2.6 : 3.3, steep ? y1 : x1)) + 2, hi = (int)std::floor(std::max(steep ? -2.6 : 3.3, steep ? y1 : x1)) - 2;
        for (int m = lo; m <= hi; ++m) CHECK(std::fabs(sum[m] - 1.0) < 1e-9, "Wu angle %d: сумма в позиции %d = %f", k, m, sum[m]);
    }
    // Ву: явный пример - точное значение дробной части
    { Collect c; RasterEngine::WuLine(0, 0, 10, 4, c); // y = 0.4x
      VirtualGrid g(-2, 12, -2, 12); RasterEngine::WuLine(0, 0, 10, 4, g);
      CHECK(std::fabs(g.Get(5, 2) - 1.0) < 1e-6 && g.Get(5, 3) < 1e-6, "Wu x=5: y=2.0 -> (5;2)=1.0, (5;3)=0 (%f %f)", g.Get(5,2), g.Get(5,3));
      CHECK(std::fabs(g.Get(3, 1) - 0.8) < 1e-6 && std::fabs(g.Get(3, 2) - 0.2) < 1e-6, "Wu x=3: y=1.2 -> 0.8/0.2 (%f %f)", g.Get(3,1), g.Get(3,2)); }

    // 7. VirtualGrid
    { VirtualGrid g(-5, 5, -5, 5); g.SetPixel(0, 0, 1.0); g.SetPixel(6, 0, 1.0); g.SetPixel(-5, 5, 0.5); g.SetPixel(-5, 5, 0.3);
      CHECK(g.CountLit() == 2 && g.Get(-5, 5) == 0.5f && g.WriteCount() == 3, "VirtualGrid"); g.Clear(); CHECK(g.CountLit() == 0 && g.WriteCount() == 0, "Clear"); }

    std::printf("Проверок: %d, провалено: %d\n", g_checks, g_fail);
    return g_fail ? 1 : 0;
}
