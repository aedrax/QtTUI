// QTuiValidator classes, and QTuiLineEdit maxLength / validator rules.
#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QtTUI/QTuiLineEdit.h>
#include <QtTUI/QTuiValidator.h>
#include <QtTUI/TuiEvents.h>

using namespace QtTUI;

static QTuiValidator::State check(const QTuiValidator &v, const QString &text)
{
    QString input = text;
    int pos = input.length();
    return v.validate(input, pos);
}

static void type(QTuiLineEdit &le, const QString &text)
{
    for (const QChar ch : text) {
        TuiKeyEvent ev(ch.toUpper().unicode(), Qt::NoModifier, QString(ch));
        QCoreApplication::sendEvent(&le, &ev);
    }
}

class TestQTuiValidator : public QObject {
    Q_OBJECT

private slots:
    void intValidatorStates()
    {
        QTuiIntValidator v(1, 100);
        QCOMPARE(check(v, QString()), QTuiValidator::Intermediate);
        QCOMPARE(check(v, QStringLiteral("42")), QTuiValidator::Acceptable);
        QCOMPARE(check(v, QStringLiteral("0")), QTuiValidator::Intermediate); // "0" < 3 digits
        QCOMPARE(check(v, QStringLiteral("500")), QTuiValidator::Invalid);
        QCOMPARE(check(v, QStringLiteral("-")), QTuiValidator::Invalid);      // no negatives
        QCOMPARE(check(v, QStringLiteral("4a")), QTuiValidator::Invalid);

        QTuiIntValidator negative(-50, 50);
        QCOMPARE(check(negative, QStringLiteral("-")), QTuiValidator::Intermediate);
        QCOMPARE(check(negative, QStringLiteral("-7")), QTuiValidator::Acceptable);
    }

    void regularExpressionValidatorStates()
    {
        QTuiRegularExpressionValidator v(QRegularExpression(QStringLiteral("[A-Z]{2}-\\d{3}")));
        QCOMPARE(check(v, QStringLiteral("AB-123")), QTuiValidator::Acceptable);
        QCOMPARE(check(v, QStringLiteral("AB-1")), QTuiValidator::Intermediate);
        QCOMPARE(check(v, QStringLiteral("ab")), QTuiValidator::Invalid);
        QCOMPARE(check(v, QStringLiteral("AB-1234")), QTuiValidator::Invalid);
    }

    void lineEditRejectsInvalidEdits()
    {
        QTuiLineEdit le;
        QTuiIntValidator v(0, 255);
        le.setValidator(&v);
        QSignalSpy returned(&le, &QTuiLineEdit::returnPressed);

        type(le, QStringLiteral("2x5"));
        QCOMPARE(le.text(), QStringLiteral("25")); // "x" rejected
        type(le, QStringLiteral("9"));
        QCOMPARE(le.text(), QStringLiteral("25")); // 259 > 255 rejected

        TuiKeyEvent enter(Qt::Key_Return, Qt::NoModifier);
        QCoreApplication::sendEvent(&le, &enter);
        QCOMPARE(returned.count(), 1);

        le.setText(QString()); // Intermediate: Enter does nothing
        QCoreApplication::sendEvent(&le, &enter);
        QCOMPARE(returned.count(), 1);
    }

    void lineEditMaxLength()
    {
        QTuiLineEdit le;
        le.setMaxLength(4);
        type(le, QStringLiteral("abcdef"));
        QCOMPARE(le.text(), QStringLiteral("abcd"));

        // A paste is cut to the room that is left
        le.setText(QStringLiteral("ab"));
        TuiPasteEvent paste(QStringLiteral("123456"));
        QCoreApplication::sendEvent(&le, &paste);
        QCOMPARE(le.text(), QStringLiteral("ab12"));

        le.setText(QStringLiteral("too long"));
        QCOMPARE(le.text(), QStringLiteral("too "));
        le.setMaxLength(2);
        QCOMPARE(le.text(), QStringLiteral("to"));
    }
};

QTEST_MAIN(TestQTuiValidator)
#include "test_qtui_validator.moc"
