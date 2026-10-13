#ifndef IMAGEPARSER_H
#define IMAGEPARSER_H

#include <QString>
#include <QFile>

struct ImageInfo {
    QString fileName;
    QString format;
    quint32 width  = 0;
    quint32 height = 0;
    double  dpiX   = 0.0;
    double  dpiY   = 0.0;
    int     bitDepth = 0;
    QString compression;
    QString status;
    QString extra;
};

class ImageParser
{
public:
    static ImageInfo parse(const QString &filePath);

private:
    static ImageInfo parseBMP(QFile &f, const QString &path);
    static ImageInfo parsePNG(QFile &f, const QString &path);
    static ImageInfo parseJPEG(QFile &f, const QString &path);
    static ImageInfo parseGIF(QFile &f, const QString &path);
    static ImageInfo parseTIFF(QFile &f, const QString &path);
    static ImageInfo parsePCX(QFile &f, const QString &path);

    static quint16 readU16LE(const QByteArray &b, int off);
    static quint16 readU16BE(const QByteArray &b, int off);
    static quint32 readU32LE(const QByteArray &b, int off);
    static quint32 readU32BE(const QByteArray &b, int off);
};

#endif // IMAGEPARSER_H