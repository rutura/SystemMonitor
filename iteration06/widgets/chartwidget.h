#ifndef CHARTWIDGET_H
#define CHARTWIDGET_H

#include <QWidget>
#include <QVector>
#include <QColor>
#include <QString>

class ChartWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ChartWidget(const QString &title, QWidget *parent = nullptr);

    void addDataPoint(double value);
    void setMaxDataPoints(int max);
    void setYAxisRange(double min, double max);
    void setColor(const QColor &color);
    void clear();
    void updateTheme();

protected:
    void paintEvent(QPaintEvent *event) override;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

private:
    QString m_title;
    QVector<double> m_dataPoints;
    int m_maxDataPoints;
    double m_minY;
    double m_maxY;
    QColor m_lineColor;
    QColor m_fillColor;

    // Drawing parameters
    static constexpr int MARGIN_LEFT = 60;
    static constexpr int MARGIN_RIGHT = 20;
    static constexpr int MARGIN_TOP = 50;
    static constexpr int MARGIN_BOTTOM = 40;
    static constexpr int GRID_LINES = 5;
};

#endif // CHARTWIDGET_H
