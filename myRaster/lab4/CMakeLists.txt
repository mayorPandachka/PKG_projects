cmake_minimum_required(VERSION 3.16)
project(Lab4RasterAlgorithms LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)   # бенчмарк имеет смысл только в Release
endif()

# Чистое ядро (без Qt): алгоритмы, виртуальный растр, трассировка, бенчмарк
add_library(raster_core STATIC
  src/RasterEngine.cpp src/VirtualGrid.cpp src/RasterTrace.cpp src/Benchmark.cpp)
target_include_directories(raster_core PUBLIC src)

# Автотесты и консольный бенчмарк
add_executable(raster_tests tests/tests.cpp)
target_link_libraries(raster_tests raster_core)
add_executable(bench_cli tools/bench_cli.cpp)
target_link_libraries(bench_cli raster_core)

# GUI на Qt (Qt6, при отсутствии - Qt5)
find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets)
if(QT_FOUND)
  find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
  add_executable(RasterLab WIN32
    src/main.cpp src/MainWindow.h src/MainWindow.cpp
    src/GridPainter.h src/GridPainter.cpp
    src/ChartWidget.h src/ChartWidget.cpp
    src/BenchmarkDialog.h src/BenchmarkDialog.cpp)
  target_link_libraries(RasterLab PRIVATE raster_core Qt${QT_VERSION_MAJOR}::Widgets)
endif()
