#pragma once
#include "Benchmark.h"
#include <QDialog>
#include <QThread>
#include <atomic>
#include <vector>

class ChartWidget;
class QPushButton;
class QProgressBar;
class QLabel;
class QSpinBox;
class QTabWidget;
class QTableWidget;

// Рабочий поток: выполняет RunBenchmark, не блокируя интерфейс.
class BenchWorker : public QThread
{
    Q_OBJECT
public:
    BenchConfig config;
    std::vector<BenchTable> result;
    bool completed = false;
    std::atomic<bool> cancel{false};

signals:
    void Progress(int done, int total, QString message);

protected:
    void run() override;
};

class BenchmarkDialog : public QDialog
{
    Q_OBJECT
public:
    explicit BenchmarkDialog(QWidget *parent = nullptr);
    ~BenchmarkDialog() override;

private slots:
    void OnStart();
    void OnProgress(int done, int total, const QString &msg);
    void OnFinished();
    void OnExportCsv();
    void OnExportPng();

private:
    void FillSummary();

    QSpinBox *m_lines, *m_circles, *m_length, *m_repeats;
    QPushButton *m_start, *m_csv, *m_png;
    QProgressBar *m_bar;
    QLabel *m_status;
    QTabWidget *m_tabs;
    QTableWidget *m_summary;
    std::vector<ChartWidget *> m_charts;
    BenchWorker *m_worker = nullptr;
    std::vector<BenchTable> m_result;
};
