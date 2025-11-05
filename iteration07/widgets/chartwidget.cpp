#include "chartwidget.h"
#include <QPainter>
#include "../utils/theme.h"
#include <QPainterPath>

ChartWidget::ChartWidget(const QString &title,
                         QWidget *parent)
    : QWidget(parent)
    , m_title(title)
    , m_maxDataPoints(60)
    , m_minY(0.0)
    , m_maxY(100.0)
    , m_lineColor(QColor("#2196F3"))
{
    m_fillColor = m_lineColor;
    m_fillColor.setAlpha(80);
    setMinimumHeight(250);
    
    // Initialize data points vector with zeros
    m_dataPoints.fill(0.0, m_maxDataPoints);
}

void ChartWidget::addDataPoint(double value)
{
    for (int i = 0; i < m_dataPoints.size() - 1; ++i) {
        m_dataPoints[i] = m_dataPoints[i + 1];
    }

    if (!m_dataPoints.isEmpty()) {
        m_dataPoints.last() = value;
    }

    update();
}

void ChartWidget::setColor(const QColor &color)
{
    m_lineColor = color;
    m_fillColor = color;
    m_fillColor.setAlpha(80);
    update();
}

void ChartWidget::setYAxisRange(double min, double max)
{
    m_minY = min;
    m_maxY = max;
    update();
}


void ChartWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Calculate drawing area
    const int chartWidth = width() - MARGIN_LEFT - MARGIN_RIGHT;
    const int chartHeight = height() - MARGIN_TOP - MARGIN_BOTTOM;

    if (chartWidth <= 0 || chartHeight <= 0) {
        return; // Widget too small
    }

    const QRect chartRect(MARGIN_LEFT, MARGIN_TOP, chartWidth, chartHeight);

    // Setup theme colors
    QPalette pal = palette();
    //const bool isDark = (pal.color(QPalette::Window).lightness() < 128);
    const bool isDark = Theme::isDarkMode();

    const QColor bgColor = isDark ? QColor("#2C2C2C") : Qt::white;
    const QColor chartBgColor = isDark ? QColor("#1E1E1E") : QColor("#FAFAFA");
    const QColor gridColor = isDark ? QColor("#404040") : QColor("#D0D0D0");
    const QColor textColor = isDark ? QColor("#E0E0E0") : QColor("#424242");
    const QColor labelColor = isDark ? QColor("#B0B0B0") : QColor("#757575");
    const QColor borderColor = isDark ? QColor("#606060") : QColor("#BDBDBD");

    // Draw background
    painter.fillRect(rect(), bgColor);

    // Draw title
    painter.setPen(textColor);
    painter.setFont(QFont(painter.font().family(), 12, QFont::Bold));
    painter.drawText(MARGIN_LEFT, 25, m_title);

    // Draw chart background
    painter.fillRect(chartRect, chartBgColor);

    // Draw grid lines and Y-axis labels
    painter.setFont(QFont(painter.font().family(), 8));
    QFontMetrics fm(painter.font());

    for (int i = 0; i <= GRID_LINES; ++i) {
        const int y = chartRect.top() + (chartRect.height() * i) / GRID_LINES;

        // Draw horizontal grid line
        painter.setPen(QPen(gridColor, 1));
        painter.drawLine(chartRect.left(), y, chartRect.right(), y);

        // Draw Y-axis label
        const double value = m_maxY - ((m_maxY - m_minY) * i) / GRID_LINES;
        const QString label = QString::number(value, 'f', 1);
        const int labelWidth = fm.horizontalAdvance(label);

        painter.setPen(labelColor);
        painter.drawText(chartRect.left() - labelWidth - 8, y + 4, label);
    }

    // Draw vertical grid lines
    painter.setPen(QPen(gridColor, 1));
    const int verticalLines = 6;
    for (int i = 0; i <= verticalLines; ++i) {
        const int x = chartRect.left() + (chartRect.width() * i) / verticalLines;
        painter.drawLine(x, chartRect.top(), x, chartRect.bottom());
    }

    // Draw chart border
    painter.setPen(QPen(borderColor, 1));
    painter.drawRect(chartRect);

    // Calculate data points
    QVector<QPointF> points;
    const double xStep = static_cast<double>(chartWidth) / (m_maxDataPoints - 1);
    const double yRange = m_maxY - m_minY;

    for (int i = 0; i < m_dataPoints.size(); ++i) {
        const double x = MARGIN_LEFT + i * xStep;
        const double normalizedValue = qBound(0.0, (m_dataPoints[i] - m_minY) / yRange, 1.0);
        const double y = MARGIN_TOP + chartHeight - (normalizedValue * chartHeight);
        points.append(QPointF(x, y));
    }


    // Draw filled area under the line
    if (points.size() >= 2) {
        QPainterPath fillPath;
        fillPath.moveTo(points.first().x(), MARGIN_TOP + chartHeight);
        for (const QPointF &point : points) {
            fillPath.lineTo(point);
        }
        fillPath.lineTo(points.last().x(), MARGIN_TOP + chartHeight);
        fillPath.closeSubpath();

        QLinearGradient gradient(0, MARGIN_TOP, 0, MARGIN_TOP + chartHeight);
        QColor gradientStart = m_fillColor;
        gradientStart.setAlpha(120);
        QColor gradientEnd = m_fillColor;
        gradientEnd.setAlpha(20);
        gradient.setColorAt(0, gradientStart);
        gradient.setColorAt(1, gradientEnd);

        painter.fillPath(fillPath, gradient);
    }

    // Draw data line with smooth curves
    if (points.size() >= 2) {
        QPainterPath linePath;
        linePath.moveTo(points.first());

        for (int i = 0; i < points.size() - 1; ++i) {
            const QPointF &p1 = points[i];
            const QPointF &p2 = points[i + 1];
            const QPointF ctrl((p1.x() + p2.x()) / 2.0, (p1.y() + p2.y()) / 2.0);
            linePath.quadTo(p1, ctrl);
            linePath.lineTo(p2);
        }

        painter.setPen(QPen(m_lineColor, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawPath(linePath);
    }


    // Draw data point markers
    painter.setPen(Qt::NoPen);
    painter.setBrush(m_lineColor);
    for (int i = 0; i < points.size(); i += 10) {
        if (m_dataPoints[i] > 0.1) {
            painter.drawEllipse(points[i], 3, 3);
        }
    }

    // Draw current value indicator
    const double currentValue = m_dataPoints.last();
    const QString valueText = QString::number(currentValue, 'f', 1) + "%";

    QFont valueFont(painter.font().family(), 10, QFont::Bold);
    QFontMetrics valueFm(valueFont);
    const int textWidth = valueFm.horizontalAdvance(valueText);
    const int textHeight = valueFm.height();

    // Draw Badge
    const QRect valueBox(width() - MARGIN_RIGHT - textWidth - 20, 10, textWidth + 16, textHeight + 8);

    painter.setPen(Qt::NoPen);
    painter.setBrush(m_lineColor);
    painter.drawRoundedRect(valueBox, 4, 4);

    painter.setPen(Qt::white);
    painter.setFont(valueFont);
    painter.drawText(valueBox, Qt::AlignCenter, valueText);

    // Draw axis labels
    painter.setPen(labelColor);
    painter.setFont(QFont(painter.font().family(), 9));
    painter.drawText(MARGIN_LEFT + chartWidth / 2 - 50, height() - 10, "Time (Last 60s)");

    painter.save();
    painter.translate(15, MARGIN_TOP + chartHeight / 2.0);
    painter.rotate(-90);
    painter.drawText(-30, 0, "Usage %");
    painter.restore();
}

void ChartWidget::updateTheme()
{
    update(); // Trigger repaint
}

void ChartWidget::clear()
{
    m_dataPoints.fill(0.0, m_maxDataPoints);
    update();
}

QSize ChartWidget::sizeHint() const
{
    return QSize(600, 300);
}

QSize ChartWidget::minimumSizeHint() const
{
    return QSize(300, 200);
}
