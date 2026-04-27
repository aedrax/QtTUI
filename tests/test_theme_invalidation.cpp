// Feature: widget-rendering-overhaul, Property 33: Theme Change Invalidation
// Feature: widget-rendering-overhaul, Property 34: Widget Theme Override
#include <QtTest/QtTest>
#include <rapidcheck.h>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiMainWindow.h>
#include <QtTUI/QTuiLabel.h>
#include <QtTUI/RenderScheduler.h>
#include <QtTUI/Theme_Engine.h>

using namespace QtTUI;

class TestThemeInvalidation : public QObject {
    Q_OBJECT

private slots:

    // -----------------------------------------------------------------------
    // Feature: widget-rendering-overhaul, Property 33: Theme Change Invalidation
    // -----------------------------------------------------------------------
    void themeChangeInvalidation()
    {
        rc::check("Theme change marks all visible widgets dirty",
            []() {
                // **Validates: Requirements 14.2**
                static int argc = 1;
                static char arg0[] = "test";
                static char *argv[] = { arg0, nullptr };
                QTuiApplication app(argc, argv);

                QTuiMainWindow mainWindow;
                mainWindow.setGeometry(QRect(0, 0, 80, 24));
                mainWindow.setVisible(true);
                app.setMainWindow(&mainWindow);

                // Generate a random number of visible child widgets
                const int numWidgets = *rc::gen::inRange(1, 8);

                QList<QTuiLabel *> labels;
                for (int i = 0; i < numWidgets; ++i) {
                    auto *label = new QTuiLabel(
                        QStringLiteral("Widget %1").arg(i), &mainWindow);
                    int x = *rc::gen::inRange(0, 60);
                    int y = *rc::gen::inRange(0, 20);
                    int w = *rc::gen::inRange(3, 20);
                    int h = *rc::gen::inRange(1, 4);
                    label->setGeometry(QRect(x, y, w, h));
                    label->setVisible(true);
                    labels.append(label);
                }

                // Also add a hidden widget that should NOT be marked dirty
                auto *hiddenLabel = new QTuiLabel(QStringLiteral("Hidden"), &mainWindow);
                hiddenLabel->setGeometry(QRect(0, 0, 10, 1));
                hiddenLabel->setVisible(false);

                // Drain initial events and clear dirty state
                QCoreApplication::processEvents();
                QCoreApplication::processEvents();

                mainWindow.clearDirtyRegions();
                for (auto *label : labels)
                    label->clearDirtyRegions();
                hiddenLabel->clearDirtyRegions();

                // Verify all are clean
                RC_ASSERT(!mainWindow.isDirty());
                for (auto *label : labels)
                    RC_ASSERT(!label->isDirty());
                RC_ASSERT(!hiddenLabel->isDirty());

                // Register a new theme and switch to it
                QHash<QString, ThemeProperties> newThemeProps;
                ThemeProperties newBase;
                newBase.foreground = Color(200, 200, 200);
                newBase.background = Color(30, 30, 30);
                newThemeProps.insert(QStringLiteral("default"), newBase);
                Theme_Engine::instance()->registerTheme(
                    QStringLiteral("test_theme"), newThemeProps);

                // Change the active theme — this should trigger themeChanged signal
                Theme_Engine::instance()->setActiveTheme(QStringLiteral("test_theme"));

                // Verify: all visible widgets should be marked dirty
                RC_ASSERT(mainWindow.isDirty());
                for (auto *label : labels)
                    RC_ASSERT(label->isDirty());

                // Hidden widget should NOT be marked dirty
                RC_ASSERT(!hiddenLabel->isDirty());

                // Scheduler should have a pending render
                RC_ASSERT(app.scheduler()->hasPendingRender());

                // Restore default theme for other tests
                Theme_Engine::instance()->setActiveTheme(QStringLiteral("default"));
            });
    }

    // -----------------------------------------------------------------------
    // Feature: widget-rendering-overhaul, Property 34: Widget Theme Override
    // -----------------------------------------------------------------------
    void widgetThemeOverride()
    {
        rc::check("Widget override returns override properties via effectiveProperties()",
            []() {
                // **Validates: Requirements 14.3**
                static int argc = 1;
                static char arg0[] = "test";
                static char *argv[] = { arg0, nullptr };
                QTuiApplication app(argc, argv);

                QTuiMainWindow mainWindow;
                mainWindow.setGeometry(QRect(0, 0, 80, 24));
                mainWindow.setVisible(true);
                app.setMainWindow(&mainWindow);

                QTuiLabel label(QStringLiteral("Override Test"), &mainWindow);
                label.setGeometry(QRect(0, 0, 20, 1));

                // Generate random override colors
                const uint8_t r1 = *rc::gen::inRange(0, 256);
                const uint8_t g1 = *rc::gen::inRange(0, 256);
                const uint8_t b1 = *rc::gen::inRange(0, 256);
                const uint8_t r2 = *rc::gen::inRange(0, 256);
                const uint8_t g2 = *rc::gen::inRange(0, 256);
                const uint8_t b2 = *rc::gen::inRange(0, 256);

                Color overrideFg(static_cast<uint8_t>(r1),
                                 static_cast<uint8_t>(g1),
                                 static_cast<uint8_t>(b1));
                Color overrideBg(static_cast<uint8_t>(r2),
                                 static_cast<uint8_t>(g2),
                                 static_cast<uint8_t>(b2));

                ThemeProperties overrideProps;
                overrideProps.foreground = overrideFg;
                overrideProps.background = overrideBg;
                overrideProps.bold = true;

                // Set the per-widget override
                Theme_Engine::instance()->setWidgetOverride(&label, overrideProps);

                // Verify effectiveProperties returns the override
                ThemeProperties effective =
                    Theme_Engine::instance()->effectiveProperties(&label);
                RC_ASSERT(effective.foreground == overrideFg);
                RC_ASSERT(effective.background == overrideBg);
                RC_ASSERT(effective.bold == true);

                // Verify that themeProperties() on the widget also returns override
                ThemeProperties widgetTheme = label.themeProperties();
                RC_ASSERT(widgetTheme.foreground == overrideFg);
                RC_ASSERT(widgetTheme.background == overrideBg);
                RC_ASSERT(widgetTheme.bold == true);

                // Verify another widget without override gets default theme
                QTuiLabel otherLabel(QStringLiteral("No Override"), &mainWindow);
                otherLabel.setGeometry(QRect(0, 1, 20, 1));

                ThemeProperties otherEffective =
                    Theme_Engine::instance()->effectiveProperties(&otherLabel);
                // Should NOT have the override colors
                // (unless by extreme coincidence the random colors match defaults)
                ThemeProperties defaultProps =
                    Theme_Engine::instance()->properties(QStringLiteral("QTuiLabel"));
                RC_ASSERT(otherEffective.foreground == defaultProps.foreground);
                RC_ASSERT(otherEffective.background == defaultProps.background);

                // Clean up override
                Theme_Engine::instance()->clearWidgetOverride(&label);

                // After clearing, effectiveProperties should return theme defaults
                ThemeProperties afterClear =
                    Theme_Engine::instance()->effectiveProperties(&label);
                RC_ASSERT(afterClear.foreground == defaultProps.foreground);
                RC_ASSERT(afterClear.background == defaultProps.background);
            });
    }
};

QTEST_APPLESS_MAIN(TestThemeInvalidation)
#include "test_theme_invalidation.moc"
