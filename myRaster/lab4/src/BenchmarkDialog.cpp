#include "BenchmarkDialog.h"
#include "ChartWidget.h"

#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <algorithm>

void BenchWorker::run()
{
    cancel = false;
    result = RunBenchmark(config, [this](int d, int t, const std::string &m) {
        emit Progress(d, t, QString::fromStdString(m));
        return !cancel.load();
    }, &completed);
}

BenchmarkDialog::BenchmarkDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Стрэс-тэст (бэнчмарк) алгарытмаў растэрызацыі"));
    resize(900, 720);

    m_lines = new QSpinBox; m_lines->setRange(1000, 1000000); m_lines->setSingleStep(10000); m_lines->setValue(100000);
    m_circles = new QSpinBox; m_circles->setRange(1000, 1000000); m_circles->setSingleStep(5000); m_circles->setValue(50000);
    m_length = new QSpinBox; m_length->setRange(10, 2000); m_length->setValue(500);
    m_repeats = new QSpinBox; m_repeats->setRange(1, 10); m_repeats->setValue(3);

    auto *form = new QFormLayout;
    form->addRow(QStringLiteral("Макс. колькасць адрэзкаў («сонца»):"), m_lines);
    form->addRow(QStringLiteral("Макс. колькасць колаў:"), m_circles);
    form->addRow(QStringLiteral("Даўжыня адрэзка, пікс.:"), m_length);
    form->addRow(QStringLiteral("Паўтораў (бярэцца мінімум):"), m_repeats);

    m_start = new QPushButton(QStringLiteral("Пачаць"));
    m_csv = new QPushButton(QStringLiteral("Экспарт CSV…"));
    m_png = new QPushButton(QStringLiteral("Захаваць графік у PNG…"));
    m_csv->setEnabled(false);
    m_png->setEnabled(false);
    m_bar = new QProgressBar;
    m_status = new QLabel(QStringLiteral("Замяраецца толькі запаўненне віртуальнага растра, без UI."));
    m_status->setWordWrap(true);

    m_tabs = new QTabWidget;
    const char *names[] = {"Адрэзкі: ад колькасці", "Адрэзкі: ад даўжыні", "Колы: ад колькасці", "Колы: ад радыуса"};
    for (const char *n : names) {
        auto *c = new ChartWidget;
        m_charts.push_back(c);
        m_tabs->addTab(c, QString::fromUtf8(n));
    }
    m_summary = new QTableWidget;
    m_summary->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tabs->addTab(m_summary, QStringLiteral("Высновы"));

    auto *btns = new QHBoxLayout;
    btns->addWidget(m_start);
    btns->addWidget(m_csv);
    btns->addWidget(m_png);
    btns->addStretch();

    auto *lay = new QVBoxLayout(this);
    lay->addLayout(form);
    lay->addLayout(btns);
    lay->addWidget(m_bar);
    lay->addWidget(m_status);
    lay->addWidget(m_tabs, 1);

    connect(m_start, &QPushButton::clicked, this, &BenchmarkDialog::OnStart);
    connect(m_csv, &QPushButton::clicked, this, &BenchmarkDialog::OnExportCsv);
    connect(m_png, &QPushButton::clicked, this, &BenchmarkDialog::OnExportPng);
}

BenchmarkDialog::~BenchmarkDialog()
{
    if (m_worker) {
        m_worker->cancel = true;
        m_worker->wait();
    }
}

void BenchmarkDialog::OnStart()
{
    if (m_worker && m_worker->isRunning()) {
        m_worker->cancel = true;
        m_start->setEnabled(false);
        return;
    }
    if (!m_worker) {
        m_worker = new BenchWorker;
        m_worker->setParent(this);
        connect(m_worker, &BenchWorker::Progress, this, &BenchmarkDialog::OnProgress, Qt::QueuedConnection);
        connect(m_worker, &QThread::finished, this, &BenchmarkDialog::OnFinished);
    }
    BenchConfig cfg;
    cfg.maxLines = m_lines->value();
    cfg.maxCircles = m_circles->value();
    cfg.lineLength = m_length->value();
    cfg.maxLength = std::max(1000, m_length->value() * 2);
    cfg.repeats = m_repeats->value();
    m_worker->config = cfg;
    m_bar->setValue(0);
    m_start->setText(QStringLiteral("Адмена"));
    m_csv->setEnabled(false);
    m_png->setEnabled(false);
    m_worker->start();
}

void BenchmarkDialog::OnProgress(int done, int total, const QString &msg)
{
    m_bar->setMaximum(total);
    m_bar->setValue(done);
    m_status->setText(msg);
}

void BenchmarkDialog::OnFinished()
{
    m_start->setText(QStringLiteral("Пачаць"));
    m_start->setEnabled(true);
    m_result = m_worker->result;
    for (size_t i = 0; i < m_charts.size() && i < m_result.size(); ++i) m_charts[i]->SetTable(m_result[i]);
    FillSummary();
    const bool ok = m_worker->completed;
    m_status->setText(ok ? QStringLiteral("Гатова.") : QStringLiteral("Перарвана: паказаны атрыманыя звесткі."));
    m_csv->setEnabled(!m_result.empty());
    m_png->setEnabled(!m_result.empty());
    m_bar->setValue(m_bar->maximum());
}

void BenchmarkDialog::FillSummary()
{
    // Выніковы час: N адрэзкаў и M колаў
    m_summary->clear();
    m_summary->setColumnCount(3);
    m_summary->setHorizontalHeaderLabels({QStringLiteral("Алгарытм"), QStringLiteral("Нагрузка"), QStringLiteral("Час, мс")});
    int row = 0;
    m_summary->setRowCount(0);
    auto add = [&](const QString &a, const QString &load, double sec) {
        m_summary->insertRow(row);
        m_summary->setItem(row, 0, new QTableWidgetItem(a));
        m_summary->setItem(row, 1, new QTableWidgetItem(load));
        m_summary->setItem(row, 2, new QTableWidgetItem(QString::number(sec * 1000.0, 'f', 2)));
        ++row;
    };
    for (size_t t : {size_t(0), size_t(2)}) {
        if (t >= m_result.size() || m_result[t].x.empty()) continue;
        const BenchTable &tb = m_result[t];
        const int n = static_cast<int>(tb.x.back());
        for (const auto &s : tb.series)
            if (!s.seconds.empty() && s.seconds.size() == tb.x.size())
                add(QString::fromStdString(s.name),
                    t == 0 ? QStringLiteral("%1 адрэзкаў").arg(n) : QStringLiteral("%1 колаў").arg(n),
                    s.seconds.back());
    }
    m_summary->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

void BenchmarkDialog::OnExportCsv()
{
    const QString fn = QFileDialog::getSaveFileName(this, QStringLiteral("Захаваць CSV"), "benchmark.csv", "CSV (*.csv)");
    if (fn.isEmpty()) return;
    QFile f(fn);
    if (!f.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, windowTitle(), QStringLiteral("Не атрымалася запісаць файл."));
        return;
    }
    f.write("\xEF\xBB\xBF");   //для Excel
    f.write(QByteArray::fromStdString(BenchToCsv(m_result)));
}

void BenchmarkDialog::OnExportPng()
{
    const int i = m_tabs->currentIndex();
    if (i < 0 || i >= static_cast<int>(m_charts.size())) {
        QMessageBox::information(this, windowTitle(), QStringLiteral("Абярыце ўкладку з графікам."));
        return;
    }
    const QString fn = QFileDialog::getSaveFileName(this, QStringLiteral("Захаваць графік"), "chart.png", "PNG (*.png)");
    if (fn.isEmpty()) return;
    m_charts[i]->grab().save(fn);
}
