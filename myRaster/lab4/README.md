# ЛР 4. Базовые растровые алгоритмы (Qt6, C++17)

Сборка (Qt 6, CMake):
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release
Или откройте RasterLab.pro в Qt Creator (qmake).

Цели: RasterLab (GUI), raster_tests (автотесты ядра), bench_cli (бенчмарк без GUI).
Бенчмарк имеет смысл запускать только в Release.

Модули: IRasterTarget, RasterEngine (StepLine, DDALine, BresenhamLine, WuLine, BresenhamCircle),
VirtualGrid, GridPainter, RasterTrace (таблицы вычислений), Benchmark/BenchmarkDialog/ChartWidget.
Управление: колесо - масштаб, правая/средняя кнопка - перемещение, левый клик - задать точки.
