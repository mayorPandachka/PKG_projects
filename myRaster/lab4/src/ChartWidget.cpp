#include "ChartWidget.h"

#include <QPainter>
#include <algorithm>
#include <cmath>

ChartWidget::ChartWidget(QWidget *parent) : QWidget(parent)
{
    setMinimumSize(520, 340);
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::white);
    setPalette(pal);
}

void ChartWidget::SetTable(const BenchTable &t)
{
    m_table = t;
    m_has = !t.x.empty();
    update();
}

namespace {
double NiceCeil(double v)
{
    if (v <= 0) return 1;
    const double e = std::pow(10.0, std::floor(std::log10(v)));
    const double f = v / e;
    const double n = f <= 1 ? 1 : f <= 2 ? 2 : f <= 2.5 ? 2.5 : f <= 5 ? 5 : 10;
    return n * e;
}
}

void ChartWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.fillRect(rect(), Qt::white);
    if (!m_has) {
        p.drawText(rect(), Qt::AlignCenter, QStringLiteral("Нет звесткаў. Запусціце бэнчмарк."));
        return;
    }
    static const QColor colors[] = {QColor(31, 119, 180), QColor(214, 39, 40), QColor(44, 160, 44), QColor(148, 103, 189)};

    const int L = 70, R = 20, T = 40, B = 60;
    const QRectF plot(L, T, std::max(50, width() - L - R), std::max(50, height() - T - B));

    double xmax = 0, ymax = 0;
    for (double x : m_table.x) xmax = std::max(xmax, x);
    for (const auto &s : m_table.series)
        for (double v : s.seconds) ymax = std::max(ymax, v * 1000.0);
    xmax = NiceCeil(xmax);
    ymax = NiceCeil(ymax * 1.05);

    p.setPen(Qt::black);
    QFont tf = p.font(); tf.setBold(true); p.setFont(tf);
    p.drawText(QRectF(0, 6, width(), 24), Qt::AlignCenter, QString::fromStdString(m_table.title));
    tf.setBold(false); p.setFont(tf);

    // Сетка и подписи
    const int nx = 5, ny = 5;
    for (int i = 0; i <= ny; ++i) {
        const double y = plot.bottom() - plot.height() * i / ny;
        p.setPen(QColor(225, 225, 225));
        p.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));
        p.setPen(Qt::black);
        p.drawText(QRectF(0, y - 9, L - 6, 18), Qt::AlignRight | Qt::AlignVCenter, QString::number(ymax * i / ny, 'g', 4));
    }
    for (int i = 0; i <= nx; ++i) {
        const double x = plot.left() + plot.width() * i / nx;
        p.setPen(QColor(225, 225, 225));
        p.drawLine(QPointF(x, plot.top()), QPointF(x, plot.bottom()));
        p.setPen(Qt::black);
        p.drawText(QRectF(x - 40, plot.bottom() + 4, 80, 18), Qt::AlignCenter, QString::number(xmax * i / nx, 'g', 6));
    }
    p.setPen(QPen(Qt::black, 1.5));
    p.drawRect(plot);
    p.drawText(QRectF(plot.left(), plot.bottom() + 24, plot.width(), 20), Qt::AlignCenter, QString::fromStdString(m_table.xLabel));
    p.save();
    p.translate(14, plot.center().y());
    p.rotate(-90);
    p.drawText(QRectF(-80, -10, 160, 20), Qt::AlignCenter, QStringLiteral("Время, мс"));
    p.restore();

    // Серии
    int idx = 0;
    for (const auto &s : m_table.series) {
        const QColor c = colors[idx % 4];
        p.setPen(QPen(c, 2));
        p.setBrush(c);
        QPointF prev;
        for (size_t i = 0; i < s.seconds.size() && i < m_table.x.size(); ++i) {
            const QPointF pt(plot.left() + plot.width() * m_table.x[i] / xmax,
                             plot.bottom() - plot.height() * (s.seconds[i] * 1000.0) / ymax);
            if (i > 0) p.drawLine(prev, pt);
            p.drawEllipse(pt, 3, 3);
            prev = pt;
        }
        // легенда
        const double ly = T + 8 + idx * 18;
        p.drawLine(QPointF(plot.left() + 12, ly), QPointF(plot.left() + 36, ly));
        p.setPen(Qt::black);
        p.drawText(QPointF(plot.left() + 42, ly + 4), QString::fromStdString(s.name));
        ++idx;
    }
}
