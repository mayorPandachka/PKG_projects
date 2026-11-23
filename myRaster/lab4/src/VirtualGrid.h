#pragma once
#include "IRasterTarget.h"
#include <cstdint>
#include <vector>

// Виртуальный растр: двумерный массив интенсивностей [0..1].
// Работает в "логических" координатах: x in [MinX..MaxX], y in [MinY..MaxY],
// ось Y вверх. Точка (0,0) - центр пикселя в середине растра.
class VirtualGrid final : public IRasterTarget
{
public:
    VirtualGrid(int minX, int maxX, int minY, int maxY);

    void SetPixel(int x, int y, double intensity) override;   // при наложении берётся максимум
    double Get(int x, int y) const;
    bool Contains(int x, int y) const { return x >= m_minX && x <= m_maxX && y >= m_minY && y <= m_maxY; }

    void Clear();                       // сброс растра и счётчика записей
    long long CountLit() const;         // число ненулевых пикселей
    std::uint64_t WriteCount() const { return m_writes; }   // число принятых вызовов SetPixel
    double Checksum() const;            // сумма интенсивностей (защита от выкидывания кода оптимизатором)

    int MinX() const { return m_minX; }
    int MaxX() const { return m_maxX; }
    int MinY() const { return m_minY; }
    int MaxY() const { return m_maxY; }
    int Width() const { return m_w; }
    int Height() const { return m_h; }

private:
    int m_minX, m_maxX, m_minY, m_maxY, m_w, m_h;
    std::vector<float> m_data;
    std::uint64_t m_writes = 0;
};
