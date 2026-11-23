#include "filters.h"
#include <algorithm>
#include <cmath>

// Пара iнтэгральных вiдарысаў:
//  I(x,y)  = sum{i<x, j<y} p(i,j)
//    I2(x,y) = sum{i<x, j<y} p(i,j)²
//  Абодва памеру (W+1)*(H+1), першы радок i слуп — нулi
filters::IntegralPair filters::buildIntegralPair(const ImageBuffer &src) const
{
    IntegralPair p;
    p.W = src.width;
    p.H = src.height;
    p.I .assign((p.W + 1) * (p.H + 1), 0);
    p.I2.assign((p.W + 1) * (p.H + 1), 0);

    int W1 = p.W + 1;
    for (int y = 0; y < p.H; ++y) {
        int64_t rowSum  = 0;
        int64_t rowSum2 = 0;
        for (int x = 0; x < p.W; ++x) {
            int64_t v = src.at(x, y);
            rowSum  += v;
            rowSum2 += v * v;
            p.I [(y + 1) * W1 + (x + 1)] = p.I [(y) * W1 + (x + 1)] + rowSum;
            p.I2[(y + 1) * W1 + (x + 1)] = p.I2[(y) * W1 + (x + 1)] + rowSum2;
        }
    }
    return p;
}

int64_t filters::rectSum(const std::vector<int64_t> &sat, int W, int x1, int y1, int x2, int y2)
{
    int W1 = W + 1;
    auto at = [&](int x, int y) -> int64_t { return sat[y * W1 + x]; };
    return at(x2 + 1, y2 + 1) - at(x1, y2 + 1) - at(x2 + 1, y1) + at(x1, y1);
}

void filters::windowStatsSAT(const IntegralPair &sat, int cx, int cy, int r,
                             double &mean, double &stddev) const
{
    int x1 = std::max(0, cx - r);
    int y1 = std::max(0, cy - r);
    int x2 = std::min(sat.W - 1, cx + r);
    int y2 = std::min(sat.H - 1, cy + r);

    int64_t N  = int64_t(x2 - x1 + 1) * (y2 - y1 + 1);
    int64_t S1 = rectSum(sat.I,  sat.W, x1, y1, x2, y2);
    int64_t S2 = rectSum(sat.I2, sat.W, x1, y1, x2, y2);

    mean = double(S1) / double(N);
    double var = double(S2) / double(N) - mean * mean;
    if (var < 0) var = 0;
    stddev = std::sqrt(var);
}

void filters::binarizeByThreshold(const ImageBuffer &src,
                                  const std::vector<double> &thresholds,
                                  ImageBuffer &dst) const
{
    dst.width  = src.width;
    dst.height = src.height;
    dst.gray.assign(src.gray.size(), 0);

    for (int y = 0; y < src.height; ++y) {
        for (int x = 0; x < src.width; ++x) {
            double T = thresholds[y * src.width + x];
            dst.set(x, y, src.at(x, y) > T ? 255 : 0);
        }
    }
}

ImageBuffer filters::niblack(const ImageBuffer &src, int window, double k)
{
    IntegralPair sat = buildIntegralPair(src);
    int r = window / 2;

    std::vector<double> T(src.width * src.height);
    for (int y = 0; y < src.height; ++y) {
        for (int x = 0; x < src.width; ++x) {
            double m, s;
            windowStatsSAT(sat, x, y, r, m, s);
            T[y * src.width + x] = m + k * s;
        }
    }

    ImageBuffer out;
    binarizeByThreshold(src, T, out);
    return out;
}

ImageBuffer filters::sauvola(const ImageBuffer &src, int window,
                             double k, double R)
{
    IntegralPair sat = buildIntegralPair(src);
    int r = window / 2;

    std::vector<double> T(src.width * src.height);
    for (int y = 0; y < src.height; ++y) {
        for (int x = 0; x < src.width; ++x) {
            double m, s;
            windowStatsSAT(sat, x, y, r, m, s);
            T[y * src.width + x] = m * (1.0 + k * (s / R - 1.0));
        }
    }

    ImageBuffer out;
    binarizeByThreshold(src, T, out);
    return out;
}

ImageBuffer filters::adaptiveBradley(const ImageBuffer &src, int window, double t)
{
    IntegralPair sat = buildIntegralPair(src);
    int W = src.width, H = src.height;
    int r = window / 2;

    ImageBuffer out;
    out.width = W; out.height = H;
    out.gray.assign(src.gray.size(), 0);

    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            int x1 = std::max(0, x - r);
            int y1 = std::max(0, y - r);
            int x2 = std::min(W - 1, x + r);
            int y2 = std::min(H - 1, y + r);

            int64_t sum  = rectSum(sat.I, W, x1, y1, x2, y2);
            int64_t area = int64_t(x2 - x1 + 1) * (y2 - y1 + 1);

            bool black = int64_t(src.at(x, y)) * area < sum * (1.0 - t);
            out.set(x, y, black ? 0 : 255);
        }
    }
    return out;
}

ImageBuffer filters::medianFilter(const ImageBuffer &src, int radius)
{
    ImageBuffer out;
    out.width = src.width;
    out.height = src.height;
    out.gray.assign(src.gray.size(), 0);

    int side = 2 * radius + 1;
    std::vector<uint8_t> buf(side * side);

    for (int y = 0; y < src.height; ++y) {
        for (int x = 0; x < src.width; ++x) {
            int idx = 0;
            for (int dy = -radius; dy <= radius; ++dy)
                for (int dx = -radius; dx <= radius; ++dx)
                    buf[idx++] = src.atClamped(x + dx, y + dy);

            std::nth_element(buf.begin(), buf.begin() + idx / 2, buf.end());
            out.set(x, y, buf[idx / 2]);
        }
    }
    return out;
}

ImageBuffer filters::minFilter(const ImageBuffer &src, int radius)
{
    ImageBuffer out;
    out.width = src.width;
    out.height = src.height;
    out.gray.assign(src.gray.size(), 0);

    for (int y = 0; y < src.height; ++y) {
        for (int x = 0; x < src.width; ++x) {
            uint8_t v = 255;
            for (int dy = -radius; dy <= radius; ++dy)
                for (int dx = -radius; dx <= radius; ++dx) {
                    uint8_t p = src.atClamped(x + dx, y + dy);
                    if (p < v) v = p;
                }
            out.set(x, y, v);
        }
    }
    return out;
}

ImageBuffer filters::maxFilter(const ImageBuffer &src, int radius)
{
    ImageBuffer out;
    out.width = src.width;
    out.height = src.height;
    out.gray.assign(src.gray.size(), 0);

    for (int y = 0; y < src.height; ++y) {
        for (int x = 0; x < src.width; ++x) {
            uint8_t v = 0;
            for (int dy = -radius; dy <= radius; ++dy)
                for (int dx = -radius; dx <= radius; ++dx) {
                    uint8_t p = src.atClamped(x + dx, y + dy);
                    if (p > v) v = p;
                }
            out.set(x, y, v);
        }
    }
    return out;
}