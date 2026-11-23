#pragma once
#include <QPointF>
#include <QWidget>

class VirtualGrid;

// Компонент UI: рисует увеличенные квадраты (пиксели) по данным VirtualGrid,
// а также сетку, оси координат и подписи. Сам алгоритмы не вызывает.
//
// Привязка целочисленных координат к сетке:
//  * клетка (x, y) - это пиксель; целые x, y - координаты ЦЕНТРА клетки;
//  * линии сетки проходят по полуцелым значениям (x +- 0.5);
//  * оси X и Y проходят через центры клеток с x = 0 и y = 0;
//  * подписи на линейках стоят напротив центров клеток.
class GridPainter : public QWidget
{
    Q_OBJECT
public:
    explicit GridPainter(QWidget *parent = nullptr);

    void SetGrid(const VirtualGrid *grid);
    double CellSize() const { return m_cell; }
    void SetCellSize(double px);          // масштаб: пикселей экрана на одну клетку
    void ResetView();

signals:
    void CellHovered(int x, int y, bool insideRaster);
    void CellClicked(int x, int y);
    void CellSizeChanged(double px);

protected:
    void paintEvent(QPaintEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void leaveEvent(QEvent *e) override;

private:
    QRectF PlotRect() const;
    QPointF ToScreen(double x, double y) const;
    QPointF FromScreen(const QPointF &p) const;
    int TickStep() const;

    const VirtualGrid *m_grid = nullptr;
    double m_cell = 16.0;
    QPointF m_center{0.0, 0.0};     // логическая точка в центре области рисования
    bool m_panning = false;
    QPoint m_lastPos;
    int m_hoverX = 0, m_hoverY = 0;
    bool m_hasHover = false;
};
