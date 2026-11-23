#include "GridPainter.h"
#include "VirtualGrid.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPolygonF>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>

namespace {
const int kLeft = 46, kBottom = 26, kTop = 8, kRight = 14;
}

GridPainter::GridPainter(QWidget *parent) : QWidget(parent)
{
    setMouseTracking(true);
    setMinimumSize(400, 300);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void GridPainter::SetGrid(const VirtualGrid *grid)
{
    m_grid = grid;
    update();
}

void GridPainter::SetCellSize(double px)
{
    px = std::max(2.0, std::min(120.0, px));
    if (std::fabs(px - m_cell) < 1e-9) return;
    m_cell = px;
    emit CellSizeChanged(m_cell);
    update();
}

void GridPainter::ResetView()
{
    m_center = QPointF(0.0, 0.0);
    SetCellSize(16.0);
    update();
}

QRectF GridPainter::PlotRect() const
{
    return QRectF(kLeft, kTop, std::max(10, width() - kLeft - kRight), std::max(10, height() - kTop - kBottom));
}

QPointF GridPainter::ToScreen(double x, double y) const
{
    const QRectF pr = PlotRect();
    const QPointF pc = pr.center();
    return QPointF(pc.x() + (x - m_center.x()) * m_cell, pc.y() - (y - m_center.y()) * m_cell);
}

QPointF GridPainter::FromScreen(const QPointF &p) const
{
    const QRectF pr = PlotRect();
    const QPointF pc = pr.center();
    return QPointF(m_center.x() + (p.x() - pc.x()) / m_cell, m_center.y() - (p.y() - pc.y()) / m_cell);
}

int GridPainter::TickStep() const
{
    static const int steps[] = {1, 2, 5, 10, 20, 50, 100, 200, 500};
    for (int s : steps)
        if (s * m_cell >= 36.0) return s;
    return 1000;
}

void GridPainter::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(), QColor(238, 238, 238));
    const QRectF pr = PlotRect();

    p.save();
    p.setClipRect(pr);
    p.fillRect(pr, QColor(246, 246, 246));

    //Межы бачнай обласці ў булевых кардынатах
    const QPointF tl = FromScreen(pr.topLeft());
    const QPointF br = FromScreen(pr.bottomRight());
    const int vx0 = static_cast<int>(std::floor(tl.x())) - 1;
    const int vx1 = static_cast<int>(std::ceil(br.x())) + 1;
    const int vy0 = static_cast<int>(std::floor(br.y())) - 1;
    const int vy1 = static_cast<int>(std::ceil(tl.y())) + 1;

    //Вобласць віртуальнага растра - белая
    if (m_grid) {
        const QPointF a = ToScreen(m_grid->MinX() - 0.5, m_grid->MaxY() + 0.5);
        const QPointF b = ToScreen(m_grid->MaxX() + 0.5, m_grid->MinY() - 0.5);
        p.fillRect(QRectF(a, b), Qt::white);
    }

    //Афарбаваныя пікселі
    if (m_grid) {
        const int x0 = std::max(vx0, m_grid->MinX()), x1 = std::min(vx1, m_grid->MaxX());
        const int y0 = std::max(vy0, m_grid->MinY()), y1 = std::min(vy1, m_grid->MaxY());
        const bool showValues = m_cell >= 44.0;
        QFont f = p.font();
        f.setPointSizeF(std::max(6.0, std::min(10.0, m_cell / 6.0)));
        for (int y = y0; y <= y1; ++y) {
            for (int x = x0; x <= x1; ++x) {
                const double v = m_grid->Get(x, y);
                if (v <= 0.0) continue;
                const QPointF c = ToScreen(x, y);
                const QRectF cell(c.x() - m_cell / 2, c.y() - m_cell / 2, m_cell, m_cell);
                QColor col(25, 90, 200);
                col.setAlphaF(std::min(1.0, v));
                p.fillRect(cell, col);
                if (showValues) {
                    p.setFont(f);
                    p.setPen(v > 0.55 ? Qt::white : QColor(20, 20, 20));
                    p.drawText(cell, Qt::AlignCenter, QString::number(v, 'f', 2));
                }
            }
        }
    }

    // Линиі сеткі па межах клетак (паўцэлыя значэнні)
    if (m_cell >= 5.0) {
        const int step = TickStep();
        for (int i = vx0; i <= vx1; ++i) {
            const double sx = ToScreen(i - 0.5, 0).x();
            const bool major = (i % step == 0) && step > 1;
            p.setPen(QPen(major ? QColor(185, 185, 185) : QColor(220, 220, 220), 1));
            p.drawLine(QPointF(sx, pr.top()), QPointF(sx, pr.bottom()));
        }
        for (int j = vy0; j <= vy1; ++j) {
            const double sy = ToScreen(0, j - 0.5).y();
            const bool major = (j % step == 0) && step > 1;
            p.setPen(QPen(major ? QColor(185, 185, 185) : QColor(220, 220, 220), 1));
            p.drawLine(QPointF(pr.left(), sy), QPointF(pr.right(), sy));
        }
    }

    //Мяжа віртуальнага растра
    if (m_grid) {
        const QPointF a = ToScreen(m_grid->MinX() - 0.5, m_grid->MaxY() + 0.5);
        const QPointF b = ToScreen(m_grid->MaxX() + 0.5, m_grid->MinY() - 0.5);
        p.setPen(QPen(QColor(200, 60, 60), 1, Qt::DashLine));
        p.setBrush(Qt::NoBrush);
        p.drawRect(QRectF(a, b));
    }

    //Восі кардынатаў: праз цэнтры клетак x = 0 і y = 0
    const QPointF o = ToScreen(0, 0);
    p.setPen(QPen(QColor(20, 20, 20), 2));
    p.drawLine(QPointF(pr.left(), o.y()), QPointF(pr.right(), o.y()));
    p.drawLine(QPointF(o.x(), pr.top()), QPointF(o.x(), pr.bottom()));
    //стрэлкі
    QPolygonF ax; ax << QPointF(pr.right() - 1, o.y()) << QPointF(pr.right() - 11, o.y() - 4) << QPointF(pr.right() - 11, o.y() + 4);
    QPolygonF ay; ay << QPointF(o.x(), pr.top() + 1) << QPointF(o.x() - 4, pr.top() + 11) << QPointF(o.x() + 4, pr.top() + 11);
    p.setBrush(QColor(20, 20, 20));
    p.drawPolygon(ax);
    p.drawPolygon(ay);
    p.setPen(QColor(20, 20, 20));
    p.drawText(QPointF(pr.right() - 14, o.y() - 8), "x");
    p.drawText(QPointF(o.x() + 8, pr.top() + 14), "y");

    //Лінейки с подпісамі: лічбы насупраць цэнтраў клетак
    p.setPen(QColor(60, 60, 60));
    p.setFont(QFont(p.font().family(), 8));
    const int step = TickStep();
    const int sx0 = (vx0 / step) * step - step, sy0 = (vy0 / step) * step - step;
    for (int i = sx0; i <= vx1; i += step) {
        const double sx = ToScreen(i, 0).x();
        if (sx < pr.left() - 1 || sx > pr.right() + 1) continue;
        p.drawLine(QPointF(sx, pr.bottom()), QPointF(sx, pr.bottom() + 4));
        p.drawText(QRectF(sx - 24, pr.bottom() + 5, 48, 16), Qt::AlignCenter, QString::number(i));
    }
    for (int j = sy0; j <= vy1; j += step) {
        const double sy = ToScreen(0, j).y();
        if (sy < pr.top() - 1 || sy > pr.bottom() + 1) continue;
        p.drawLine(QPointF(pr.left() - 4, sy), QPointF(pr.left(), sy));
        p.drawText(QRectF(0, sy - 8, kLeft - 6, 16), Qt::AlignRight | Qt::AlignVCenter, QString::number(j));
    }
    p.setPen(QPen(QColor(120, 120, 120), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRect(pr);
}

void GridPainter::wheelEvent(QWheelEvent *e)
{
    const QPointF m = e->position();
    const QPointF lp = FromScreen(m);
    const double factor = e->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15;
    const double old = m_cell;
    m_cell = std::max(2.0, std::min(120.0, m_cell * factor));
    if (std::fabs(old - m_cell) < 1e-9) return;
    const QPointF pc = PlotRect().center();
    m_center = QPointF(lp.x() - (m.x() - pc.x()) / m_cell, lp.y() + (m.y() - pc.y()) / m_cell);
    emit CellSizeChanged(m_cell);
    update();
}

void GridPainter::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::RightButton || e->button() == Qt::MiddleButton) {
        m_panning = true;
        m_lastPos = e->position().toPoint();
        setCursor(Qt::ClosedHandCursor);
    } else if (e->button() == Qt::LeftButton && PlotRect().contains(e->position())) {
        const QPointF lp = FromScreen(e->position());
        emit CellClicked(static_cast<int>(std::lround(lp.x())), static_cast<int>(std::lround(lp.y())));
    }
}

void GridPainter::mouseMoveEvent(QMouseEvent *e)
{
    const QPoint pos = e->position().toPoint();
    if (m_panning) {
        const QPoint d = pos - m_lastPos;
        m_lastPos = pos;
        m_center -= QPointF(d.x() / m_cell, -d.y() / m_cell);
        update();
        return;
    }
    if (PlotRect().contains(e->position())) {
        const QPointF lp = FromScreen(e->position());
        m_hoverX = static_cast<int>(std::lround(lp.x()));
        m_hoverY = static_cast<int>(std::lround(lp.y()));
        m_hasHover = true;
        emit CellHovered(m_hoverX, m_hoverY, m_grid && m_grid->Contains(m_hoverX, m_hoverY));
    } else {
        m_hasHover = false;
    }
    update();
}

void GridPainter::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::RightButton || e->button() == Qt::MiddleButton) {
        m_panning = false;
        unsetCursor();
    }
}

void GridPainter::leaveEvent(QEvent *)
{
    m_hasHover = false;
    update();
}
