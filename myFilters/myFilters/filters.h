#ifndef filters_H
#define filters_H

#include "imagebuffer.h"
#include <vector>
#include <cstdint>

class filters {
public:
    filters() = default;

    ImageBuffer niblack(const ImageBuffer &src, int window = 15, double k = -0.2);
    ImageBuffer sauvola(const ImageBuffer &src, int window = 15,
                        double k = 0.5, double R = 128.0);

    ImageBuffer adaptiveBradley(const ImageBuffer &src, int window = 25,
                                double t = 0.15);

    ImageBuffer medianFilter(const ImageBuffer &src, int radius = 1);
    ImageBuffer minFilter(const ImageBuffer &src, int radius = 1);
    ImageBuffer maxFilter(const ImageBuffer &src, int radius = 1);

private:
    struct IntegralPair {
        int W = 0, H = 0;
        std::vector<int64_t> I;
        std::vector<int64_t> I2;
    };

    IntegralPair buildIntegralPair(const ImageBuffer &src) const;

    static int64_t rectSum(const std::vector<int64_t> &sat, int W, int x1, int y1, int x2, int y2);

    void windowStatsSAT(const IntegralPair &sat, int cx, int cy, int r, double &mean, double &stddev) const;

    void binarizeByThreshold(const ImageBuffer &src, const std::vector<double> &thresholds, ImageBuffer &dst) const;
};

#endif