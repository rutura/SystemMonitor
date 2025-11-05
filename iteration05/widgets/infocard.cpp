#include "infocard.h"
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>

InfoCard::InfoCard(const QString &title, QWidget *parent)
    : QWidget(parent)
    , m_title(title)
    , m_iconLabel(new QLabel(this))
    , m_titleLabel(new QLabel(this))
    , m_valueLabel(new QLabel(this))
    , m_subtitleLabel(new QLabel(this))
    , m_progressBar(new QProgressBar(this))
{
    setupUI();
}

void InfoCard::setupUI()
{
    // Main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(6);

    // Header layout (icon + title)
    QHBoxLayout *headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(6);

    m_iconLabel->setFixedSize(24, 24);
    m_iconLabel->setScaledContents(false);
    m_iconLabel->setAlignment(Qt::AlignCenter);
    QFont iconFont;
    iconFont.setPointSize(16);
    m_iconLabel->setFont(iconFont);

    // Add glow effect for dark mode visibility
    QGraphicsDropShadowEffect *iconGlow = new QGraphicsDropShadowEffect();
    iconGlow->setBlurRadius(15);
    iconGlow->setColor(QColor(255, 255, 255, 180));
    iconGlow->setOffset(0, 0);
    m_iconLabel->setGraphicsEffect(iconGlow);

    // Add background to icon label for better visibility
    m_iconLabel->setStyleSheet(
        "QLabel {"
        "    background-color: #808080;"
        "    border-radius: 4px;"
        "    padding: 2px;"
        "}"
        );

    headerLayout->addWidget(m_iconLabel);

    m_titleLabel->setText(m_title);
    m_titleLabel->setStyleSheet("font-size: 11pt; font-weight: bold;");
    headerLayout->addWidget(m_titleLabel);
    headerLayout->addStretch();

    mainLayout->addLayout(headerLayout);

    // Value label
    m_valueLabel->setText("0%");
    m_valueLabel->setStyleSheet("font-size: 20pt; font-weight: bold;");
    m_valueLabel->setAlignment(Qt::AlignLeft);
    mainLayout->addWidget(m_valueLabel);

    // Subtitle label
    m_subtitleLabel->setStyleSheet("font-size: 9pt;");
    m_subtitleLabel->setAlignment(Qt::AlignLeft);
    mainLayout->addWidget(m_subtitleLabel);

    // Progress bar
    m_progressBar->setMinimum(0);
    m_progressBar->setMaximum(100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(false);
    m_progressBar->setFixedHeight(6);
    mainLayout->addWidget(m_progressBar);

    mainLayout->addStretch();

    setMinimumSize(220, 120);
    setMaximumHeight(130);

    // Add shadow effect
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect();
    shadow->setBlurRadius(10);
    shadow->setColor(QColor(0, 0, 0, 30));
    shadow->setOffset(0, 2);
    setGraphicsEffect(shadow);

    // Apply initial theme
    applyTheme();
}


void InfoCard::applyTheme()
{
    QPalette pal = palette();
    bool isDark = (pal.color(QPalette::Window).lightness() < 128);

    QString bgColor = isDark ? "#2C2C2C" : "white";
    QString borderColor = isDark ? "#404040" : "#E0E0E0";
    QString progressBg = isDark ? "#404040" : "#E0E0E0";

    // Update icon background and glow effect based on theme
    if (m_iconLabel) {
        QString iconBgColor = isDark ? "#A0A0A0" : "#909090";
        m_iconLabel->setStyleSheet(
            "QLabel {"
            "    background-color: " + iconBgColor + ";"
                            "    border-radius: 4px;"
                            "    padding: 2px;"
                            "}"
            );

        if (m_iconLabel->graphicsEffect()) {
            QGraphicsDropShadowEffect *iconGlow = qobject_cast<QGraphicsDropShadowEffect*>(m_iconLabel->graphicsEffect());
            if (iconGlow) {
                if (isDark) {
                    // Strong white glow for dark mode
                    iconGlow->setBlurRadius(20);
                    iconGlow->setColor(QColor(255, 255, 255, 200));
                } else {
                    // Subtle shadow for light mode
                    iconGlow->setBlurRadius(8);
                    iconGlow->setColor(QColor(0, 0, 0, 60));
                }
            }
        }
    }

    // Card styling
    setStyleSheet(
        "InfoCard {"
        "    background-color: " + bgColor + ";"
                    "    border-radius: 8px;"
                    "    border: 1px solid " + borderColor + ";"
                        "}"
        );

    m_progressBar->setStyleSheet(
        "QProgressBar {"
        "    border: none;"
        "    border-radius: 4px;"
        "    background-color: " + progressBg + ";"
                       "    text-align: center;"
                       "}"
        );
}

void InfoCard::setValue(const QString &value)
{
    m_valueLabel->setText(value);
}

void InfoCard::setPercentage(double percent)
{
    m_progressBar->setValue(static_cast<int>(percent));

    // Auto-update color based on percentage
    QColor barColor;
    if (percent < 60.0) {
        barColor = QColor("#4CAF50"); // Green
    } else if (percent < 80.0) {
        barColor = QColor("#FFC107"); // Amber
    } else {
        barColor = QColor("#F44336"); // Red
    }

    // Apply color to progress bar
    m_progressBar->setStyleSheet(
        QString("QProgressBar::chunk { background-color: %1; border-radius: 3px; }")
        .arg(barColor.name())
    );
}

void InfoCard::setIcon(const QIcon &icon)
{
    m_iconLabel->setPixmap(icon.pixmap(24, 24));
    m_iconLabel->setAlignment(Qt::AlignCenter);
}

void InfoCard::setIconText(const QString &iconText)
{
    m_iconLabel->setText(iconText);
}

void InfoCard::updateTheme()
{
    applyTheme();
}

void InfoCard::setSubtitle(const QString &subtitle)
{
    m_subtitleLabel->setText(subtitle);
}



