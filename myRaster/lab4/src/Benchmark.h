#pragma once
#include <functional>
#include <string>
#include <vector>

// Модуль стресс-тестирования. Не зависит от Qt/UI: измеряется только время
// заполнения виртуального растра (VirtualGrid). Таймер - std::chrono::steady_clock
// (высокоточный монотонный, аналог Stopwatch в .NET).
struct BenchSeries
{
    std::string name;
    std::vector<double> seconds;   // одно значение на каждый x
};

struct BenchTable
{
    std::string id;       // короткий идентификатор (для имён файлов)
    std::string title;
    std::string xLabel;
    std::vector<double> x;
    std::vector<BenchSeries> series;
};

struct BenchConfig
{
    int maxLines = 100000;     // максимум отрезков в "солнце"
    int maxCircles = 50000;    // максимум окружностей
    int lineLength = 500;      // длина отрезков в тесте "время от количества"
    int maxLength = 1000;      // максимальная длина в тесте "время от длины"
    int linesForLength = 20000;// число отрезков в тесте "время от длины"
    int maxRadius = 500;       // максимальный радиус в тесте "время от количества окружностей"
    int repeats = 3;           // число повторов; берётся минимум
    unsigned seed = 12345;     // для воспроизводимости
};

// progress: (сделано, всего, сообщение); вернуть false - прервать.
using BenchProgress = std::function<bool(int, int, const std::string &)>;

// Возвращает 4 таблицы: lines_count, lines_length, circles_count, circles_radius.
// При отмене возвращается то, что успело посчитаться (ready == false).
std::vector<BenchTable> RunBenchmark(const BenchConfig &cfg, const BenchProgress &progress, bool *completed = nullptr);

// Сохранение таблиц в CSV (разделитель ';', десятичная точка).
std::string BenchToCsv(const std::vector<BenchTable> &tables);
