// Synchronized output: a render pass that changes cells writes its bytes
// between CSI ? 2026 h and CSI ? 2026 l. A pass without a changed cell
// writes neither.
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiMainWindow.h>
#include <QtTUI/QTuiWidget.h>
#include <QtTUI/RenderUtils.h>

#include "RecordingBackend.h"

#include <QtTest/QtTest>

using namespace QtTUI;

static const QByteArray kBegin = "\x1b[?2026h";
static const QByteArray kEnd = "\x1b[?2026l";

class TextWidget : public QTuiWidget {
public:
    using QTuiWidget::QTuiWidget;
    QString text = QStringLiteral("one");

    void paintEvent(ScreenBuffer &buffer) override
    {
        clearBackground(buffer, geometry());
        renderText(buffer, geometry().x(), geometry().y(), text, geometry().width(), Cell());
    }
};

class TestSynchronizedOutput : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() { useTemporaryStateDir(); }

    void passIsWrappedOnlyWhenCellsChange()
    {
        QByteArray log;
        int argc = 1;
        char name[] = "test";
        char *argv[] = {name, nullptr};
        QTuiApplication app(argc, argv, std::make_unique<RecordingBackend>(&log));

        QTuiMainWindow window;
        window.setGeometry(QRect(0, 0, 80, 24));
        app.setMainWindow(&window);
        auto *widget = new TextWidget(&window);
        window.setCentralWidget(widget);
        window.show();
        log.clear();

        // The first pass paints all cells
        QCoreApplication::processEvents();
        QVERIFY(log.startsWith(kBegin));
        QVERIFY(log.endsWith(kEnd));
        QCOMPARE(int(log.count(kBegin)), 1);
        QCOMPARE(int(log.count(kEnd)), 1);
        QVERIFY(log.contains("one"));

        // No dirty region: no pass and no bytes
        log.clear();
        QCoreApplication::processEvents();
        QVERIFY(log.isEmpty());

        // A dirty widget with the same cells: the diff is empty
        widget->markDirty();
        QCoreApplication::processEvents();
        QVERIFY2(log.isEmpty(), log.toHex().constData());

        // A changed cell: one wrapped block, with only the changed text
        widget->text = QStringLiteral("two");
        widget->markDirty();
        QCoreApplication::processEvents();
        QVERIFY(log.startsWith(kBegin));
        QVERIFY(log.endsWith(kEnd));
        QCOMPARE(int(log.count(kBegin)), 1);
        QCOMPARE(int(log.count(kEnd)), 1);
        QVERIFY(log.contains("two"));
        QVERIFY(log.size() < 100);
    }
};

QTEST_APPLESS_MAIN(TestSynchronizedOutput)
#include "test_synchronized_output.moc"
