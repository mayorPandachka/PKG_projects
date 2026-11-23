#ifndef filters_H
#define filters_H

#include "imagebuffer.h"
#include <vector>
#include <cstdint>

class filters {
public:
    filters() = default;

    // ----- Локальная пороговая обработка -----
    ImageBuffer niblack(const ImageBuffer &src, int window = 15, double k = -0.2);
    ImageBuffer sauvola(const ImageBuffer &src, int window = 15,
                        double k = 0.5, double R = 128.0);

    // ----- Адаптивная пороговая обработка (Bradley через SAT) -----
    ImageBuffer adaptiveBradley(const ImageBuffer &src, int window = 25,
                                double t = 0.15);

    // ----- Нелинейные фильтры на порядковых статистиках -----
    ImageBuffer medianFilter(const ImageBuffer &src, int radius = 1);
    ImageBuffer minFilter(const ImageBuffer &src, int radius = 1);
    ImageBuffer maxFilter(const ImageBuffer &src, int radius = 1);

private:
    // ---- SAT-инфраструктура ----
    // Интегральные изображения суммы I и суммы квадратов I2.
    // Оба размера (W+1)×(H+1), int64 — чтобы избежать переполнения.
    struct IntegralPair {
        int W = 0, H = 0;
        std::vector<int64_t> I;   // Σ пикселей
        std::vector<int64_t> I2;  // Σ пикселей²
    };

    IntegralPair buildIntegralPair(const ImageBuffer &src) const;

    // Сумма по прямоугольнику [x1..x2]×[y1..y2] (включительно), по одному из SAT
    static int64_t rectSum(const std::vector<int64_t> &sat, int W,
                           int x1, int y1, int x2, int y2);

    // Статистика по окну через SAT (O(1)):
    void windowStatsSAT(const IntegralPair &sat, int cx, int cy, int r,
                        double &mean, double &stddev) const;

    // Пороговая бинаризация: пиксель > T ? 255 : 0
    void binarizeByThreshold(const ImageBuffer &src,
                             const std::vector<double> &thresholds,
                             ImageBuffer &dst) const;
};

#endif