// Feature: widget-rendering-overhaul, Property 22: Widget Background Clearing
// Feature: widget-rendering-overhaul, Property 23: Geometry Shrink Clearing
// Feature: widget-rendering-overhaul, Property 24: Hidden Widget Clearing
#include <QtTest/QtTest>
#include <rapidcheck.h>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiMainWindow.h>
#include <QtTUI/QTuiWidget.h>
#include <QtTUI/RenderScheduler.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/Theme_Engine.h>

using namespace QtTUI;

// ---------------------------------------------------------------------------
// A test widget that calls clearBackground() at the start of paintEvent()
// and then writes a known character to verify content is painted after clearing.
// ---------------------------------------------------------------------------
class ClearingWidget : public QTuiWidget {
    Q_OBJECT
public:
    explicit ClearingWidget(QTuiWidget *parent = nullptr)
        : QTuiWidget(parent)
    {
    }

    QChar contentChar = QLatin1Char('W');
    int paintCallCount = 0;

    void paintEvent(ScreenBuffer &buffer) override
    {
        ++paintCallCount;
        const QRect geo = geometry();

        // Call clearBackground with the widget's own geometry as clip rect
        // (simulating what a real widget would do)
        clearBackground(buffer, geo);

        // Then write content character to the first cell only
        if (!geo.isEmpty() && geo.x() < buffer.width() && geo.y() < buffer.height()) {
            Cell &cell = buffer.backCell(geo.x(), geo.y());
            cell.character = contentChar;
        }
    }
};

// ---------------------------------------------------------------------------
// A parent widget that paints its background with a known character
// ---------------------------------------------------------------------------
class ParentWidget : public QTuiWidget {
    Q_OBJECT
public:
    explicit ParentWidget(QTuiWidget *parent = nullptr)
        : QTuiWidget(parent)
    {
    }

    void paintEvent(ScreenBuffer &buffer) override
    {
        const QRect geo = geometry();
        const auto theme = themeProperties();

        // Fill with parent background
        for (int row = geo.y(); row < geo.y() + geo.height(); ++row) {
            for (int col = geo.x(); col < geo.x() + geo.width(); ++col) {
                if (col >= 0 && col < buffer.width() && row >= 0 && row < buffer.height()) {
                    Cell &cell = buffer.backCell(col, row);
                    cell.character = QLatin1Char(' ');
                    cell.foreground = theme.foreground;
                    cell.background = theme.background;
                }
            }
        }
    }
};

class TestWidgetClearing : public QObject {
    Q_OBJECT

private slots:

    // -----------------------------------------------------------------------
    // Feature: widget-rendering-overhaul, Property 22: Widget Background Clearing
    // -----------------------------------------------------------------------
    void widgetBackgroundClearing()
    {
        rc::check("At the start of paintEvent(), every cell within the widget's geometry "
                  "is set to the theme background color with a space character",
            []() {
                // **Validates: Requirements 10.1**
                static int argc = 1;
                static char arg0[] = "test";
                static char *argv[] = { arg0, nullptr };
                QTuiApplication app(argc, argv);

                QTuiMainWindow mainWindow;
                mainWindow.setGeometry(QRect(0, 0, 80, 24));
                mainWindow.setVisible(true);
                app.setMainWindow(&mainWindow);

                // Create a widget with random geometry
                auto *widget = new ClearingWidget(&mainWindow);
                const int x = *rc::gen::inRange(0, 60);
                const int y = *rc::gen::inRange(0, 20);
                const int w = *rc::gen::inRange(2, 20);
                const int h = *rc::gen::inRange(1, 4);
                widget->setGeometry(QRect(x, y, w, h));
                widget->setVisible(true);

                // First, fill the buffer area with garbage characters
                ScreenBuffer *buf = app.screenBuffer();
                for (int row = y; row < y + h && row < buf->height(); ++row) {
                    for (int col = x; col < x + w && col < buf->width(); ++col) {
                        Cell &cell = buf->backCell(col, row);
                        cell.character = QLatin1Char('Z');
                        cell.foreground = Color(255, 0, 0);
                        cell.background = Color(0, 255, 0);
                    }
                }

                // Trigger a render pass
                widget->markDirty();
                app.triggerRender();

                // After clearBackground() + paintEvent(), all cells except the first
                // should have the theme background color and space character
                const auto theme = widget->themeProperties();
                const QRect geo = widget->geometry();

                for (int row = geo.y(); row < geo.y() + geo.height() && row < buf->height(); ++row) {
                    for (int col = geo.x(); col < geo.x() + geo.width() && col < buf->width(); ++col) {
                        const Cell &cell = buf->backCell(col, row);
                        if (col == geo.x() && row == geo.y()) {
                            // First cell has the content character
                            RC_ASSERT(cell.character == widget->contentChar);
                        } else {
                            // All other cells should be cleared to theme background
                            RC_ASSERT(cell.character == QLatin1Char(' '));
                            RC_ASSERT(cell.background == theme.background);
                            RC_ASSERT(cell.foreground == theme.foreground);
                        }
                    }
                }
            });
    }

    // -----------------------------------------------------------------------
    // Feature: widget-rendering-overhaul, Property 23: Geometry Shrink Clearing
    // -----------------------------------------------------------------------
    void geometryShrinkClearing()
    {
        rc::check("When a widget's geometry shrinks, the old geometry area not covered "
                  "by the new geometry is cleared with the parent's background color",
            []() {
                // **Validates: Requirements 10.2**
                static int argc = 1;
                static char arg0[] = "test";
                static char *argv[] = { arg0, nullptr };
                QTuiApplication app(argc, argv);

                QTuiMainWindow mainWindow;
                mainWindow.setGeometry(QRect(0, 0, 80, 24));
                mainWindow.setVisible(true);
                app.setMainWindow(&mainWindow);

                // Create a parent widget
                auto *parent = new ParentWidget(&mainWindow);
                parent->setGeometry(QRect(0, 0, 80, 24));
                parent->setVisible(true);

                // Create a child widget with an initial larger geometry
                auto *widget = new ClearingWidget(parent);
                const int x = *rc::gen::inRange(0, 40);
                const int y = *rc::gen::inRange(0, 12);
                const int oldW = *rc::gen::inRange(10, 30);
                const int oldH = *rc::gen::inRange(4, 10);
                widget->setGeometry(QRect(x, y, oldW, oldH));
                widget->setVisible(true);

                // Do an initial render to establish the widget's content
                app.triggerRender();

                // Now shrink the widget's geometry
                const int newW = *rc::gen::inRange(2, oldW);
                const int newH = *rc::gen::inRange(1, oldH);
                RC_PRE(newW < oldW || newH < oldH); // Ensure it actually shrinks

                widget->setGeometry(QRect(x, y, newW, newH));

                // Trigger a render pass
                app.triggerRender();

                // Verify: cells in the old geometry that are NOT in the new geometry
                // should be cleared with the parent's background
                ScreenBuffer *buf = app.screenBuffer();
                const auto parentTheme = parent->themeProperties();
                const QRect oldGeo(x, y, oldW, oldH);
                const QRect newGeo(x, y, newW, newH);

                for (int row = oldGeo.y(); row < oldGeo.y() + oldGeo.height() && row < buf->height(); ++row) {
                    for (int col = oldGeo.x(); col < oldGeo.x() + oldGeo.width() && col < buf->width(); ++col) {
                        if (!newGeo.contains(col, row)) {
                            const Cell &cell = buf->backCell(col, row);
                            RC_ASSERT(cell.character == QLatin1Char(' '));
                            RC_ASSERT(cell.background == parentTheme.background);
                        }
                    }
                }
            });
    }

    // -----------------------------------------------------------------------
    // Feature: widget-rendering-overhaul, Property 24: Hidden Widget Clearing
    // -----------------------------------------------------------------------
    void hiddenWidgetClearing()
    {
        rc::check("When a widget transitions from visible to hidden, its geometry "
                  "is cleared with the parent widget's background color",
            []() {
                // **Validates: Requirements 10.3**
                static int argc = 1;
                static char arg0[] = "test";
                static char *argv[] = { arg0, nullptr };
                QTuiApplication app(argc, argv);

                QTuiMainWindow mainWindow;
                mainWindow.setGeometry(QRect(0, 0, 80, 24));
                mainWindow.setVisible(true);
                app.setMainWindow(&mainWindow);

                // Create a parent widget
                auto *parent = new ParentWidget(&mainWindow);
                parent->setGeometry(QRect(0, 0, 80, 24));
                parent->setVisible(true);

                // Create a child widget
                auto *widget = new ClearingWidget(parent);
                const int x = *rc::gen::inRange(0, 60);
                const int y = *rc::gen::inRange(0, 20);
                const int w = *rc::gen::inRange(2, 20);
                const int h = *rc::gen::inRange(1, 4);
                widget->setGeometry(QRect(x, y, w, h));
                widget->setVisible(true);

                // Do an initial render to paint the widget
                app.triggerRender();

                // Verify the widget was painted (first cell has content char)
                ScreenBuffer *buf = app.screenBuffer();
                RC_PRE(x < buf->width() && y < buf->height());
                RC_ASSERT(buf->backCell(x, y).character == widget->contentChar);

                // Now hide the widget
                widget->setVisible(false);

                // Trigger a render pass
                app.triggerRender();

                // Verify: all cells in the widget's geometry should be cleared
                // with the parent's background color
                const auto parentTheme = parent->themeProperties();
                const QRect geo(x, y, w, h);

                for (int row = geo.y(); row < geo.y() + geo.height() && row < buf->height(); ++row) {
                    for (int col = geo.x(); col < geo.x() + geo.width() && col < buf->width(); ++col) {
                        const Cell &cell = buf->backCell(col, row);
                        RC_ASSERT(cell.character == QLatin1Char(' '));
                        RC_ASSERT(cell.background == parentTheme.background);
                    }
                }
            });
    }

    // -----------------------------------------------------------------------
    // Unit test: clearBackground fills with theme background
    // -----------------------------------------------------------------------
    void clearBackgroundFillsThemeBackground()
    {
        static int argc = 1;
        static char arg0[] = "test";
        static char *argv[] = { arg0, nullptr };
        QTuiApplication app(argc, argv);

        QTuiMainWindow mainWindow;
        mainWindow.setGeometry(QRect(0, 0, 80, 24));
        mainWindow.setVisible(true);
        app.setMainWindow(&mainWindow);

        auto *widget = new ClearingWidget(&mainWindow);
        widget->setGeometry(QRect(5, 3, 10, 4));
        widget->setVisible(true);

        // Fill buffer with garbage
        ScreenBuffer *buf = app.screenBuffer();
        for (int row = 3; row < 7; ++row) {
            for (int col = 5; col < 15; ++col) {
                Cell &cell = buf->backCell(col, row);
                cell.character = QLatin1Char('G');
                cell.foreground = Color(100, 100, 100);
                cell.background = Color(200, 200, 200);
            }
        }

        // Trigger render
        widget->markDirty();
        app.triggerRender();

        // Verify all cells (except first) are cleared
        const auto theme = widget->themeProperties();
        for (int row = 3; row < 7; ++row) {
            for (int col = 5; col < 15; ++col) {
                const Cell &cell = buf->backCell(col, row);
                if (col == 5 && row == 3) {
                    QCOMPARE(cell.character, widget->contentChar);
                } else {
                    QCOMPARE(cell.character, QLatin1Char(' '));
                    QVERIFY(cell.background == theme.background);
                    QVERIFY(cell.foreground == theme.foreground);
                }
            }
        }
    }
};

QTEST_APPLESS_MAIN(TestWidgetClearing)
#include "test_widget_clearing.moc"
