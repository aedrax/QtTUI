// QTuiCompleter: matching rules, and the popup flow inside QTuiLineEdit
// (typing opens the popup, Down/Enter accepts, Escape closes).
#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QtTUI/QTuiCompleter.h>
#include <QtTUI/QTuiLineEdit.h>
#include <QtTUI/TuiEvents.h>

using namespace QtTUI;

static void sendKey(QTuiWidget &w, int key, const QString &text = QString())
{
    TuiKeyEvent ev(key, Qt::NoModifier, text);
    QCoreApplication::sendEvent(&w, &ev);
}

static void type(QTuiWidget &w, const QString &text)
{
    for (const QChar ch : text)
        sendKey(w, ch.toUpper().unicode(), QString(ch));
}

class TestQTuiCompleter : public QObject {
    Q_OBJECT

private slots:
    void matchingRules()
    {
        QTuiCompleter c({QStringLiteral("apple"), QStringLiteral("Apricot"),
                         QStringLiteral("banana"), QStringLiteral("pineapple")});

        c.setCompletionPrefix(QStringLiteral("ap"));
        QCOMPARE(c.completions(), QStringList{QStringLiteral("apple")});

        c.setCaseSensitivity(Qt::CaseInsensitive);
        c.setCompletionPrefix(QStringLiteral("ap"));
        QCOMPARE(c.completions(), (QStringList{QStringLiteral("apple"), QStringLiteral("Apricot")}));
        QCOMPARE(c.currentCompletion(), QStringLiteral("apple"));

        c.setFilterMode(QTuiCompleter::Contains);
        c.setCompletionPrefix(QStringLiteral("apple"));
        QCOMPARE(c.completions(), (QStringList{QStringLiteral("apple"), QStringLiteral("pineapple")}));
    }

    void lineEditPopupFlow()
    {
        QTuiLineEdit le;
        le.setGeometry(QRect(0, 0, 20, 1));
        QTuiCompleter c({QStringLiteral("red"), QStringLiteral("green"), QStringLiteral("grey")});
        le.setCompleter(&c);
        QSignalSpy activated(&c, &QTuiCompleter::activated);
        QSignalSpy returnSpy(&le, &QTuiLineEdit::returnPressed);

        type(le, QStringLiteral("gr"));
        QVERIFY(c.isPopupVisible());
        QCOMPARE(c.completionCount(), 2);

        // Down selects "grey"; Enter accepts it and is not a return press
        sendKey(le, Qt::Key_Down);
        sendKey(le, Qt::Key_Return);
        QVERIFY(!c.isPopupVisible());
        QCOMPARE(le.text(), QStringLiteral("grey"));
        QCOMPARE(activated.count(), 1);
        QCOMPARE(returnSpy.count(), 0);

        // With the popup closed, Enter is a normal return press
        sendKey(le, Qt::Key_Return);
        QCOMPARE(returnSpy.count(), 1);

        // Escape closes the popup and keeps the typed text
        le.setText(QString());
        type(le, QStringLiteral("r"));
        QVERIFY(c.isPopupVisible());
        sendKey(le, Qt::Key_Escape);
        QVERIFY(!c.isPopupVisible());
        QCOMPARE(le.text(), QStringLiteral("r"));

        // No match: no popup
        type(le, QStringLiteral("x"));
        QVERIFY(!c.isPopupVisible());
    }
};

QTEST_MAIN(TestQTuiCompleter)
#include "test_qtui_completer.moc"
