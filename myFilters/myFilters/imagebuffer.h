#ifndef IMAGEBUFFER_H
#define IMAGEBUFFER_H

#include <QImage>
#include <vector>
#include <cstdint>

class ImageBuffer {
public:
    int width = 0;
    int height = 0;

    // ---- Grayscale-канал (по нему работают все фильтры) ----
    std::vector<uint8_t> gray;   // размер W*H

    // ---- Цветные каналы (для отображения и цветных операций) ----
    // Внутренний формат: planar RGB (три плоскости подряд).
    std::vector<uint8_t> r;      // W*H
    std::vector<uint8_t> g;      // W*H
    std::vector<uint8_t> b;      // W*H
    bool hasColor = false;       // false, если исходник был чисто grayscale

    ImageBuffer() = default;

    // Конструктор из QImage: заполняет и gray, и (если есть цвет) r/g/b
    explicit ImageBuffer(const QImage &img);

    // Создать пустой буфер заданного размера (все каналы = 0)
    ImageBuffer(int w, int h, bool color = false);

    // ---- Grayscale-доступ (совместимость со старым кодом) ----
    inline uint8_t at(int x, int y) const { return gray[y * width + x]; }
    inline void set(int x, int y, uint8_t v) { gray[y * width + x] = v; }

    inline uint8_t atClamped(int x, int y) const {
        if (x < 0) x = 0; if (x >= width)  x = width  - 1;
        if (y < 0) y = 0; if (y >= height) y = height - 1;
        return gray[y * width + x];
    }

    // ---- Цветной доступ ----
    inline uint8_t rAt(int x, int y) const { return r[y * width + x]; }
    inline uint8_t gAt(int x, int y) const { return g[y * width + x]; }
    inline uint8_t bAt(int x, int y) const { return b[y * width + x]; }
    inline void setRGB(int x, int y, uint8_t R, uint8_t G, uint8_t B) {
        int i = y * width + x;
        r[i] = R; g[i] = G; b[i] = B;
        // пересчитать яркость (BT.601), чтобы gray оставался согласован
        gray[i] = uint8_t(0.299 * R + 0.587 * G + 0.114 * B + 0.5);
    }

    // ---- Конвертации в QImage ----
    QImage toQImage() const;        // grayscale (как раньше)
    QImage toQImageColor() const;   // RGB, если hasColor; иначе grayscale

    // ---- Применение grayscale-результата как маски к цвету ----
    // result(x,y) = grayResult(x,y) ? originalColor(x,y) : 0
    // Полезно для пороговых методов: цветной фон + чёрная маска.
    static ImageBuffer applyBinaryMask(const ImageBuffer &colorSource,
                                       const ImageBuffer &binaryMask);
};

#endif