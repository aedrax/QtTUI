#include <QtTest/QtTest>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/QTuiWidget.h>
#include <QtTUI/ScreenBuffer.h>
#include <QTemporaryFile>
#include <QSignalSpy>

using namespace QtTUI;

// Concrete widget for testing theme integration
class ThemeTestWidget : public QTuiWidget {
    Q_OBJECT
public:
    explicit ThemeTestWidget(QTuiWidget *parent = nullptr)
        : QTuiWidget(parent) {}
    void paintEvent(ScreenBuffer &) override {}
};

class TestThemeEngine : public QObject {
    Q_OBJECT
private slots:

    // ---------------------------------------------------------------
    // Default theme tests (Req 1.1)
    // ---------------------------------------------------------------

    void defaultThemeExists()
    {
        auto *engine = Theme_Engine::instance();
        QCOMPARE(engine->activeTheme(), QStringLiteral("default"));
    }

    void defaultThemeProvidesPropertiesForWidgetTypes()
    {
        auto *engine = Theme_Engine::instance();
        const QStringList types = {
            "QTuiWidget", "QTuiPushButton", "QTuiCheckBox",
            "QTuiLabel", "QTuiLineEdit", "QTuiTableView",
            "QTuiListView", "QTuiMainWindow"
        };
        for (const auto &type : types) {
            ThemeProperties props = engine->properties(type);
            // Default theme should provide white-on-black with single border
            QCOMPARE(props.foreground, Color(255, 255, 255));
            QCOMPARE(props.background, Color(0, 0, 0));
            QCOMPARE(props.borderStyle, QStringLiteral("single"));
        }
    }

    void defaultThemeFallbackForUnknownType()
    {
        auto *engine = Theme_Engine::instance();
        ThemeProperties props = engine->properties("NonExistentWidget");
        // Should fall back to "default" entry
        QCOMPARE(props.foreground, Color(255, 255, 255));
        QCOMPARE(props.background, Color(0, 0, 0));
    }

    // ---------------------------------------------------------------
    // Custom theme registration and activation (Req 1.2, 1.3)
    // ---------------------------------------------------------------

    void registerAndActivateCustomTheme()
    {
        auto *engine = Theme_Engine::instance();

        ThemeProperties btnProps;
        btnProps.foreground = Color(0, 255, 0);
        btnProps.background = Color(50, 50, 50);
        btnProps.bold = true;

        QHash<QString, ThemeProperties> customTheme;
        customTheme.insert("QTuiPushButton", btnProps);

        engine->registerTheme("green", customTheme);
        engine->setActiveTheme("green");
        QCOMPARE(engine->activeTheme(), QStringLiteral("green"));

        ThemeProperties result = engine->properties("QTuiPushButton");
        QCOMPARE(result.foreground, Color(0, 255, 0));
        QCOMPARE(result.background, Color(50, 50, 50));
        QVERIFY(result.bold);

        // Restore default
        engine->setActiveTheme("default");
    }

    void themeChangedSignalEmitted()
    {
        auto *engine = Theme_Engine::instance();

        ThemeProperties p;
        QHash<QString, ThemeProperties> theme;
        theme.insert("default", p);
        engine->registerTheme("signal_test", theme);

        QSignalSpy spy(engine, &Theme_Engine::themeChanged);
        engine->setActiveTheme("signal_test");
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toString(), QStringLiteral("signal_test"));

        // Setting same theme again should not emit
        engine->setActiveTheme("signal_test");
        QCOMPARE(spy.count(), 1);

        engine->setActiveTheme("default");
    }

    void setActiveThemeUnknownDoesNothing()
    {
        auto *engine = Theme_Engine::instance();
        QString before = engine->activeTheme();
        engine->setActiveTheme("totally_nonexistent_theme");
        QCOMPARE(engine->activeTheme(), before);
    }

    // ---------------------------------------------------------------
    // Per-widget overrides (Req 1.4)
    // ---------------------------------------------------------------

    void widgetOverrideTakesPrecedence()
    {
        auto *engine = Theme_Engine::instance();
        engine->setActiveTheme("default");

        ThemeTestWidget widget;
        ThemeProperties override;
        override.foreground = Color(255, 0, 0);
        override.background = Color(0, 0, 255);
        override.bold = true;

        engine->setWidgetOverride(&widget, override);

        ThemeProperties effective = engine->effectiveProperties(&widget);
        QCOMPARE(effective.foreground, Color(255, 0, 0));
        QCOMPARE(effective.background, Color(0, 0, 255));
        QVERIFY(effective.bold);

        engine->clearWidgetOverride(&widget);

        ThemeProperties afterClear = engine->effectiveProperties(&widget);
        // Should fall back to theme defaults
        QCOMPARE(afterClear.foreground, Color(255, 255, 255));
        QCOMPARE(afterClear.background, Color(0, 0, 0));
    }

    void effectivePropertiesNullWidget()
    {
        auto *engine = Theme_Engine::instance();
        ThemeProperties props = engine->effectiveProperties(nullptr);
        // Should return default properties
        QCOMPARE(props.foreground, Color(255, 255, 255));
    }

    // ---------------------------------------------------------------
    // Color aliases (Req 1.5)
    // ---------------------------------------------------------------

    void colorAliasResolution()
    {
        auto *engine = Theme_Engine::instance();

        engine->setColorAlias("primary", Color(52, 152, 219));
        engine->setColorAlias("danger", Color(231, 76, 60));
        engine->setColorAlias("success", Color(46, 204, 113));

        Color primary = engine->resolveColor("primary");
        QCOMPARE(primary.r, uint8_t(52));
        QCOMPARE(primary.g, uint8_t(152));
        QCOMPARE(primary.b, uint8_t(219));

        Color danger = engine->resolveColor("danger");
        QCOMPARE(danger.r, uint8_t(231));
        QCOMPARE(danger.g, uint8_t(76));
        QCOMPARE(danger.b, uint8_t(60));
    }

    void resolveColorHex()
    {
        auto *engine = Theme_Engine::instance();
        Color c = engine->resolveColor("#ff8800");
        QCOMPARE(c.r, uint8_t(255));
        QCOMPARE(c.g, uint8_t(136));
        QCOMPARE(c.b, uint8_t(0));
    }

    void resolveColorInvalidReturnsDefault()
    {
        auto *engine = Theme_Engine::instance();
        Color c = engine->resolveColor("not_a_color");
        QCOMPARE(c, Color(0, 0, 0));
    }

    // ---------------------------------------------------------------
    // Border chars (Req 1.6)
    // ---------------------------------------------------------------

    void borderCharsSingle()
    {
        auto bc = Theme_Engine::borderChars("single");
        QCOMPARE(bc.topLeft, char32_t(0x250C));
        QCOMPARE(bc.topRight, char32_t(0x2510));
        QCOMPARE(bc.bottomLeft, char32_t(0x2514));
        QCOMPARE(bc.bottomRight, char32_t(0x2518));
        QCOMPARE(bc.horizontal, char32_t(0x2500));
        QCOMPARE(bc.vertical, char32_t(0x2502));
    }

    void borderCharsDouble()
    {
        auto bc = Theme_Engine::borderChars("double");
        QCOMPARE(bc.topLeft, char32_t(0x2554));
        QCOMPARE(bc.topRight, char32_t(0x2557));
        QCOMPARE(bc.bottomLeft, char32_t(0x255A));
        QCOMPARE(bc.bottomRight, char32_t(0x255D));
        QCOMPARE(bc.horizontal, char32_t(0x2550));
        QCOMPARE(bc.vertical, char32_t(0x2551));
    }

    void borderCharsRounded()
    {
        auto bc = Theme_Engine::borderChars("rounded");
        QCOMPARE(bc.topLeft, char32_t(0x256D));
        QCOMPARE(bc.topRight, char32_t(0x256E));
        QCOMPARE(bc.bottomLeft, char32_t(0x2570));
        QCOMPARE(bc.bottomRight, char32_t(0x256F));
        QCOMPARE(bc.horizontal, char32_t(0x2500));
        QCOMPARE(bc.vertical, char32_t(0x2502));
    }

    void borderCharsAscii()
    {
        auto bc = Theme_Engine::borderChars("ascii");
        QCOMPARE(bc.topLeft, U'+');
        QCOMPARE(bc.topRight, U'+');
        QCOMPARE(bc.bottomLeft, U'+');
        QCOMPARE(bc.bottomRight, U'+');
        QCOMPARE(bc.horizontal, U'-');
        QCOMPARE(bc.vertical, U'|');
    }

    void borderCharsNone()
    {
        auto bc = Theme_Engine::borderChars("none");
        QCOMPARE(bc.topLeft, U' ');
        QCOMPARE(bc.horizontal, U' ');
        QCOMPARE(bc.vertical, U' ');
    }

    // ---------------------------------------------------------------
    // loadTheme JSON (Req 1.8, 1.9)
    // ---------------------------------------------------------------

    void loadThemeValidJson()
    {
        auto *engine = Theme_Engine::instance();

        QByteArray json = R"({
            "name": "test_dark",
            "colorAliases": {
                "primary": "#3498db"
            },
            "widgets": {
                "QTuiPushButton": {
                    "foreground": "#ffffff",
                    "background": "#333333",
                    "focusForeground": "#ffffff",
                    "focusBackground": "primary",
                    "borderStyle": "double",
                    "bold": true
                }
            }
        })";

        QTemporaryFile tmpFile;
        QVERIFY(tmpFile.open());
        tmpFile.write(json);
        tmpFile.flush();

        bool ok = engine->loadTheme(tmpFile.fileName());
        QVERIFY(ok);

        engine->setActiveTheme("test_dark");
        ThemeProperties props = engine->properties("QTuiPushButton");
        QCOMPARE(props.foreground, Color(255, 255, 255));
        QCOMPARE(props.background, Color(51, 51, 51));
        QCOMPARE(props.focusBackground, Color(52, 152, 219)); // resolved from "primary" alias
        QCOMPARE(props.borderStyle, QStringLiteral("double"));
        QVERIFY(props.bold);

        engine->setActiveTheme("default");
    }

    void loadThemeInvalidJson()
    {
        auto *engine = Theme_Engine::instance();

        QByteArray badJson = "{ this is not valid json }}}";

        QTemporaryFile tmpFile;
        QVERIFY(tmpFile.open());
        tmpFile.write(badJson);
        tmpFile.flush();

        bool ok = engine->loadTheme(tmpFile.fileName());
        QVERIFY(!ok);
    }

    void loadThemeMissingProperties()
    {
        auto *engine = Theme_Engine::instance();

        // Only specifies foreground, everything else should use defaults
        QByteArray json = R"({
            "name": "partial_theme",
            "widgets": {
                "QTuiLabel": {
                    "foreground": "#00ff00"
                }
            }
        })";

        QTemporaryFile tmpFile;
        QVERIFY(tmpFile.open());
        tmpFile.write(json);
        tmpFile.flush();

        bool ok = engine->loadTheme(tmpFile.fileName());
        QVERIFY(ok);

        engine->setActiveTheme("partial_theme");
        ThemeProperties props = engine->properties("QTuiLabel");
        QCOMPARE(props.foreground, Color(0, 255, 0));
        // Unspecified properties should have defaults
        QCOMPARE(props.background, Color(0, 0, 0));
        QCOMPARE(props.borderStyle, QStringLiteral("single"));
        QVERIFY(!props.bold);

        engine->setActiveTheme("default");
    }

    void loadThemeMissingFile()
    {
        auto *engine = Theme_Engine::instance();
        bool ok = engine->loadTheme("/nonexistent/path/theme.json");
        QVERIFY(!ok);
    }

    void loadThemeMissingName()
    {
        auto *engine = Theme_Engine::instance();

        QByteArray json = R"({ "widgets": {} })";

        QTemporaryFile tmpFile;
        QVERIFY(tmpFile.open());
        tmpFile.write(json);
        tmpFile.flush();

        bool ok = engine->loadTheme(tmpFile.fileName());
        QVERIFY(!ok);
    }

    // ---------------------------------------------------------------
    // Widget theme integration (Req 1.7)
    // ---------------------------------------------------------------

    void widgetThemePropertiesHelper()
    {
        auto *engine = Theme_Engine::instance();
        engine->setActiveTheme("default");

        ThemeTestWidget widget;
        ThemeProperties props = widget.themeProperties();
        QCOMPARE(props.foreground, Color(255, 255, 255));
        QCOMPARE(props.background, Color(0, 0, 0));
    }
};

QTEST_MAIN(TestThemeEngine)
#include "test_theme_engine.moc"
