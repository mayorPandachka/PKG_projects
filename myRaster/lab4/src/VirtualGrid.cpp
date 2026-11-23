#include "VirtualGrid.h"
#include <algorithm>

VirtualGrid::VirtualGrid(int minX, int maxX, int minY, int maxY)
    : m_minX(minX), m_maxX(maxX), m_minY(minY), m_maxY(maxY),
      m_w(maxX - minX + 1), m_h(maxY - minY + 1),
      m_data(static_cast<size_t>(m_w) * static_cast<size_t>(m_h), 0.0f)
{
}

void VirtualGrid::SetPixel(int x, int y, double intensity)
{
    if (x < m_minX || x > m_maxX || y < m_minY || y > m_maxY) return;   //адсячэнне
    if (intensity <= 0.0) return;
    float &cell = m_data[static_cast<size_t>(y - m_minY) * m_w + (x - m_minX)];
    const float v = static_cast<float>(intensity > 1.0 ? 1.0 : intensity);
    if (v > cell) cell = v;
    ++m_writes;
}

double VirtualGrid::Get(int x, int y) const
{
    if (!Contains(x, y)) return 0.0;
    return m_data[static_cast<size_t>(y - m_minY) * m_w + (x - m_minX)];
}

void VirtualGrid::Clear()
{
    std::fill(m_data.begin(), m_data.end(), 0.0f);
    m_writes = 0;
}

long long VirtualGrid::CountLit() const
{
    long long n = 0;
    for (float v : m_data) if (v > 0.0f) ++n;
    return n;
}

double VirtualGrid::Checksum() const
{
    double s = 0.0;
    for (float v : m_data) s += v;
    return s;
}
