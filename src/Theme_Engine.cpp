#include <QtTUI/Theme_Engine.h>
#include <QtTUI/QTuiWidget.h>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDebug>

namespace QtTUI {

Theme_Engine *Theme_Engine::s_instance = nullptr;

Theme_Engine::Theme_Engine()
    : QObject(nullptr)
{
    registerDefaultTheme();
    m_activeTheme = QStringLiteral("default");
}

Theme_Engine *Theme_Engine::instance()
{
    if (!s_instance)
        s_instance = new Theme_Engine();
    return s_instance;
}

void Theme_Engine::registerDefaultTheme()
{
    QHash<QString, ThemeProperties> defaultProps;

    // Default properties apply to all widget types via fallback
    ThemeProperties base;
    defaultProps.insert(QStringLiteral("default"), base);

    // Register specific widget type defaults
    const QStringList widgetTypes = {
        QStringLiteral("QTuiWidget"),
        QStringLiteral("QTuiPushButton"),
        QStringLiteral("QTuiCheckBox"),
        QStringLiteral("QTuiRadioButton"),
        QStringLiteral("QTuiLabel"),
        QStringLiteral("QTuiComboBox"),
        QStringLiteral("QTuiSpinBox"),
        QStringLiteral("QTuiDoubleSpinBox"),
        QStringLiteral("QTuiSlider"),
        QStringLiteral("QTuiDial"),
        QStringLiteral("QTuiProgressBar"),
        QStringLiteral("QTuiLineEdit"),
        QStringLiteral("QTuiTextEdit"),
        QStringLiteral("QTuiTableView"),
        QStringLiteral("QTuiListView"),
        QStringLiteral("QTuiTreeView"),
        QStringLiteral("QTuiMainWindow"),
        QStringLiteral("QTuiGroupBox"),
        QStringLiteral("QTuiTabWidget"),
        QStringLiteral("QTuiScrollArea"),
    };

    for (const auto &type : widgetTypes)
        defaultProps.insert(type, base);

    m_themes.insert(QStringLiteral("default"), defaultProps);
}

void Theme_Engine::registerTheme(const QString &name, const QHash<QString, ThemeProperties> &properties)
{
    m_themes.insert(name, properties);
}

void Theme_Engine::setActiveTheme(const QString &name)
{
    if (!m_themes.contains(name)) {
        qWarning() << "Theme_Engine::setActiveTheme: unknown theme" << name;
        return;
    }
    if (m_activeTheme == name)
        return;
    m_activeTheme = name;
    emit themeChanged(name);
}

QString Theme_Engine::activeTheme() const
{
    return m_activeTheme;
}

ThemeProperties Theme_Engine::properties(const QString &widgetType) const
{
    auto themeIt = m_themes.find(m_activeTheme);
    if (themeIt == m_themes.end())
        themeIt = m_themes.find(QStringLiteral("default"));

    if (themeIt == m_themes.end())
        return ThemeProperties();

    const auto &themeMap = themeIt.value();

    // Look up specific widget type first
    auto propIt = themeMap.find(widgetType);
    if (propIt != themeMap.end())
        return propIt.value();

    // Fall back to "default" entry within the theme
    propIt = themeMap.find(QStringLiteral("default"));
    if (propIt != themeMap.end())
        return propIt.value();

    return ThemeProperties();
}

void Theme_Engine::setWidgetOverride(QTuiWidget *widget, const ThemeProperties &props)
{
    if (!widget)
        return;
    m_overrides.insert(widget, props);
}

void Theme_Engine::clearWidgetOverride(QTuiWidget *widget)
{
    m_overrides.remove(widget);
}

ThemeProperties Theme_Engine::effectiveProperties(const QTuiWidget *widget) const
{
    if (!widget)
        return properties(QStringLiteral("default"));

    // Per-widget override takes precedence
    auto overrideIt = m_overrides.find(const_cast<QTuiWidget *>(widget));
    if (overrideIt != m_overrides.end())
        return overrideIt.value();

    // Derive widget type name from the class name
    QString widgetType = QString::fromLatin1(widget->metaObject()->className());
    // Strip namespace prefix if present (e.g., "QtTUI::QTuiPushButton" -> "QTuiPushButton")
    int colonIdx = widgetType.lastIndexOf(QLatin1String("::"));
    if (colonIdx >= 0)
        widgetType = widgetType.mid(colonIdx + 2);

    return properties(widgetType);
}

void Theme_Engine::setColorAlias(const QString &name, const Color &color)
{
    m_colorAliases.insert(name, color);
}

Color Theme_Engine::resolveColor(const QString &nameOrHex) const
{
    // Check alias first
    auto it = m_colorAliases.find(nameOrHex);
    if (it != m_colorAliases.end())
        return it.value();

    // Try parsing as hex color (#RRGGBB)
    if (nameOrHex.startsWith(QLatin1Char('#')) && nameOrHex.length() == 7) {
        bool ok = false;
        int r = nameOrHex.mid(1, 2).toInt(&ok, 16);
        if (!ok) return Color();
        int g = nameOrHex.mid(3, 2).toInt(&ok, 16);
        if (!ok) return Color();
        int b = nameOrHex.mid(5, 2).toInt(&ok, 16);
        if (!ok) return Color();
        return Color(static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b));
    }

    qWarning() << "Theme_Engine::resolveColor: unresolved color" << nameOrHex;
    return Color();
}

Theme_Engine::BorderChars Theme_Engine::borderChars(const QString &style)
{
    if (style == QLatin1String("single")) {
        return { QChar(0x250C), QChar(0x2510), QChar(0x2514), QChar(0x2518),
                 QChar(0x2500), QChar(0x2502) };
    }
    if (style == QLatin1String("double")) {
        return { QChar(0x2554), QChar(0x2557), QChar(0x255A), QChar(0x255D),
                 QChar(0x2550), QChar(0x2551) };
    }
    if (style == QLatin1String("rounded")) {
        return { QChar(0x256D), QChar(0x256E), QChar(0x2570), QChar(0x256F),
                 QChar(0x2500), QChar(0x2502) };
    }
    if (style == QLatin1String("ascii")) {
        return { QLatin1Char('+'), QLatin1Char('+'), QLatin1Char('+'), QLatin1Char('+'),
                 QLatin1Char('-'), QLatin1Char('|') };
    }
    // "none" or unknown
    return { QLatin1Char(' '), QLatin1Char(' '), QLatin1Char(' '), QLatin1Char(' '),
             QLatin1Char(' '), QLatin1Char(' ') };
}

static Color parseColorString(const QString &str, const QHash<QString, Color> &aliases)
{
    // Check alias
    auto it = aliases.find(str);
    if (it != aliases.end())
        return it.value();

    // Parse hex
    if (str.startsWith(QLatin1Char('#')) && str.length() == 7) {
        bool ok = false;
        int r = str.mid(1, 2).toInt(&ok, 16);
        if (!ok) return Color();
        int g = str.mid(3, 2).toInt(&ok, 16);
        if (!ok) return Color();
        int b = str.mid(5, 2).toInt(&ok, 16);
        if (!ok) return Color();
        return Color(static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b));
    }

    return Color();
}

bool Theme_Engine::loadTheme(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Theme_Engine::loadTheme: cannot open file" << filePath;
        return false;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "Theme_Engine::loadTheme: JSON parse error:" << parseError.errorString();
        return false;
    }

    if (!doc.isObject()) {
        qWarning() << "Theme_Engine::loadTheme: root is not a JSON object";
        return false;
    }

    QJsonObject root = doc.object();
    QString themeName = root.value(QStringLiteral("name")).toString();
    if (themeName.isEmpty()) {
        qWarning() << "Theme_Engine::loadTheme: missing 'name' field";
        return false;
    }

    // Parse color aliases
    QHash<QString, Color> loadedAliases;
    QJsonObject aliasesObj = root.value(QStringLiteral("colorAliases")).toObject();
    for (auto it = aliasesObj.begin(); it != aliasesObj.end(); ++it) {
        QString hexStr = it.value().toString();
        if (hexStr.startsWith(QLatin1Char('#')) && hexStr.length() == 7) {
            bool ok = false;
            int r = hexStr.mid(1, 2).toInt(&ok, 16);
            if (!ok) continue;
            int g = hexStr.mid(3, 2).toInt(&ok, 16);
            if (!ok) continue;
            int b = hexStr.mid(5, 2).toInt(&ok, 16);
            if (!ok) continue;
            Color c(static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b));
            loadedAliases.insert(it.key(), c);
            m_colorAliases.insert(it.key(), c);
        } else {
            qWarning() << "Theme_Engine::loadTheme: invalid color alias value" << hexStr
                        << "for key" << it.key();
        }
    }

    // Parse widgets section
    QHash<QString, ThemeProperties> themeProps;
    QJsonObject widgetsObj = root.value(QStringLiteral("widgets")).toObject();
    ThemeProperties defaults;

    for (auto it = widgetsObj.begin(); it != widgetsObj.end(); ++it) {
        QString widgetType = it.key();
        QJsonObject propObj = it.value().toObject();
        ThemeProperties props = defaults;

        if (propObj.contains(QStringLiteral("foreground")))
            props.foreground = parseColorString(propObj.value(QStringLiteral("foreground")).toString(), loadedAliases);
        if (propObj.contains(QStringLiteral("background")))
            props.background = parseColorString(propObj.value(QStringLiteral("background")).toString(), loadedAliases);
        if (propObj.contains(QStringLiteral("focusForeground")))
            props.focusForeground = parseColorString(propObj.value(QStringLiteral("focusForeground")).toString(), loadedAliases);
        if (propObj.contains(QStringLiteral("focusBackground")))
            props.focusBackground = parseColorString(propObj.value(QStringLiteral("focusBackground")).toString(), loadedAliases);
        if (propObj.contains(QStringLiteral("disabledForeground")))
            props.disabledForeground = parseColorString(propObj.value(QStringLiteral("disabledForeground")).toString(), loadedAliases);
        if (propObj.contains(QStringLiteral("disabledBackground")))
            props.disabledBackground = parseColorString(propObj.value(QStringLiteral("disabledBackground")).toString(), loadedAliases);
        if (propObj.contains(QStringLiteral("borderStyle")))
            props.borderStyle = propObj.value(QStringLiteral("borderStyle")).toString();
        if (propObj.contains(QStringLiteral("bold")))
            props.bold = propObj.value(QStringLiteral("bold")).toBool();
        if (propObj.contains(QStringLiteral("italic")))
            props.italic = propObj.value(QStringLiteral("italic")).toBool();
        if (propObj.contains(QStringLiteral("underline")))
            props.underline = propObj.value(QStringLiteral("underline")).toBool();

        themeProps.insert(widgetType, props);
    }

    m_themes.insert(themeName, themeProps);
    return true;
}

} // namespace QtTUI
