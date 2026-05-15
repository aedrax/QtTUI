// Widgets painted by their parent, driven through the real pipeline
// (QTuiApplication + RenderScheduler + ScreenBuffer).
//
// Original bug: the content of a QTuiScrollArea has geometry relative to the
// scroll area, but the render pass and the mouse hit test read it as a screen
// position. The content painted a second time at the top-left of the screen,
// and it took mouse clicks there. The scroll area now marks its content with
// setPaintedByParent(true), and both tree walks skip it.
#include <QtTest/QtTest>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiMainWindow.h>
#include <QtTUI/QTuiScrollArea.h>
#include <QtTUI/QTuiWidget.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

using namespace QtTUI;

// Fills its entire geometry with a fixed character.
class FillWidget : public QTuiWidget {
    Q_OBJECT
public:
    explicit FillWidget(QChar ch, QTuiWidget *parent = nullptr)
        : QTuiWidget(parent)
        , m_ch(ch)
    {
        setFocusPolicy(Qt::StrongFocus);
    }

    void paintEvent(ScreenBuffer &buffer) override
    {
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

// Central widget that holds a scroll area at a fixed position.
class Holder : public FillWidget {
    Q_OBJECT
public:
    explicit Holder(QTuiWidget *parent = nullptr)
        : FillWidget(QLatin1Char('.'), parent)
        , scrollArea(new QTuiScrollArea(this))
    {
    }

    QTuiScrollArea *scrollArea;
};

class TestPaintedByParent : public QObject {
    Q_OBJECT

private slots:
    void scrollContentPaintsOnlyInsideViewport()
    {
        static int argc = 1;
        static char arg0[] = "test";
        static char *argv[] = { arg0, nullptr };
        QTuiApplication app(argc, argv);

        QTuiMainWindow mainWindow;
        mainWindow.setGeometry(QRect(0, 0, 40, 20));
        app.setMainWindow(&mainWindow);

        auto *holder = new Holder;
        mainWindow.setCentralWidget(holder);

        auto *content = new FillWidget(QLatin1Char('X'), holder->scrollArea);
        content->setGeometry(QRect(0, 0, 6, 10));
        holder->scrollArea->setWidget(content);
        QVERIFY(content->isPaintedByParent());

        mainWindow.show();
        holder->scrollArea->setGeometry(QRect(10, 10, 10, 5));
        app.triggerRender();

        ScreenBuffer *buf = app.screenBuffer();
        // Inside the viewport: the content
        QCOMPARE(buf->backCell(10, 10).ch, U'X');
        // At the content's local position: not the content
        QVERIFY(buf->backCell(0, 0).ch != U'X');
        QVERIFY(buf->backCell(5, 5).ch != U'X');

        // A click at the content's local position must not reach the content
        TuiMouseEvent ev(TuiMouseEvent::Press, Qt::LeftButton, QPoint(5, 5));
        QCoreApplication::sendEvent(&mainWindow, &ev);
        QVERIFY(!content->hasFocus());
        QVERIFY(holder->hasFocus());
    }

    void setWidgetClearsFlagOnOldContent()
    {
        QTuiScrollArea area;
        auto *first = new FillWidget(QLatin1Char('A'), &area);
        auto *second = new FillWidget(QLatin1Char('B'), &area);
        area.setWidget(first);
        area.setWidget(second);
        QVERIFY(!first->isPaintedByParent());
        QVERIFY(second->isPaintedByParent());
    }
};

QTEST_MAIN(TestPaintedByParent)
#include "test_painted_by_parent.moc"
