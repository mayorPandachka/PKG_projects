#ifndef IMAGEBUFFER_H
#define IMAGEBUFFER_H

#include <QImage>
#include <vector>
#include <cstdint>

class ImageBuffer {
public:
    int width = 0;
    int height = 0;

    std::vector<uint8_t> gray;
    std::vector<uint8_t> r;
    std::vector<uint8_t> g;
    std::vector<uint8_t> b;
    bool hasColor = false;

    ImageBuffer() = default;

    explicit ImageBuffer(const QImage &img);

    ImageBuffer(int w, int h, bool color = false);

    inline uint8_t at(int x, int y) const { return gray[y * width + x]; }
    inline void set(int x, int y, uint8_t v) { gray[y * width + x] = v; }

    inline uint8_t atClamped(int x, int y) const {
        if (x < 0) x = 0; if (x >= width)  x = width  - 1;
        if (y < 0) y = 0; if (y >= height) y = height - 1;
        return gray[y * width + x];
    }

    inline uint8_t rAt(int x, int y) const { return r[y * width + x]; }
    inline uint8_t gAt(int x, int y) const { return g[y * width + x]; }
    inline uint8_t bAt(int x, int y) const { return b[y * width + x]; }
    inline void setRGB(int x, int y, uint8_t R, uint8_t G, uint8_t B) {
        int i = y * width + x;
        r[i] = R; g[i] = G; b[i] = B;

        gray[i] = uint8_t(0.299 * R + 0.587 * G + 0.114 * B);
    }

    QImage toQImage() const;
    QImage toQImageColor() const;

    static ImageBuffer applyBinaryMask(const ImageBuffer &colorSource, const ImageBuffer &binaryMask);
};

#endif