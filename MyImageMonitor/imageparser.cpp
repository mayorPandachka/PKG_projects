#include "imageparser.h"
#include <QFileInfo>
#include <QtEndian>
#include <QByteArray>

quint16 ImageParser::readU16LE(const QByteArray &b, int off)
{
    return static_cast<quint16>(static_cast<unsigned char>(b[off]))|(static_cast<quint16>(static_cast<unsigned char>(b[off+1])) << 8);
}
quint16 ImageParser::readU16BE(const QByteArray &b, int off)
{
    return (static_cast<quint16>(static_cast<unsigned char>(b[off])) << 8)|static_cast<quint16>(static_cast<unsigned char>(b[off+1]));
}
quint32 ImageParser::readU32LE(const QByteArray &b, int off)
{
    return  static_cast<quint32>(static_cast<unsigned char>(b[off]))|(static_cast<quint32>(static_cast<unsigned char>(b[off+1])) << 8)
    |(static_cast<quint32>(static_cast<unsigned char>(b[off+2])) << 16)|(static_cast<quint32>(static_cast<unsigned char>(b[off+3])) << 24);
}
quint32 ImageParser::readU32BE(const QByteArray &b, int off) {
    return (static_cast<quint32>(static_cast<unsigned char>(b[off])) << 24)
    | (static_cast<quint32>(static_cast<unsigned char>(b[off+1])) << 16)
        | (static_cast<quint32>(static_cast<unsigned char>(b[off+2])) << 8)
        |  static_cast<quint32>(static_cast<unsigned char>(b[off+3]));
}

ImageInfo ImageParser::parse(const QString &filePath)
{
    ImageInfo info;
    info.fileName = QFileInfo(filePath).fileName();

    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        info.status = "Не удалось открыть файл";
        return info;
    }

    QByteArray head = f.read(16);
    if (head.size() < 2) {
        info.status = "Файл поврежден (слишком мал)";
        return info;
    }

    const unsigned char *u = reinterpret_cast<const unsigned char*>(head.constData());

    if (u[0] == 0xFF && u[1] == 0xD8)
        return parseJPEG(f, filePath);

    if (head.size() >= 8 &&
        u[0]==0x89 && u[1]==0x50 && u[2]==0x4E && u[3]==0x47 &&
        u[4]==0x0D && u[5]==0x0A && u[6]==0x1A && u[7]==0x0A)
        return parsePNG(f, filePath);

    if (head.size() >= 6 && head.startsWith("GIF8"))
        return parseGIF(f, filePath);

    if (u[0] == 'B' && u[1] == 'M')
        return parseBMP(f, filePath);

    if (head.size() >= 4 &&
        ((u[0]=='I' && u[1]=='I' && u[2]==0x2A && u[3]==0x00) ||
         (u[0]=='M' && u[1]=='M' && u[2]==0x00 && u[3]==0x2A)))
        return parseTIFF(f, filePath);

    if (u[0] == 0x0A && u[1] <= 5 && u[2] == 0x01 && (u[3] == 1 || u[3] == 2 ||
                                                      u[3] == 4 || u[3] == 8))
        return parsePCX(f, filePath);

    info.status = "Неизвестный формат";
    return info;
}

ImageInfo ImageParser::parseBMP(QFile &f, const QString &)
{
    ImageInfo info;
    info.format = "BMP";
    info.fileName = QFileInfo(f.fileName()).fileName();

    f.seek(0);
    QByteArray buf = f.read(54);
    if (buf.size() < 54) {
        info.status = "Файл поврежден (заголовок обрезан)";
        return info;
    }

    quint32 fileSize = readU32LE(buf, 2);
    if (static_cast<quint32>(f.size()) < fileSize) {
        info.status = "Файл поврежден (размер меньше заявленного)";
        return info;
    }

    quint32 dataOffset = readU32LE(buf, 10);
    quint32 headerSize = readU32LE(buf, 14);

    qint32  width  = static_cast<qint32>(readU32LE(buf, 18));
    qint32  height = static_cast<qint32>(readU32LE(buf, 22));
    // quint16 planes = readU16LE(buf, 26);
    quint16 bpp    = readU16LE(buf, 28);
    quint32 compr  = readU32LE(buf, 30);
    qint32  xppm   = static_cast<qint32>(readU32LE(buf, 38));
    qint32  yppm   = static_cast<qint32>(readU32LE(buf, 42));

    info.width  = static_cast<quint32>(width < 0 ? -width : width);
    info.height = static_cast<quint32>(height < 0 ? -height : height);
    info.bitDepth = bpp;

    if (xppm > 0) info.dpiX = xppm * 0.0254;
        else info.dpiX = 72;
    if (yppm > 0) info.dpiY = yppm * 0.0254;
        else info.dpiY  = 72;

    switch (compr) {
    case 0: info.compression = "BI_RGB (без сжатия)"; break;
    case 1: info.compression = "BI_RLE8 (RLE, 8 бит)"; break;
    case 2: info.compression = "BI_RLE4 (RLE, 4 бита)"; break;
    case 3: info.compression = "BI_BITFIELDS"; break;
    case 4: info.compression = "BI_JPEG"; break;
    case 5: info.compression = "BI_PNG"; break;
    default: info.compression = QString("Неизвестно (%1)").arg(compr);
    }

    if (bpp <= 8) {
        quint32 paletteColors = (headerSize >= 40)
        ? (dataOffset - 14 - headerSize) / 4
        : 0;
        info.extra = QString("Палитра: %1 цвет(ов)").arg(paletteColors);
    }

    info.status = "OK";
    return info;
}

ImageInfo ImageParser::parsePNG(QFile &f, const QString &)
{
    ImageInfo info;
    info.format = "PNG";
    info.fileName = QFileInfo(f.fileName()).fileName();

    f.seek(8);

    QByteArray chunk = f.read(8 + 13 + 4);
    if (chunk.size() < 25) {
        info.status = "Файл поврежден (IHDR обрезан)";
        return info;
    }
    if (chunk.mid(4,4) != "IHDR") {
        info.status = "Файл поврежден (нет IHDR)";
        return info;
    }

    quint32 w = readU32BE(chunk, 8);
    quint32 h = readU32BE(chunk, 12);
    quint8  bitDepth = static_cast<quint8>(chunk[16]);
    quint8  colorType = static_cast<quint8>(chunk[17]);
    quint8  compr = static_cast<quint8>(chunk[18]);
    quint8  filter = static_cast<quint8>(chunk[19]);
    quint8  interlace = static_cast<quint8>(chunk[20]);

    info.width = w;
    info.height = h;

    int channels = 1;
    QString colorStr;
    switch (colorType) {
    case 0: channels = 1; colorStr = "Grayscale"; break;
    case 2: channels = 3; colorStr = "TrueColor (RGB)"; break;
    case 3: channels = 1; colorStr = "Indexed"; break;
    case 4: channels = 2; colorStr = "Grayscale+Alpha"; break;
    case 6: channels = 4; colorStr = "TrueColor+Alpha"; break;
    default: colorStr = "Неизвестно";
    }
    info.bitDepth = bitDepth * channels;

    info.compression = (compr == 0)
                           ? "Deflate/Inflate (по умолчанию)"
                           : QString("Неизвестно (%1)").arg(compr);

    f.seek(8);
    bool foundPhys = false;
    while (!f.atEnd()) {
        QByteArray hdr = f.read(8);
        if (hdr.size() < 8) break;
        quint32 len = readU32BE(hdr, 0);
        QByteArray type = hdr.mid(4, 4);
        if (type == "pHYs" && len >= 9) {
            QByteArray data = f.read(9);
            if (data.size() == 9) {
                quint32 ppuX = readU32BE(data, 0);
                quint32 ppuY = readU32BE(data, 4);
                quint8 unit = static_cast<quint8>(data[8]);
                if (unit == 1) {
                    info.dpiX = ppuX * 0.0254;
                    info.dpiY = ppuY * 0.0254;
                    foundPhys = true;
                }
            }
            f.read(4); // crc
        } else if (type == "IEND") {
            info.extra = QString("Цветовой тип: %1, Interlace: %2")
                             .arg(colorStr).arg(interlace);
            info.status = "OK";
            return info;
        } else {
            f.seek(f.pos() + len + 4);
        }
    }

    if (!foundPhys) info.dpiX = info.dpiY = 72.0;

    info.extra = QString("Цветовой тип: %1, Interlace: %2")
                     .arg(colorStr).arg(interlace);
    info.status = "Файл поврежден (нет IEND)";
    return info;
}

ImageInfo ImageParser::parseJPEG(QFile &f, const QString &)
{
    ImageInfo info;
    info.format = "JPEG";
    info.fileName = QFileInfo(f.fileName()).fileName();

    f.seek(2);

    bool progressive = false;
    bool foundSOF = false;

    while (!f.atEnd()) {
        quint8 marker;
        QByteArray b = f.read(1);
        if (b.isEmpty()) break;
        marker = static_cast<quint8>(b[0]);
        if (marker != 0xFF) continue;

        do { b = f.read(1); if (b.isEmpty()) { info.status = "Файл поврежден"; return info; } }
        while (static_cast<quint8>(b[0]) == 0xFF);
        marker = static_cast<quint8>(b[0]);

        if (marker == 0xD8 || marker == 0xD9 ||
            (marker >= 0xD0 && marker <= 0xD7) || marker == 0x01)
            continue;

        QByteArray lenBytes = f.read(2);
        if (lenBytes.size() < 2) break;
        quint16 segLen = readU16BE(lenBytes, 0);
        if (segLen < 2) { info.status = "Файл поврежден (сегмент)"; return info; }

        QByteArray segData = f.read(segLen - 2);
        if (segData.size() < segLen - 2) {
            info.status = "Файл поврежден (сегмент обрезан)";
            return info;
        }

        if (marker >= 0xC0 && marker <= 0xCF &&
            marker != 0xC4 && marker != 0xC8 && marker != 0xCC) {
            if (segData.size() >= 6) {
                quint8 precision = static_cast<quint8>(segData[0]);
                quint16 h = readU16BE(segData, 1);
                quint16 w = readU16BE(segData, 3);
                quint8 comps = static_cast<quint8>(segData[5]);
                info.width = w;
                info.height = h;
                info.bitDepth = precision * comps;
                progressive = (marker == 0xC2 || marker == 0xC6 ||
                               marker == 0xCA || marker == 0xCE);
                foundSOF = true;
            }
        }
        if (marker == 0xE0 && segData.size() >= 12 &&
            segData.startsWith("JFIF\0")) {
            quint8 units = static_cast<quint8>(segData[7]);
            quint16 xd = readU16BE(segData, 8);
            quint16 yd = readU16BE(segData, 10);
            if (units == 1) { info.dpiX = xd; info.dpiY = yd; }
            else if (units == 2) { info.dpiX = xd * 2.54; info.dpiY = yd * 2.54; }
        }

        if (foundSOF) break;
    }

    info.compression = progressive ? "Progressive DCT" : "Baseline DCT (Huffman)";

    f.seek(f.size() - 2);
    QByteArray tail = f.read(2);
    if (tail.size() == 2 &&
        static_cast<unsigned char>(tail[0]) == 0xFF &&
        static_cast<unsigned char>(tail[1]) == 0xD9) {
        info.status = "OK";
    } else {
        info.status = "Файл поврежден (нет маркера EOI)";
    }
    return info;
}

ImageInfo ImageParser::parseGIF(QFile &f, const QString &)
{
    ImageInfo info;
    info.format = "GIF";
    info.fileName = QFileInfo(f.fileName()).fileName();

    f.seek(0);
    QByteArray buf = f.read(13);
    if (buf.size() < 13) {
        info.status = "Файл поврежден (заголовок обрезан)";
        return info;
    }

    info.width  = readU16LE(buf, 6);
    info.height = readU16LE(buf, 8);
    quint8 packed = static_cast<quint8>(buf[10]);
    quint8 bgIndex = static_cast<quint8>(buf[11]);

    bool globalColorTable = (packed & 0x80) != 0;
    int colorRes = ((packed >> 4) & 0x07) + 1;
    int gctSize = 0;
    if (globalColorTable) {
        gctSize = 1 << ((packed & 0x07) + 1);
        info.bitDepth = (packed & 0x07) + 1;
    } else {
        info.bitDepth = 0;
    }

    info.compression = "LZW";
    info.dpiX = info.dpiY = 72.0;

    info.extra = QString("GCT: %1, цветов: %2, фон: %3, цвет.резолюция: %4 бит")
                     .arg(globalColorTable ? "есть" : "нет")
                     .arg(gctSize).arg(bgIndex).arg(colorRes);

    f.seek(f.size() - 1);
    QByteArray tail = f.read(1);
    if (tail.size() == 1 && static_cast<unsigned char>(tail[0]) == 0x3B)
        info.status = "OK";
    else
        info.status = "Файл поврежден (нет трейлера 0x3B)";

    return info;
}

ImageInfo ImageParser::parseTIFF(QFile &f, const QString &)
{
    ImageInfo info;
    info.format = "TIFF";
    info.fileName = QFileInfo(f.fileName()).fileName();

    f.seek(0);
    QByteArray head = f.read(8);
    if (head.size() < 8) { info.status = "Файл поврежден"; return info; }

    bool littleEndian = (head[0] == 'I');
    auto rd16 = [&](const QByteArray &b, int off) {
        return littleEndian ? readU16LE(b, off) : readU16BE(b, off);
    };
    auto rd32 = [&](const QByteArray &b, int off) {
        return littleEndian ? readU32LE(b, off) : readU32BE(b, off);
    };

    quint32 ifdOffset = rd32(head, 4);
    if (ifdOffset == 0 || ifdOffset >= static_cast<quint32>(f.size())) {
        info.status = "Файл поврежден (нет IFD)";
        return info;
    }

    f.seek(ifdOffset);
    QByteArray cnt = f.read(2);
    if (cnt.size() < 2) { info.status = "Файл поврежден (IFD)"; return info; }
    quint16 numEntries = rd16(cnt, 0);

    if (numEntries > 4096) { info.status = "Файл поврежден (IFD слишком большой)"; return info; }

    QByteArray entries = f.read(numEntries * 12);
    if (entries.size() < numEntries * 12) {
        info.status = "Файл поврежден (IFD обрезан)";
        return info;
    }

    int compression = -1;

    for (int i = 0; i < numEntries; ++i) {
        int off = i * 12;
        quint16 tag = rd16(entries, off);
        quint16 type = rd16(entries, off + 2);
        quint32 count = rd32(entries, off + 4);
        quint32 value = rd32(entries, off + 8);

        auto shortVal = [&]() -> quint16 {
            return littleEndian
                       ? readU16LE(entries, off + 8)
                       : readU16BE(entries, off + 8);
        };

        switch (tag) {
        case 256: info.width  = (type == 3) ? shortVal() : value; break;
        case 257: info.height = (type == 3) ? shortVal() : value; break;
        case 258: info.bitDepth = (type == 3) ? shortVal() : value; break;
        case 259: compression = (type == 3) ? shortVal() : value; break;
        case 282: {
            if (type == 5 && count == 1 && value + 8 <= static_cast<quint32>(f.size())) {
                qint64 save = f.pos();
                f.seek(value);
                QByteArray r = f.read(8);
                if (r.size() == 8) {
                    quint32 num = rd32(r, 0);
                    quint32 den = rd32(r, 4);
                    if (den) info.dpiX = double(num) / den;
                }
                f.seek(save);
            }
            break;
        }
        case 283: {
            if (type == 5 && count == 1 && value + 8 <= static_cast<quint32>(f.size())) {
                qint64 save = f.pos();
                f.seek(value);
                QByteArray r = f.read(8);
                if (r.size() == 8) {
                    quint32 num = rd32(r, 0);
                    quint32 den = rd32(r, 4);
                    if (den) info.dpiY = double(num) / den;
                }
                f.seek(save);
            }
            break;
        }
        default: break;
        }
    }

    switch (compression) {
    case 1: info.compression = "None (без сжатия)"; break;
    case 2: info.compression = "CCITT Group 3"; break;
    case 3: info.compression = "CCITT Group 4"; break;
    case 4: info.compression = "LZW"; break;
    case 5: info.compression = "JPEG (старое)"; break;
    case 6: info.compression = "JPEG (новое)"; break;
    case 7: info.compression = "JPEG"; break;
    case 8: info.compression = "Deflate"; break;
    case 32773: info.compression = "PackBits"; break;
    default: info.compression = QString("Неизвестно (%1)").arg(compression);
    }

    if (info.width == 0 || info.height == 0)
        info.status = "Файл поврежден (нет размеров)";
    else
        info.status = "OK";

    return info;
}

ImageInfo ImageParser::parsePCX(QFile &f, const QString &)
{
    ImageInfo info;
    info.format = "PCX";
    info.fileName = QFileInfo(f.fileName()).fileName();

    f.seek(0);
    QByteArray buf = f.read(128);
    if (buf.size() < 128) {
        info.status = "Файл поврежден (заголовок обрезан)";
        return info;
    }

    quint8 version = static_cast<quint8>(buf[1]);
    quint8 encoding = static_cast<quint8>(buf[2]);
    quint8 bitsPerPlane = static_cast<quint8>(buf[3]);
    quint16 xmin = readU16LE(buf, 4);
    quint16 ymin = readU16LE(buf, 6);
    quint16 xmax = readU16LE(buf, 8);
    quint16 ymax = readU16LE(buf, 10);
    quint16 hdpi = readU16LE(buf, 12);
    quint16 vdpi = readU16LE(buf, 14);
    quint8 nplanes = static_cast<quint8>(buf[65]);
    quint16 bytesPerLine = readU16LE(buf, 66);
    quint16 paletteInfo = readU16LE(buf, 68);

    info.width  = static_cast<quint32>(xmax - xmin + 1);
    info.height = static_cast<quint32>(ymax - ymin + 1);
    info.bitDepth = bitsPerPlane * nplanes;
    info.dpiX = hdpi;
    info.dpiY = vdpi;

    info.compression = (encoding == 1) ? "RLE" : "Без сжатия";

    QString verStr;
    switch (version) {
    case 0: verStr = "2.5"; break;
    case 2: verStr = "2.8 с палитрой"; break;
    case 3: verStr = "2.8 без палитры"; break;
    case 4: verStr = "Paintbrush for Windows"; break;
    case 5: verStr = "3.0+"; break;
    default: verStr = QString("Неизвестно (%1)").arg(version);
    }

    info.extra = QString("Версия: %1, плоскостей: %2, байт/строку: %3, палитра: %4")
                     .arg(verStr).arg(nplanes).arg(bytesPerLine)
                     .arg(paletteInfo == 1 ? "есть" : "нет");

    if (f.size() < 128) {
        info.status = "Файл поврежден (размер < 128)";
    } else {
        info.status = "OK";
    }
    return info;
}