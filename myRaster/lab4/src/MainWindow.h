#pragma once
#include "RasterTrace.h"
#include "VirtualGrid.h"
#include <QMainWindow>

class GridPainter;
class QComboBox;
class QDoubleSpinBox;
class QSpinBox;
class QSlider;
class QCheckBox;
class QLabel;
class QTableWidget;
class QGroupBox;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void OnDraw();
    void OnClear();
    void OnDemoOctants();
    void OnAlgorithmChanged();
    void OnZoomSlider(int value);
    void OnCellSizeChanged(double px);
    void OnCellHovered(int x, int y, bool inside);
    void OnCellClicked(int x, int y);
    void OnSavePng();
    void OnBenchmark();

private:
    enum Algo { Step = 0, DDA, Bresenham, Wu, Circle };

    void DrawCurrent(bool allowTable);
    void FillTable(const TraceTable &t);
    bool IsCircle() const;

    VirtualGrid m_grid;
    GridPainter *m_view;
    QComboBox *m_algo;
    QDoubleSpinBox *m_x0, *m_y0, *m_x1, *m_y1, *m_cx, *m_cy;
    QSpinBox *m_r;
    QGroupBox *m_lineBox, *m_circleBox;
    QCheckBox *m_accumulate;
    QSlider *m_zoom;
    QLabel *m_info, *m_cursor;
    QTableWidget *m_table;
    int m_clickPhase = 0;
};
