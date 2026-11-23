#include "imagebuffer.h"
#include <algorithm>
#include <cmath>

//выняць RGB i вылiчыць Gray
ImageBuffer::ImageBuffer(const QImage &img)
{
    QImage src = img.convertToFormat(QImage::Format_RGB32);
    width  = src.width();
    height = src.height();

    const int N = width * height;
    gray.resize(N);
    r.resize(N);
    g.resize(N);
    b.resize(N);

    // Азначаем, цi не з'яўляецца колер адценнем шэрага
    bool color = false;

    for (int y = 0; y < height; ++y) {
        const QRgb *line = reinterpret_cast<const QRgb*>(src.constScanLine(y));
        for (int x = 0; x < width; ++x) {
            QRgb px = line[x];
            uint8_t R = qRed(px);
            uint8_t G = qGreen(px);
            uint8_t B = qBlue(px);

            int i = y * width + x;
            r[i] = R; g[i] = G; b[i] = B;
            gray[i] = uint8_t(0.299 * R + 0.587 * G + 0.114 * B + 0.5);

            if (!color && (R != G || G != B))
                color = true;
        }
    }
    hasColor = color;
}

ImageBuffer::ImageBuffer(int w, int h, bool color)
    : width(w), height(h), hasColor(color)
{
    const int N = w * h;
    gray.assign(N, 0);
    if (color) {
        r.assign(N, 0);
        g.assign(N, 0);
        b.assign(N, 0);
    }
}

QImage ImageBuffer::toQImage() const
{
    QImage out(width, height, QImage::Format_Grayscale8);
    for (int y = 0; y < height; ++y) {
        uint8_t *dst = out.scanLine(y);
        std::copy(gray.begin() + y * width,
                  gray.begin() + (y + 1) * width, dst);
    }
    return out;
}

QImage ImageBuffer::toQImageColor() const
{
    if (!hasColor) return toQImage();

    QImage out(width, height, QImage::Format_RGB32);
    for (int y = 0; y < height; ++y) {
        QRgb *line = reinterpret_cast<QRgb*>(out.scanLine(y));
        for (int x = 0; x < width; ++x) {
            int i = y * width + x;
            line[x] = qRgb(r[i], g[i], b[i]);
        }
    }
    return out;
}


ImageBuffer ImageBuffer::applyBinaryMask(const ImageBuffer &colorSource,
                                         const ImageBuffer &binaryMask)
{
    ImageBuffer out(colorSource.width, colorSource.height, colorSource.hasColor);

    const int N = colorSource.width * colorSource.height;
    for (int i = 0; i < N; ++i) {
        if (binaryMask.gray[i] > 127) {
            out.gray[i] = colorSource.gray[i];
            if (out.hasColor) {
                out.r[i] = colorSource.r[i];
                out.g[i] = colorSource.g[i];
                out.b[i] = colorSource.b[i];
            }
        } else {
            out.gray[i] = 0;
            if (out.hasColor) { out.r[i] = 0; out.g[i] = 0; out.b[i] = 0; }
        }
    }
    return out;
}