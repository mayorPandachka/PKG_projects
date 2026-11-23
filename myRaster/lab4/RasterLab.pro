# Альтернативная сборка через qmake (Qt Creator)
QT += widgets
CONFIG += c++17
TARGET = RasterLab
INCLUDEPATH += src
SOURCES += src/main.cpp src/MainWindow.cpp src/GridPainter.cpp src/ChartWidget.cpp src/BenchmarkDialog.cpp \
           src/RasterEngine.cpp src/VirtualGrid.cpp src/RasterTrace.cpp src/Benchmark.cpp
HEADERS += src/MainWindow.h src/GridPainter.h src/ChartWidget.h src/BenchmarkDialog.h \
           src/RasterEngine.h src/IRasterTarget.h src/VirtualGrid.h src/RasterTrace.h src/Benchmark.h
