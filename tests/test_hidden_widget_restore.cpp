// Regression tests: hiding a widget must restore the content underneath it
// on the next (partial) render pass — without requiring a full repaint.
// Bug: RenderScheduler previously repainted only widgets whose own isDirty()
// flag was set, so cells vacated by a hidden overlay kept stale content, and
// the vacated-region clear ran after siblings painted, wiping fresh output.
#include <QtTest/QtTest>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiMainWindow.h>
#include <QtTUI/QTuiWidget.h>
#include <QtTUI/RenderScheduler.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/Theme_Engine.h>

using namespace QtTUI;

// Fills its entire geometry with a fixed character.
class FillWidget : public QTuiWidget {
    Q_OBJECT
public:
    explicit FillWidget(QChar ch, QTuiWidget *parent = nullptr)
        : QTuiWidget(parent)
        , m_ch(ch)
    {
    }

    int paintCallCount = 0;

    void paintEvent(ScreenBuffer &buffer) override
    {
        ++paintCallCount;
        clearBackground(buffer, geometry());
        const QRect geo = geometry();
        for (int row = geo.y(); row < geo.y() + geo.height() && row < buffer.height(); ++row) {
            for (int col = geo.x(); col < geo.x() + geo.width() && col < buffer.width(); ++col) {
                if (col >= 0 && row >= 0)
                    buffer.backCell(col, row).setChar(m_ch.unicode());
            }
        }
    }

private:
    QChar m_ch;
};

class TestHiddenWidgetRestore : public QObject {
    Q_OBJECT

private slots:
    // Overlay over sibling content: hiding the overlay must repaint the
    // sibling's cells underneath, via a partial pass only.
    void hiddenOverlayRestoresSiblingContent()
    {
        static int argc = 1;
        static char arg0[] = "test";
        static char *argv[] = { arg0, nullptr };
        QTuiApplication app(argc, argv);

        QTuiMainWindow mainWindow;
        mainWindow.setGeometry(QRect(0, 0, 80, 24));
        mainWindow.setVisible(true);
        app.setMainWindow(&mainWindow);

        auto *content = new FillWidget(QLatin1Char('T'), &mainWindow);
        content->setGeometry(QRect(0, 0, 40, 10));

        // Later sibling = painted after = visually on top of content
        auto *overlay = new FillWidget(QLatin1Char('O'), &mainWindow);
        overlay->setGeometry(QRect(5, 2, 20, 3));

        // Unrelated widget far away — must not repaint during the hide pass
        auto *far = new FillWidget(QLatin1Char('F'), &mainWindow);
        far->setGeometry(QRect(60, 20, 10, 3));

        app.triggerRender();
        ScreenBuffer *buf = app.screenBuffer();
        QCOMPARE(buf->backCell(6, 3).ch, U'O');
        QCOMPARE(buf->backCell(61, 21).ch, U'F');

        const int farPaints = far->paintCallCount;

        // Hide the overlay; the scheduled partial pass must restore 'T'
        overlay->setVisible(false);
        QTest::qWait(20);

        for (int row = 2; row < 5; ++row) {
            for (int col = 5; col < 25; ++col) {
                QVERIFY2(buf->backCell(col, row).ch == U'T',
                         qPrintable(QStringLiteral("stale cell at %1,%2: '%3'")
                                        .arg(col)
                                        .arg(row)
                                        .arg(buf->backCell(col, row).text())));
            }
        }

        // Partial pass: the far widget must not have been repainted
        QCOMPARE(far->paintCallCount, farPaints);
    }

    // Overlay over bare main-window background (nothing paints underneath):
    // hiding it must clear the vacated cells to the parent background.
    void hiddenOverlayOverBareBackgroundClears()
    {
        static int argc = 1;
        static char arg0[] = "test";
        static char *argv[] = { arg0, nullptr };
        QTuiApplication app(argc, argv);

        QTuiMainWindow mainWindow;
        mainWindow.setGeometry(QRect(0, 0, 80, 24));
        mainWindow.setVisible(true);
        app.setMainWindow(&mainWindow);

        auto *overlay = new FillWidget(QLatin1Char('O'), &mainWindow);
        overlay->setGeometry(QRect(30, 12, 10, 4));

        app.triggerRender();
        ScreenBuffer *buf = app.screenBuffer();
        QCOMPARE(buf->backCell(31, 13).ch, U'O');

        overlay->setVisible(false);
        QTest::qWait(20);

        const auto theme = mainWindow.themeProperties();
        for (int row = 12; row < 16; ++row) {
            for (int col = 30; col < 40; ++col) {
                QCOMPARE(buf->backCell(col, row).ch, U' ');
                QVERIFY(buf->backCell(col, row).background == theme.background);
            }
        }
    }

    // Re-showing the overlay must paint it back on top.
    void reshowPaintsOverlayAgain()
    {
        static int argc = 1;
        static char arg0[] = "test";
        static char *argv[] = { arg0, nullptr };
        QTuiApplication app(argc, argv);

        QTuiMainWindow mainWindow;
        mainWindow.setGeometry(QRect(0, 0, 80, 24));
        mainWindow.setVisible(true);
        app.setMainWindow(&mainWindow);

        auto *content = new FillWidget(QLatin1Char('T'), &mainWindow);
        content->setGeometry(QRect(0, 0, 40, 10));
        auto *overlay = new FillWidget(QLatin1Char('O'), &mainWindow);
        overlay->setGeometry(QRect(5, 2, 20, 3));

        app.triggerRender();
        ScreenBuffer *buf = app.screenBuffer();

        overlay->setVisible(false);
        QTest::qWait(20);
        QCOMPARE(buf->backCell(6, 3).ch, U'T');

        overlay->setVisible(true);
        QTest::qWait(20);
        QCOMPARE(buf->backCell(6, 3).ch, U'O');
    }
};

QTEST_APPLESS_MAIN(TestHiddenWidgetRestore)
#include "test_hidden_widget_restore.moc"
