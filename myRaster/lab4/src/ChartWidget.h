#pragma once
#include "Benchmark.h"
#include <QWidget>

// Простой график "время (мс) от x" с несколькими сериями; рисуется через QPainter,
// чтобы не зависеть от модуля QtCharts.
class ChartWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ChartWidget(QWidget *parent = nullptr);
    void SetTable(const BenchTable &t);

protected:
    void paintEvent(QPaintEvent *e) override;

private:
    BenchTable m_table;
    bool m_has = false;
};
