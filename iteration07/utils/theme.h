#ifndef THEME_H
#define THEME_H

#include <QPalette>
#include <QColor>
#include <QString>
#include <QApplication>

class Theme
{
public:
    // Theme detection
    static bool isDarkMode()
    {
        QPalette pal = QApplication::palette();
        return pal.color(QPalette::Window).lightness() < 128;
    }

    // Palette creation
    static QPalette createDarkPalette()
    {
        QPalette darkPalette;
        darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
        darkPalette.setColor(QPalette::WindowText, Qt::white);
        darkPalette.setColor(QPalette::Base, QColor(42, 42, 42));
        darkPalette.setColor(QPalette::AlternateBase, QColor(66, 66, 66));
        darkPalette.setColor(QPalette::ToolTipBase, QColor(53, 53, 53));
        darkPalette.setColor(QPalette::ToolTipText, Qt::white);
        darkPalette.setColor(QPalette::Text, Qt::white);
        darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
        darkPalette.setColor(QPalette::ButtonText, Qt::white);
        darkPalette.setColor(QPalette::BrightText, Qt::red);
        darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
        darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
        darkPalette.setColor(QPalette::HighlightedText, Qt::black);
        return darkPalette;
    }

    static QPalette createLightPalette()
    {
        QPalette lightPalette;
        lightPalette.setColor(QPalette::Window, Qt::white);
        lightPalette.setColor(QPalette::WindowText, Qt::black);
        lightPalette.setColor(QPalette::Base, Qt::white);
        lightPalette.setColor(QPalette::AlternateBase, QColor(245, 245, 245));
        lightPalette.setColor(QPalette::ToolTipBase, Qt::white);
        lightPalette.setColor(QPalette::ToolTipText, Qt::black);
        lightPalette.setColor(QPalette::Text, Qt::black);
        lightPalette.setColor(QPalette::Button, QColor(240, 240, 240));
        lightPalette.setColor(QPalette::ButtonText, Qt::black);
        lightPalette.setColor(QPalette::BrightText, Qt::red);
        lightPalette.setColor(QPalette::Link, QColor(0, 0, 255));
        lightPalette.setColor(QPalette::Highlight, QColor(0, 120, 215));
        lightPalette.setColor(QPalette::HighlightedText, Qt::white);
        return lightPalette;
    }

    // Background colors
    static constexpr const char* DARK_BACKGROUND = "#2B2B2B";
    static constexpr const char* LIGHT_BACKGROUND = "#FAFAFA";

    // Tab Widget Styles
    static QString getDarkTabWidgetStyle()
    {
        return "QTabWidget::pane {"
               "    border: 1px solid #404040;"
               "    background: #2C2C2C;"
               "    border-radius: 4px;"
               "}"
               "QTabBar::tab {"
               "    background: #1E1E1E;"
               "    color: #E0E0E0;"
               "    border: 1px solid #404040;"
               "    padding: 8px 16px;"
               "    margin-right: 2px;"
               "}"
               "QTabBar::tab:selected {"
               "    background: #2C2C2C;"
               "    border-bottom-color: #2C2C2C;"
               "    font-weight: bold;"
               "}";
    }

    static QString getLightTabWidgetStyle()
    {
        return "QTabWidget::pane {"
               "    border: 1px solid #E0E0E0;"
               "    background: white;"
               "    border-radius: 4px;"
               "}"
               "QTabBar::tab {"
               "    background: #FAFAFA;"
               "    color: #424242;"
               "    border: 1px solid #E0E0E0;"
               "    padding: 8px 16px;"
               "    margin-right: 2px;"
               "}"
               "QTabBar::tab:selected {"
               "    background: white;"
               "    border-bottom-color: white;"
               "    font-weight: bold;"
               "}";
    }

    // Menu Bar Styles
    static QString getDarkMenuBarStyle()
    {
        return "QMenuBar {"
               "    background-color: #2C2C2C;"
               "    color: #E0E0E0;"
               "}"
               "QMenuBar::item {"
               "    background-color: transparent;"
               "    padding: 4px 8px;"
               "}"
               "QMenuBar::item:selected {"
               "    background-color: #404040;"
               "}"
               "QMenu {"
               "    background-color: #2C2C2C;"
               "    color: #E0E0E0;"
               "    border: 1px solid #404040;"
               "}"
               "QMenu::item {"
               "    padding: 6px 20px;"
               "}"
               "QMenu::item:selected {"
               "    background-color: #404040;"
               "}";
    }

    static QString getLightMenuBarStyle()
    {
        return "QMenuBar {"
               "    background-color: #F0F0F0;"
               "    color: #000000;"
               "}"
               "QMenuBar::item {"
               "    background-color: transparent;"
               "    padding: 4px 8px;"
               "}"
               "QMenuBar::item:selected {"
               "    background-color: #E0E0E0;"
               "}"
               "QMenu {"
               "    background-color: white;"
               "    color: #000000;"
               "    border: 1px solid #C0C0C0;"
               "}"
               "QMenu::item {"
               "    padding: 6px 20px;"
               "}"
               "QMenu::item:selected {"
               "    background-color: #2196F3;"
               "    color: white;"
               "}";
    }
};

#endif // THEME_H
