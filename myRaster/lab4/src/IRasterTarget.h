#pragma once

// Интерфейс приёмника пикселей. Алгоритмы растеризации знают только о нём:
// они не рисуют, а лишь сообщают "пиксель (x, y) имеет интенсивность I".
// intensity: 0.0 - пиксель не закрашен, 1.0 - полностью закрашен.
class IRasterTarget
{
public:
    virtual ~IRasterTarget() = default;
    virtual void SetPixel(int x, int y, double intensity) = 0;
};
