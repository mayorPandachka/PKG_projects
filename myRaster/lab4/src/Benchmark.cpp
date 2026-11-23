#include "Benchmark.h"
#include "RasterEngine.h"
#include "VirtualGrid.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <limits>
#include <random>

namespace {

struct Seg { int x1, y1; double fx1, fy1; };
struct Circ { int r; };

std::vector<Seg> MakeFan(int n, int len, std::mt19937 &rng)
{
    std::uniform_real_distribution<double> ang(0.0, 6.283185307179586);
    std::vector<Seg> v;
    v.reserve(n);
    for (int i = 0; i < n; ++i) {
        const double a = ang(rng);
        Seg s;
        s.fx1 = len * std::cos(a);
        s.fy1 = len * std::sin(a);
        s.x1 = static_cast<int>(std::lround(s.fx1));
        s.y1 = static_cast<int>(std::lround(s.fy1));
        v.push_back(s);
    }
    return v;
}

enum LineAlgo { A_STEP, A_DDA, A_BRES, A_WU, A_COUNT };
const char *kNames[A_COUNT] = {"Пашагавы", "ЦДА", "Брэзенхэм", "Ву"};

double Now()
{
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

volatile double g_sink = 0.0;

double TimeLines(LineAlgo a, VirtualGrid &g, const std::vector<Seg> &segs, int n, int repeats)
{
    double best = std::numeric_limits<double>::max();
    for (int r = 0; r < repeats; ++r) {
        g.Clear();
        const double t0 = Now();
        switch (a) {
        case A_STEP: for (int i = 0; i < n; ++i) RasterEngine::StepLine(0, 0, segs[i].x1, segs[i].y1, g); break;
        case A_DDA:  for (int i = 0; i < n; ++i) RasterEngine::DDALine(0, 0, segs[i].x1, segs[i].y1, g); break;
        case A_BRES: for (int i = 0; i < n; ++i) RasterEngine::BresenhamLine(0, 0, segs[i].x1, segs[i].y1, g); break;
        case A_WU:   for (int i = 0; i < n; ++i) RasterEngine::WuLine(0.0, 0.0, segs[i].fx1, segs[i].fy1, g); break;
        default: break;
        }
        const double t1 = Now();
        g_sink = g_sink + static_cast<double>(g.WriteCount());
        best = std::min(best, t1 - t0);
    }
    return best;
}

double TimeCircles(VirtualGrid &g, const std::vector<Circ> &c, int n, int repeats)
{
    double best = std::numeric_limits<double>::max();
    for (int r = 0; r < repeats; ++r) {
        g.Clear();
        const double t0 = Now();
        for (int i = 0; i < n; ++i) RasterEngine::BresenhamCircle(0, 0, c[i].r, g);
        const double t1 = Now();
        g_sink = g_sink + static_cast<double>(g.WriteCount());
        best = std::min(best, t1 - t0);
    }
    return best;
}

}

std::vector<BenchTable> RunBenchmark(const BenchConfig &cfg, const BenchProgress &progress, bool *completed)
{
    std::vector<BenchTable> out;
    if (completed) *completed = false;
    std::mt19937 rng(cfg.seed);

    const int half = std::max({cfg.lineLength, cfg.maxLength, cfg.maxRadius}) + 2;
    VirtualGrid grid(-half, half, -half, half);

    const int steps = 10;
    const int total = steps * (A_COUNT + 1) + steps * (A_COUNT + 1) + steps * 2 + steps * 2;   // с запасом, для прогресса
    int done = 0;
    auto report = [&](const std::string &msg) {
        ++done;
        return !progress || progress(done, total, msg);
    };

    //Час ад колькасці адрэзкаў
    {
        BenchTable t;
        t.id = "lines_count";
        t.title = "Адрэзкі: час ад колькасці адрэзкаў (даўжыня " + std::to_string(cfg.lineLength) + " пікс.)";
        t.xLabel = "Колькасць адрэзкаў";
        const auto fan = MakeFan(cfg.maxLines, cfg.lineLength, rng);
        for (int s = 1; s <= steps; ++s) t.x.push_back(std::max(1, cfg.maxLines * s / steps));
        for (int a = 0; a < A_COUNT; ++a) {
            BenchSeries ser;
            ser.name = kNames[a];
            for (double nx : t.x) {
                ser.seconds.push_back(TimeLines(static_cast<LineAlgo>(a), grid, fan, static_cast<int>(nx), cfg.repeats));
                if (!report(std::string(kNames[a]) + ": " + std::to_string(static_cast<int>(nx)) + " адрэзкаў")) { t.series.push_back(ser); out.push_back(t); return out; }
            }
            t.series.push_back(ser);
        }
        out.push_back(t);
    }

    //Час ад даўжыні адрэзка
    {
        BenchTable t;
        t.id = "lines_length";
        t.title = "Адрэзкі: час ад даўжыні адрэзка (" + std::to_string(cfg.linesForLength) + " адрэзкаў)";
        t.xLabel = "Даўжыня адрэзка, пікс.";
        for (int s = 1; s <= steps; ++s) t.x.push_back(std::max(1, cfg.maxLength * s / steps));
        std::vector<std::vector<Seg>> fans;
        for (double L : t.x) fans.push_back(MakeFan(cfg.linesForLength, static_cast<int>(L), rng));
        for (int a = 0; a < A_COUNT; ++a) {
            BenchSeries ser;
            ser.name = kNames[a];
            for (size_t k = 0; k < t.x.size(); ++k) {
                ser.seconds.push_back(TimeLines(static_cast<LineAlgo>(a), grid, fans[k], cfg.linesForLength, cfg.repeats));
                if (!report(std::string(kNames[a]) + ": даўжыня " + std::to_string(static_cast<int>(t.x[k])))) { t.series.push_back(ser); out.push_back(t); return out; }
            }
            t.series.push_back(ser);
        }
        out.push_back(t);
    }

    //Колы: час ад колькасці
    {
        BenchTable t;
        t.id = "circles_count";
        t.title = "Колы: час ад колькасці (радыус выпадковы: 1-" + std::to_string(cfg.maxRadius) + ")";
        t.xLabel = "Колькасць колаў";
        std::uniform_int_distribution<int> rd(1, cfg.maxRadius);
        std::vector<Circ> cs(cfg.maxCircles);
        for (auto &c : cs) c.r = rd(rng);
        for (int s = 1; s <= steps; ++s) t.x.push_back(std::max(1, cfg.maxCircles * s / steps));
        BenchSeries ser; ser.name = "Брэзенхэм (кола)";
        for (double nx : t.x) {
            ser.seconds.push_back(TimeCircles(grid, cs, static_cast<int>(nx), cfg.repeats));
            if (!report("Колы: " + std::to_string(static_cast<int>(nx)))) { t.series.push_back(ser); out.push_back(t); return out; }
        }
        t.series.push_back(ser);
        out.push_back(t);
    }

    //Колы: час ад радыуса
    {
        BenchTable t;
        t.id = "circles_radius";
        t.title = "Колы: час ад радыуса (" + std::to_string(cfg.maxCircles / 10) + " колаў)";
        t.xLabel = "Радыус, пікс.";
        for (int s = 1; s <= steps; ++s) t.x.push_back(std::max(1, cfg.maxRadius * 2 * s / steps));
        BenchSeries ser; ser.name = "Брэзенхэм (кола)";
        const int n = std::max(1, cfg.maxCircles / 10);
        for (double rx : t.x) {
            std::vector<Circ> cs(n, Circ{static_cast<int>(rx)});
            ser.seconds.push_back(TimeCircles(grid, cs, n, cfg.repeats));
            if (!report("Радыус " + std::to_string(static_cast<int>(rx)))) { t.series.push_back(ser); out.push_back(t); return out; }
        }
        t.series.push_back(ser);
        out.push_back(t);
    }

    if (completed) *completed = true;
    if (progress) progress(total, total, "Гатова");
    return out;
}

std::string BenchToCsv(const std::vector<BenchTable> &tables)
{
    std::string s;
    char buf[64];
    for (const auto &t : tables) {
        s += "# " + t.title + "\n";
        s += t.xLabel;
        for (const auto &ser : t.series) s += ";" + ser.name + ", с";
        s += "\n";
        for (size_t i = 0; i < t.x.size(); ++i) {
            std::snprintf(buf, sizeof buf, "%.0f", t.x[i]);
            s += buf;
            for (const auto &ser : t.series) {
                if (i < ser.seconds.size()) { std::snprintf(buf, sizeof buf, ";%.6f", ser.seconds[i]); s += buf; }
                else s += ";";
            }
            s += "\n";
        }
        s += "\n";
    }
    return s;
}
