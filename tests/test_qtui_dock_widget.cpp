#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QCoreApplication>
#include <QtTUI/QTuiDockWidget.h>
#include <QtTUI/QTuiWidget.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>
#include <QtTUI/Theme_Engine.h>

using namespace QtTUI;

static void sendKey(QTuiWidget &w, int key, Qt::KeyboardModifiers mods = Qt::NoModifier,
                    const QString &text = QString())
{
    TuiKeyEvent ev(key, mods, text);
    QCoreApplication::sendEvent(&w, &ev);
}

// Minimal concrete widget for use as dock content
class DockContentWidget : public QTuiWidget {
    Q_OBJECT
public:
    explicit DockContentWidget(QTuiWidget *parent = nullptr) : QTuiWidget(parent) {}
    void paintEvent(ScreenBuffer &) override {}
    QSize minimumSize() const override { return QSize(8, 3); }
};

class TestQTuiDockWidget : public QObject {
    Q_OBJECT
private slots:

    void inheritsFromQTuiWidget()
    {
        QTuiDockWidget dock;
        auto *asWidget = qobject_cast<QTuiWidget *>(&dock);
        QVERIFY(asWidget != nullptr);
    }

    void constructionWithTitle()
    {
        QTuiDockWidget dock(QStringLiteral("Explorer"));
        QCOMPARE(dock.windowTitle(), QStringLiteral("Explorer"));
    }

    void defaultConstruction()
    {
        QTuiDockWidget dock;
        QCOMPARE(dock.windowTitle(), QString());
        QCOMPARE(dock.isFloatable(), true);
        QCOMPARE(dock.allowedAreas(), Qt::AllDockWidgetAreas);
        QVERIFY(dock.widget() == nullptr);
    }

    void setWindowTitle()
    {
        QTuiDockWidget dock;
        dock.setWindowTitle(QStringLiteral("Output"));
        QCOMPARE(dock.windowTitle(), QStringLiteral("Output"));
    }

    void paintEventRendersTitleAndCloseIndicator()
    {
        QTuiDockWidget dock(QStringLiteral("Log"));
        dock.setGeometry(QRect(0, 0, 20, 5));

        ScreenBuffer buffer(20, 5);
        dock.paintEvent(buffer);

        // Extract the title bar row
        QString titleRow;
        for (int col = 0; col < 20; ++col)
            titleRow += buffer.backCell(col, 0).character;

        QVERIFY(titleRow.contains(QStringLiteral("Log")));
        QVERIFY(titleRow.contains(QStringLiteral("[X]")));
    }

    void setWidgetAcceptsSingleChild()
    {
        QTuiDockWidget dock(QStringLiteral("Props"));
        auto *content = new DockContentWidget();
        dock.setWidget(content);
        QCOMPARE(dock.widget(), content);
    }

    void setWidgetParentsChild()
    {
        QTuiDockWidget dock(QStringLiteral("Props"));
        auto *content = new DockContentWidget();
        dock.setWidget(content);
        QCOMPARE(content->parentWidget(), &dock);
    }

    void contentWidgetRenderedInsideBorder()
    {
        QTuiDockWidget dock(QStringLiteral("Test"));
        auto *content = new DockContentWidget();
        dock.setWidget(content);
        dock.setGeometry(QRect(0, 0, 20, 8));

        ScreenBuffer buffer(20, 8);
        dock.paintEvent(buffer);

        // Content widget should be positioned inside the border
        QRect contentGeo = content->geometry();
        QVERIFY(contentGeo.x() >= 1);
        QVERIFY(contentGeo.y() >= 1);
        QVERIFY(contentGeo.right() < 19);
        QVERIFY(contentGeo.bottom() < 7);
    }


    void defaultAllowedAreasIsAll()
    {
        QTuiDockWidget dock;
        QCOMPARE(dock.allowedAreas(), Qt::AllDockWidgetAreas);
    }

    void setAllowedAreasLeftRight()
    {
        QTuiDockWidget dock;
        dock.setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        QCOMPARE(dock.allowedAreas(), Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    }

    void setAllowedAreasSingle()
    {
        QTuiDockWidget dock;
        dock.setAllowedAreas(Qt::BottomDockWidgetArea);
        QCOMPARE(dock.allowedAreas(), Qt::BottomDockWidgetArea);
    }

    void setAllowedAreasTopBottom()
    {
        QTuiDockWidget dock;
        dock.setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
        Qt::DockWidgetAreas expected = Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea;
        QCOMPARE(dock.allowedAreas(), expected);
    }

    void escapeHidesDockAndEmitsSignal()
    {
        QTuiDockWidget dock(QStringLiteral("Panel"));
        dock.setGeometry(QRect(0, 0, 20, 5));
        dock.setFocus();

        QSignalSpy spy(&dock, &QTuiDockWidget::visibilityChanged);
        QVERIFY(dock.isVisible());

        sendKey(dock, Qt::Key_Escape);

        QVERIFY(!dock.isVisible());
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toBool(), false);
    }

    void qKeyHidesDockAndEmitsSignal()
    {
        QTuiDockWidget dock(QStringLiteral("Panel"));
        dock.setGeometry(QRect(0, 0, 20, 5));
        dock.setFocus();

        QSignalSpy spy(&dock, &QTuiDockWidget::visibilityChanged);
        QVERIFY(dock.isVisible());

        sendKey(dock, Qt::Key_Q);

        QVERIFY(!dock.isVisible());
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toBool(), false);
    }

    void defaultFloatableIsTrue()
    {
        QTuiDockWidget dock;
        QCOMPARE(dock.isFloatable(), true);
    }

    void setFloatableFalse()
    {
        QTuiDockWidget dock;
        dock.setFloatable(false);
        QCOMPARE(dock.isFloatable(), false);
    }

    void setFloatableToggle()
    {
        QTuiDockWidget dock;
        dock.setFloatable(false);
        QCOMPARE(dock.isFloatable(), false);
        dock.setFloatable(true);
        QCOMPARE(dock.isFloatable(), true);
    }

    // --- minimumSize ---

    void minimumSizeWithoutContent()
    {
        QTuiDockWidget dock(QStringLiteral("Hi"));
        QSize ms = dock.minimumSize();
        QVERIFY(ms.width() >= 8); // "Hi" + "[X]" + borders
        QCOMPARE(ms.height(), 3); // title + 1 content row + bottom border
    }

    void minimumSizeWithContent()
    {
        QTuiDockWidget dock(QStringLiteral("X"));
        auto *content = new DockContentWidget(); // 8x3 minimum
        dock.setWidget(content);
        QSize ms = dock.minimumSize();
        QVERIFY(ms.width() >= 10); // 8 content + 2 borders
        QCOMPARE(ms.height(), 3);
    }

    // --- Paint event doesn't crash with various states ---

    void paintEventNoCrashEmpty()
    {
        QTuiDockWidget dock;
        dock.setGeometry(QRect(0, 0, 15, 4));
        ScreenBuffer buffer(15, 4);
        dock.paintEvent(buffer); // should not crash
    }

    void paintEventNoCrashHidden()
    {
        QTuiDockWidget dock(QStringLiteral("Hidden"));
        dock.setVisible(false);
        dock.setGeometry(QRect(0, 0, 15, 4));
        ScreenBuffer buffer(15, 4);
        dock.paintEvent(buffer); // should not crash, should not render
    }

    void paintEventRendersBottomBorder()
    {
        QTuiDockWidget dock(QStringLiteral("Dock"));
        dock.setGeometry(QRect(0, 0, 15, 4));

        ScreenBuffer buffer(15, 4);
        dock.paintEvent(buffer);

        // Bottom row should have border characters
        QChar bottomLeft = buffer.backCell(0, 3).character;
        QChar bottomRight = buffer.backCell(14, 3).character;
        // They should be border chars (not spaces)
        QVERIFY(bottomLeft != QLatin1Char(' '));
        QVERIFY(bottomRight != QLatin1Char(' '));
    }
};

QTEST_MAIN(TestQTuiDockWidget)
#include "test_qtui_dock_widget.moc"
