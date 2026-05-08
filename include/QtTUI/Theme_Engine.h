#ifndef QTTUI_THEME_ENGINE_H
#define QTTUI_THEME_ENGINE_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/ScreenBuffer.h>
#include <QObject>
#include <QHash>
#include <QString>
#include <QChar>

namespace QtTUI {

class QTuiWidget;

struct ThemeProperties {
    Color foreground = Color(255, 255, 255);
    Color background = Color(0, 0, 0);
    Color focusForeground = Color(255, 255, 255);
    Color focusBackground = Color(0, 80, 160);
    Color disabledForeground = Color(128, 128, 128);
    Color disabledBackground = Color(0, 0, 0);
    QString borderStyle = QStringLiteral("single");
    bool bold = false;
    bool italic = false;
    bool underline = false;
};

class Theme_Engine : public QObject {
    Q_OBJECT
public:
    static Theme_Engine *instance();

    // Theme management
    bool loadTheme(const QString &filePath);
    void registerTheme(const QString &name, const QHash<QString, ThemeProperties> &properties);
    void setActiveTheme(const QString &name);
    QString activeTheme() const;

    // Property lookup
    ThemeProperties properties(const QString &widgetType) const;

    // Per-widget overrides
    void setWidgetOverride(QTuiWidget *widget, const ThemeProperties &props);
    void clearWidgetOverride(QTuiWidget *widget);
    ThemeProperties effectiveProperties(const QTuiWidget *widget) const;

    // Named color aliases
    void setColorAlias(const QString &name, const Color &color);
    Color resolveColor(const QString &nameOrHex) const;

    // Border rendering helpers (code points; box-drawing chars are width 1)
    struct BorderChars {
        char32_t topLeft, topRight, bottomLeft, bottomRight;
        char32_t horizontal, vertical;
    };
    static BorderChars borderChars(const QString &style);

signals:
    void themeChanged(const QString &themeName);

private:
    Theme_Engine();
    static Theme_Engine *s_instance;

    QString m_activeTheme;
    QHash<QString, QHash<QString, ThemeProperties>> m_themes;
    QHash<QTuiWidget *, ThemeProperties> m_overrides;
    QHash<QString, Color> m_colorAliases;

    void registerDefaultTheme();
};

} // namespace QtTUI

#endif // QTTUI_THEME_ENGINE_H
