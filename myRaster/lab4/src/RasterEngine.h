#pragma once
#include "IRasterTarget.h"

// Библиотека статических методов с "чистыми" алгоритмами растеризации.
// Никакого рисования: результат передаётся только через IRasterTarget::SetPixel.
// Система координат: целое (x, y) - центр пикселя; ось Y направлена вверх.
class RasterEngine
{
public:
    // 1. Пошаговый алгоритм: y = y0 + k*(x - x0), округление на каждом шаге.
    static void StepLine(int x0, int y0, int x1, int y1, IRasterTarget &target);

    // 2. ЦДА (DDA): приращения dx/L, dy/L, L = max(|dx|,|dy|), накопление в double.
    static void DDALine(int x0, int y0, int x1, int y1, IRasterTarget &target);

    // 3. Брезенхем (отрезок): только целые числа, работает во всех 8 октантах.
    static void BresenhamLine(int x0, int y0, int x1, int y1, IRasterTarget &target);

    // 4. Ву (сглаживание): интенсивность двух соседних пикселей определяется
    //    дробной частью координаты. Координаты вещественные.
    static void WuLine(double x0, double y0, double x1, double y1, IRasterTarget &target);

    // 5. Брезенхем (окружность): вычисляется один октант, остальные - симметрией.
    static void BresenhamCircle(int cx, int cy, int r, IRasterTarget &target);
};
