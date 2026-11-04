// QTuiColorDialog: keys and clicks go through the main window, as the
// input of a terminal does.
#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QTimer>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiColorDialog.h>
#include <QtTUI/QTuiLineEdit.h>
#include <QtTUI/QTuiMainWindow.h>
#include <QtTUI/QTuiSlider.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

using namespace QtTUI;

namespace {

struct Env {
    Env()
        : app(argc(), argv())
    {
        window.setGeometry(QRect(0, 0, 80, 24));
        window.setVisible(true);
        app.setMainWindow(&window);
    }

    void sendKey(int key, Qt::KeyboardModifiers mods = Qt::NoModifier,
                 const QString &text = QString())
    {
        TuiKeyEvent ev(key, mods, text);
        QCoreApplication::sendEvent(&window, &ev);
    }

    void type(const QString &text)
    {
        for (const QChar ch : text)
            sendKey(ch.toUpper().unicode(), Qt::NoModifier, QString(ch));
    }

    void tab(int count)
    {
        for (int i = 0; i < count; ++i)
            sendKey(Qt::Key_Tab, Qt::NoModifier, QStringLiteral("\t"));
    }

    static int &argc()
    {
        static int c = 1;
        return c;
    }
    static char **argv()
    {
        static char arg0[] = "test";
        static char *args[] = { arg0, nullptr };
        return args;
    }

    QTuiApplication app;
    QTuiMainWindow window;
};

// The parts of the dialog. Tab order: grid, hex text, R, G, B, OK, Cancel.
struct Parts {
    explicit Parts(QTuiColorDialog &dialog)
        : hex(dialog.findChild<QTuiLineEdit *>())
        , sliders(dialog.findChildren<QTuiSlider *>())
        , grid(hex->parentWidget()->parentWidget()->childWidgets().first())
    {
    }
    QTuiLineEdit *hex;
    QList<QTuiSlider *> sliders;
    QTuiWidget *grid;
};

} // namespace

class TestColorDialog : public QObject {
    Q_OBJECT

private slots:
    void partsStayInSync()
    {
        Env env;
        QTuiColorDialog dialog(Color(1, 2, 3));
        Parts parts(dialog);
        QCOMPARE(parts.sliders.size(), 3);
        QCOMPARE(dialog.currentColor(), Color(1, 2, 3));
        QCOMPARE(parts.hex->text(), QStringLiteral("#010203"));
        QCOMPARE(parts.sliders[2]->value(), 3);

        QSignalSpy changed(&dialog, &QTuiColorDialog::currentColorChanged);
        dialog.setCurrentColor(Color(255, 128, 0));
        dialog.setCurrentColor(Color(255, 128, 0)); // no change, no signal
        QCOMPARE(changed.count(), 1);
        QCOMPARE(parts.hex->text(), QStringLiteral("#FF8000"));
        QCOMPARE(parts.sliders[0]->value(), 255);
        QCOMPARE(parts.sliders[1]->value(), 128);
        QCOMPARE(parts.sliders[2]->value(), 0);

        // A slider changes one channel and emits one signal
        parts.sliders[2]->setValue(16);
        QCOMPARE(changed.count(), 2);
        QCOMPARE(dialog.currentColor(), Color(255, 128, 16));
        QCOMPARE(parts.hex->text(), QStringLiteral("#FF8010"));

        // The hex text sets the color only while it is complete
        parts.hex->setText(QStringLiteral("#00ff"));
        QCOMPARE(dialog.currentColor(), Color(255, 128, 16));
        parts.hex->setText(QStringLiteral("#00ff7f"));
        QCOMPARE(dialog.currentColor(), Color(0, 255, 127));
        QCOMPARE(parts.sliders[1]->value(), 255);
        // The dialog does not replace text that the user types
        QCOMPARE(parts.hex->text(), QStringLiteral("#00ff7f"));
    }

    // The dialog and each part have a size, so the dialog shows all parts
    void sizeAndPaint()
    {
        Env env;
        QTuiColorDialog dialog(Color(255, 0, 0));
        Parts parts(dialog);
        dialog.open();
        QCoreApplication::processEvents();

        const QRect geo = dialog.geometry();
        QVERIFY(geo.width() >= 44 && geo.height() >= 15);
        QVERIFY(env.window.geometry().contains(geo));
        const QRect grid = parts.grid->geometry();
        QCOMPARE(grid.size(), QSize(24, 6));
        QVERIFY(parts.hex->geometry().width() >= 7);
        for (QTuiSlider *slider : parts.sliders)
            QVERIFY(slider->geometry().width() >= 10);

        // Each cell has its color as the background: red is cell 1 of row 2
        ScreenBuffer *buf = env.app.screenBuffer();
        QCOMPARE(buf->backCell(grid.x() + 3, grid.y() + 1).background, Color(255, 0, 0));
        QCOMPARE(buf->backCell(grid.x(), grid.y()).background, Color(0, 0, 0));
        // The grid has focus: brackets mark the current color
        QCOMPARE(buf->backCell(grid.x() + 3, grid.y() + 1).text(), QStringLiteral("["));
        // The preview, above the hex text, shows the current color
        const QRect hex = parts.hex->geometry();
        QCOMPARE(buf->backCell(hex.x(), hex.y() - 2).background, Color(255, 0, 0));

        // A new color paints the preview again
        dialog.setCurrentColor(Color(1, 2, 3));
        QCoreApplication::processEvents();
        QCOMPARE(buf->backCell(hex.x(), hex.y() - 2).background, Color(1, 2, 3));
        QCOMPARE(buf->backCell(grid.x() + 3, grid.y() + 1).text(), QStringLiteral(" "));

        // Without focus, a dot marks the current color in place of brackets
        dialog.setCurrentColor(Color(255, 0, 0));
        parts.hex->setFocus();
        QCoreApplication::processEvents();
        QCOMPARE(buf->backCell(grid.x() + 4, grid.y() + 1).text(), QStringLiteral("•"));
        QCOMPARE(buf->backCell(grid.x() + 3, grid.y() + 1).text(), QStringLiteral(" "));
        dialog.reject();
    }

    void gridKeysAndClick()
    {
        Env env;
        QTuiColorDialog dialog; // black: the first cell
        Parts parts(dialog);
        dialog.open();
        QVERIFY(parts.grid->hasFocus());

        env.sendKey(Qt::Key_Right);
        QCOMPARE(dialog.currentColor(), Color(128, 0, 0));
        env.sendKey(Qt::Key_Down);
        QCOMPARE(dialog.currentColor(), Color(255, 0, 0));
        env.sendKey(Qt::Key_Left);
        env.sendKey(Qt::Key_Left); // stops at the edge
        QCOMPARE(dialog.currentColor(), Color(128, 128, 128));
        env.sendKey(Qt::Key_Up);
        env.sendKey(Qt::Key_Up);
        QCOMPARE(dialog.currentColor(), Color(0, 0, 0));

        // A click on the last cell of the first row: silver
        const QRect grid = parts.grid->geometry();
        TuiMouseEvent press(TuiMouseEvent::Press, Qt::LeftButton,
                            QPoint(grid.x() + 7 * 3 + 1, grid.y()));
        QCoreApplication::sendEvent(&env.window, &press);
        QCOMPARE(dialog.currentColor(), Color(192, 192, 192));

        // A color that is not a preset: the arrows start from the last cell
        dialog.setCurrentColor(Color(1, 2, 3));
        env.sendKey(Qt::Key_Down);
        QCOMPARE(dialog.currentColor(), Color(255, 255, 255));

        // Enter in the grid accepts
        QSignalSpy selected(&dialog, &QTuiColorDialog::colorSelected);
        env.sendKey(Qt::Key_Return, Qt::NoModifier, QStringLiteral("\r"));
        QVERIFY(!dialog.isVisible());
        QCOMPARE(dialog.result(), int(QTuiDialog::Accepted));
        QCOMPARE(selected.count(), 1);
        QCOMPARE(dialog.selectedColor(), Color(255, 255, 255));
    }

    void tabOrderAndSliderKeys()
    {
        Env env;
        QTuiColorDialog dialog;
        Parts parts(dialog);
        dialog.open();

        env.tab(1);
        QVERIFY(parts.hex->hasFocus());
        env.tab(1);
        QVERIFY(parts.sliders[0]->hasFocus());
        env.sendKey(Qt::Key_Right);
        env.sendKey(Qt::Key_PageUp);
        QCOMPARE(dialog.currentColor(), Color(17, 0, 0));
        env.sendKey(Qt::Key_PageDown);
        env.sendKey(Qt::Key_PageDown); // stops at 0
        QCOMPARE(dialog.currentColor(), Color(0, 0, 0));
        env.tab(2);
        QVERIFY(parts.sliders[2]->hasFocus());
        env.sendKey(Qt::Key_PageUp);
        QCOMPARE(dialog.currentColor(), Color(0, 0, 16));

        // Shift+Tab goes back; Escape rejects and selects no color
        env.sendKey(Qt::Key_Backtab, Qt::ShiftModifier);
        QVERIFY(parts.sliders[1]->hasFocus());
        QSignalSpy selected(&dialog, &QTuiColorDialog::colorSelected);
        env.sendKey(Qt::Key_Escape);
        QVERIFY(!dialog.isVisible());
        QCOMPARE(dialog.result(), int(QTuiDialog::Rejected));
        QCOMPARE(selected.count(), 0);
        QCOMPARE(dialog.selectedColor(), Color());
    }

    void hexEditKeys()
    {
        Env env;
        QTuiColorDialog dialog(Color(255, 255, 255));
        Parts parts(dialog);
        dialog.open();
        env.tab(1);

        // "#FFFFFF": remove two digits, then type two
        env.sendKey(Qt::Key_Backspace);
        env.sendKey(Qt::Key_Backspace);
        QCOMPARE(dialog.currentColor(), Color(255, 255, 255));
        // Enter does not accept text that is not complete
        env.sendKey(Qt::Key_Return, Qt::NoModifier, QStringLiteral("\r"));
        QVERIFY(dialog.isVisible());
        env.type(QStringLiteral("g")); // not a hex digit: the validator rejects it
        env.type(QStringLiteral("0a"));
        QCOMPARE(parts.hex->text(), QStringLiteral("#FFFF0a"));
        QCOMPARE(dialog.currentColor(), Color(255, 255, 10));
        QCOMPARE(parts.sliders[2]->value(), 10);

        env.sendKey(Qt::Key_Return, Qt::NoModifier, QStringLiteral("\r"));
        QVERIFY(!dialog.isVisible());
        QCOMPARE(dialog.selectedColor(), Color(255, 255, 10));
    }

    void getColor()
    {
        Env env;
        const Color initial(10, 20, 30);

        // Enter on a slider accepts
        bool ok = false;
        QTimer::singleShot(0, [&env]() {
            env.tab(2);
            env.sendKey(Qt::Key_Right);
            env.sendKey(Qt::Key_Return, Qt::NoModifier, QStringLiteral("\r"));
        });
        QCOMPARE(QTuiColorDialog::getColor(initial, nullptr, QStringLiteral("Pick"), &ok),
                 Color(11, 20, 30));
        QVERIFY(ok);

        // Escape: the initial color comes back
        QTimer::singleShot(0, [&env]() {
            env.sendKey(Qt::Key_Right);
            env.sendKey(Qt::Key_Escape);
        });
        QCOMPARE(QTuiColorDialog::getColor(initial, nullptr, QString(), &ok), initial);
        QVERIFY(!ok);

        // Tab to OK, then Enter
        QTimer::singleShot(0, [&env]() {
            env.sendKey(Qt::Key_Right);
            env.tab(5);
            env.sendKey(Qt::Key_Return, Qt::NoModifier, QStringLiteral("\r"));
        });
        QCOMPARE(QTuiColorDialog::getColor(Color(), nullptr, QString(), &ok), Color(128, 0, 0));
        QVERIFY(ok);
    }
};

QTEST_APPLESS_MAIN(TestColorDialog)
#include "test_qtui_colordialog.moc"
