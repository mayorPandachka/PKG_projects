#include "MainWindow.h"
#include "BenchmarkDialog.h"
#include "GridPainter.h"
#include "RasterEngine.h"
#include "RasterTrace.h"

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QStatusBar>
#include <QTableWidget>
#include <QVBoxLayout>
#include <chrono>
#include <cmath>

namespace {
QDoubleSpinBox *MakeCoord(double v)
{
    auto *s = new QDoubleSpinBox;
    s->setRange(-1000, 1000);
    s->setDecimals(2);
    s->setSingleStep(1.0);
    s->setValue(v);
    return s;
}
int RI(double v) { return static_cast<int>(std::lround(v)); }
}

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), m_grid(-200, 200, -200, 200)
{
    setWindowTitle(QStringLiteral("Лабараторная работа 4. Базавыя растравыя алгарытмы"));
    resize(1280, 800);

    m_view = new GridPainter;
    m_view->SetGrid(&m_grid);
    setCentralWidget(m_view);

    auto *panel = new QWidget;
    auto *pl = new QVBoxLayout(panel);

    m_algo = new QComboBox;
    m_algo->addItems({QStringLiteral("Пашагавы алгарытм"), QStringLiteral("ЦДА (DDA)"), QStringLiteral("Брэзенхэм (адрэзак)"), QStringLiteral("Ву (згладжванне)"), QStringLiteral("Брэзенхэм (кола)")});
    auto *algoForm = new QFormLayout;
    algoForm->addRow(QStringLiteral("Алгарытм:"), m_algo);
    pl->addLayout(algoForm);

    m_x0 = MakeCoord(-10); m_y0 = MakeCoord(-4); m_x1 = MakeCoord(12); m_y1 = MakeCoord(7);
    m_lineBox = new QGroupBox(QStringLiteral("Адрэзак"));
    auto *lf = new QFormLayout(m_lineBox);
    lf->addRow(QStringLiteral("x0:"), m_x0);
    lf->addRow(QStringLiteral("y0:"), m_y0);
    lf->addRow(QStringLiteral("x1:"), m_x1);
    lf->addRow(QStringLiteral("y1:"), m_y1);
    pl->addWidget(m_lineBox);

    m_cx = MakeCoord(0); m_cy = MakeCoord(0);
    m_r = new QSpinBox; m_r->setRange(0, 1000); m_r->setValue(9);
    m_circleBox = new QGroupBox(QStringLiteral("Кола"));
    auto *cf = new QFormLayout(m_circleBox);
    cf->addRow(QStringLiteral("Цэнтр x:"), m_cx);
    cf->addRow(QStringLiteral("Цэнтр y:"), m_cy);
    cf->addRow(QStringLiteral("Радыус R:"), m_r);
    pl->addWidget(m_circleBox);

    m_accumulate = new QCheckBox(QStringLiteral("Запашваць (не чысціць растр)"));
    pl->addWidget(m_accumulate);

    auto *bDraw = new QPushButton(QStringLiteral("Пабудаваць"));
    auto *bClear = new QPushButton(QStringLiteral("Ачысціць растр"));
    auto *bDemo = new QPushButton(QStringLiteral("Дэма: ўсе 8 актантаў"));
    auto *bBench = new QPushButton(QStringLiteral("Бэнчмарк…"));
    pl->addWidget(bDraw);
    pl->addWidget(bClear);
    pl->addWidget(bDemo);
    pl->addWidget(bBench);

    m_zoom = new QSlider(Qt::Horizontal);
    m_zoom->setRange(2, 100);
    m_zoom->setValue(16);
    auto *zf = new QFormLayout;
    zf->addRow(QStringLiteral("Маштаб (пікс/клетку):"), m_zoom);
    pl->addLayout(zf);
    auto *bReset = new QPushButton(QStringLiteral("Зкінуць від"));
    pl->addWidget(bReset);

    m_info = new QLabel;
    m_info->setWordWrap(true);
    m_info->setTextFormat(Qt::RichText);
    m_info->setText(QStringLiteral(
        "<b>Прывязка кардынатаў да сеткі.</b> Клетка — піксель віртуальнага растра. "
        "Цэлыя (x, y) — кардынаты <i>цэнтра</i> клеткі; простыя сеткі праходзяць па паўцэлых значэннях. "
        "Оси X і Y ідуць праз цэнтры клетак x=0, y=0. Растр: x, y ∈ [−200; 200] (401×401), "
        "чырвоная пункцірная рамка — яго мяжа.<br><br>"
        "<b>Кіраванне.</b> Кола — маштаб; правая/сярэдняя кнопка — змяшчэнне; "
        "левы клік — задаць пункты (1ы клік — пачатак/цэнтр, 2і — канец/радыус)."));
    pl->addWidget(m_info);
    m_cursor = new QLabel(QStringLiteral("—"));
    pl->addWidget(m_cursor);
    pl->addStretch();

    auto *dock = new QDockWidget(QStringLiteral("Параметры"), this);
    dock->setWidget(panel);
    dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    addDockWidget(Qt::RightDockWidgetArea, dock);

    //Табліца вылічэнняў
    m_table = new QTableWidget;
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    auto *tdock = new QDockWidget(QStringLiteral("Суправаджальныя вылічэнні"), this);
    tdock->setWidget(m_table);
    tdock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    addDockWidget(Qt::BottomDockWidgetArea, tdock);
    resizeDocks({tdock}, {220}, Qt::Vertical);

    //Меню
    QMenu *file = menuBar()->addMenu(QStringLiteral("Файл"));
    QAction *aSave = file->addAction(QStringLiteral("Захаваць растр як PNG…"));
    QAction *aExit = file->addAction(QStringLiteral("Выхад"));
    QAction *aBench = menuBar()->addAction(QStringLiteral("Бэнчмарк…"));
    connect(aSave, &QAction::triggered, this, &MainWindow::OnSavePng);
    connect(aExit, &QAction::triggered, this, &QWidget::close);
    connect(aBench, &QAction::triggered, this, &MainWindow::OnBenchmark);

    connect(bDraw, &QPushButton::clicked, this, &MainWindow::OnDraw);
    connect(bClear, &QPushButton::clicked, this, &MainWindow::OnClear);
    connect(bDemo, &QPushButton::clicked, this, &MainWindow::OnDemoOctants);
    connect(bBench, &QPushButton::clicked, this, &MainWindow::OnBenchmark);
    connect(bReset, &QPushButton::clicked, m_view, &GridPainter::ResetView);
    connect(m_algo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::OnAlgorithmChanged);
    connect(m_zoom, &QSlider::valueChanged, this, &MainWindow::OnZoomSlider);
    connect(m_view, &GridPainter::CellSizeChanged, this, &MainWindow::OnCellSizeChanged);
    connect(m_view, &GridPainter::CellHovered, this, &MainWindow::OnCellHovered);
    connect(m_view, &GridPainter::CellClicked, this, &MainWindow::OnCellClicked);

    OnAlgorithmChanged();
    OnDraw();
}

bool MainWindow::IsCircle() const { return m_algo->currentIndex() == Circle; }

void MainWindow::OnAlgorithmChanged()
{
    m_lineBox->setEnabled(!IsCircle());
    m_circleBox->setEnabled(IsCircle());
    m_clickPhase = 0;
    statusBar()->showMessage(m_algo->currentIndex() == Wu ? QStringLiteral("Ву: кардынаты могуць быць дробавымі — дробавая частка задае інтэнсіўнасць пікселяў.") : QStringLiteral("Для цэлалічбенных алгарытмаў кардынаты акругляюцца да целых."), 6000);
}

void MainWindow::OnZoomSlider(int v) { m_view->SetCellSize(v); }

void MainWindow::OnCellSizeChanged(double px)
{
    m_zoom->blockSignals(true);
    m_zoom->setValue(static_cast<int>(std::lround(px)));
    m_zoom->blockSignals(false);
}

void MainWindow::OnCellHovered(int x, int y, bool inside)
{
    if (!inside) {
        m_cursor->setText(QStringLiteral("Курсор: (%1; %2) — па-за растрам").arg(x).arg(y));
        return;
    }
    m_cursor->setText(QStringLiteral("Курсор: (%1; %2), інтэнсіўнасць %3").arg(x).arg(y).arg(m_grid.Get(x, y), 0, 'f', 2));
}

void MainWindow::OnCellClicked(int x, int y)
{
    if (IsCircle()) {
        if (m_clickPhase == 0) {
            m_cx->setValue(x); m_cy->setValue(y);
            m_clickPhase = 1;
        } else {
            m_r->setValue(RI(std::hypot(x - m_cx->value(), y - m_cy->value())));
            m_clickPhase = 0;
            OnDraw();
        }
    } else {
        if (m_clickPhase == 0) {
            m_x0->setValue(x); m_y0->setValue(y);
            m_clickPhase = 1;
        } else {
            m_x1->setValue(x); m_y1->setValue(y);
            m_clickPhase = 0;
            OnDraw();
        }
    }
}

void MainWindow::OnClear()
{
    m_grid.Clear();
    m_table->clear();
    m_table->setRowCount(0);
    m_table->setColumnCount(0);
    m_view->update();
}

void MainWindow::OnDraw()
{
    if (!m_accumulate->isChecked()) m_grid.Clear();
    DrawCurrent(true);
    m_view->update();
}

void MainWindow::DrawCurrent(bool allowTable)
{
    using clock = std::chrono::steady_clock;
    const int a = m_algo->currentIndex();
    const int x0 = RI(m_x0->value()), y0 = RI(m_y0->value());
    const int x1 = RI(m_x1->value()), y1 = RI(m_y1->value());
    const int cx = RI(m_cx->value()), cy = RI(m_cy->value()), r = m_r->value();

    const auto t0 = clock::now();
    switch (a) {
    case Step:      RasterEngine::StepLine(x0, y0, x1, y1, m_grid); break;
    case DDA:       RasterEngine::DDALine(x0, y0, x1, y1, m_grid); break;
    case Bresenham: RasterEngine::BresenhamLine(x0, y0, x1, y1, m_grid); break;
    case Wu:        RasterEngine::WuLine(m_x0->value(), m_y0->value(), m_x1->value(), m_y1->value(), m_grid); break;
    case Circle:    RasterEngine::BresenhamCircle(cx, cy, r, m_grid); break;
    }
    const double us = std::chrono::duration<double, std::micro>(clock::now() - t0).count();

    if (!allowTable) return;

    TraceTable t;
    switch (a) {
    case Step:      t = RasterTrace::Step(x0, y0, x1, y1); break;
    case DDA:       t = RasterTrace::DDA(x0, y0, x1, y1); break;
    case Bresenham: t = RasterTrace::Bresenham(x0, y0, x1, y1); break;
    case Wu:        t = RasterTrace::Wu(m_x0->value(), m_y0->value(), m_x1->value(), m_y1->value()); break;
    case Circle:    t = RasterTrace::Circle(cx, cy, r); break;
    }
    FillTable(t);
}

void MainWindow::FillTable(const TraceTable &t)
{
    m_table->clear();
    m_table->setColumnCount(static_cast<int>(t.header.size()));
    QStringList h;
    for (const auto &s : t.header) h << QString::fromStdString(s);
    m_table->setHorizontalHeaderLabels(h);
    m_table->setRowCount(static_cast<int>(t.rows.size()));
    for (int r = 0; r < static_cast<int>(t.rows.size()); ++r)
        for (int c = 0; c < static_cast<int>(t.rows[r].size()); ++c)
            m_table->setItem(r, c, new QTableWidgetItem(QString::fromStdString(t.rows[r][c])));
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setStretchLastSection(true);
    if (t.truncated)
        statusBar()->showMessage(statusBar()->currentMessage() + QStringLiteral(" Паказаныя першыя 2000 радкоў табліцы."));
}

void MainWindow::OnDemoOctants()
{
    if (!m_accumulate->isChecked()) m_grid.Clear();
    const int a = m_algo->currentIndex();
    const double PI = 3.14159265358979323846;
    if (a == Circle) {
        for (int r = 4; r <= 40; r += 6) RasterEngine::BresenhamCircle(0, 0, r, m_grid);
    } else {
        // 16 накірункаў праз 22.5 градуса
        const int R = 30;
        for (int k = 0; k < 16; ++k) {
            const double ang = (k * 22.5 + 11.25 / 2) * PI / 180.0;
            const double fx = R * std::cos(ang), fy = R * std::sin(ang);
            const int x1 = RI(fx), y1 = RI(fy);
            switch (a) {
            case Step:      RasterEngine::StepLine(0, 0, x1, y1, m_grid); break;
            case DDA:       RasterEngine::DDALine(0, 0, x1, y1, m_grid); break;
            case Bresenham: RasterEngine::BresenhamLine(0, 0, x1, y1, m_grid); break;
            case Wu:        RasterEngine::WuLine(0, 0, fx, fy, m_grid); break;
            }
        }
    }
    m_view->SetCellSize(10);
    statusBar()->showMessage(QStringLiteral("Дэма: промні з цэнтра ва ўсе 8 актантаў. Афарбавана пікселяў: %1").arg(m_grid.CountLit()));
    m_view->update();
}

void MainWindow::OnSavePng()
{
    const QString fn = QFileDialog::getSaveFileName(this, QStringLiteral("Захаваць растр"), "raster.png", "PNG (*.png)");
    if (!fn.isEmpty()) m_view->grab().save(fn);
}

void MainWindow::OnBenchmark()
{
    BenchmarkDialog dlg(this);
    dlg.exec();
}
