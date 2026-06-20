#ifndef QTTUI_QTUIVALIDATOR_H
#define QTTUI_QTUIVALIDATOR_H

#include <QtTUI/QtTUIGlobal.h>
#include <QObject>
#include <QRegularExpression>
#include <QString>

namespace QtTUI {

// Input check for QTuiLineEdit, like QValidator (which is in Qt GUI).
// The line edit rejects an edit that makes the text Invalid, and emits
// returnPressed() only for Acceptable text.
class QTuiValidator : public QObject {
    Q_OBJECT
public:
    enum State { Invalid, Intermediate, Acceptable };

    explicit QTuiValidator(QObject *parent = nullptr);

    // Intermediate: not acceptable yet, but more typing can make it so
    virtual State validate(QString &input, int &pos) const = 0;
    // Tries to make Intermediate input Acceptable. The default does nothing.
    virtual void fixup(QString &input) const;
};

// Integers in [bottom, top]
class QTuiIntValidator : public QTuiValidator {
    Q_OBJECT
public:
    explicit QTuiIntValidator(QObject *parent = nullptr);
    QTuiIntValidator(int bottom, int top, QObject *parent = nullptr);

    int bottom() const;
    int top() const;
    void setRange(int bottom, int top);

    State validate(QString &input, int &pos) const override;

private:
    int m_bottom;
    int m_top;
};

// Text that the whole regular expression matches
class QTuiRegularExpressionValidator : public QTuiValidator {
    Q_OBJECT
public:
    explicit QTuiRegularExpressionValidator(QObject *parent = nullptr);
    QTuiRegularExpressionValidator(const QRegularExpression &re, QObject *parent = nullptr);

    QRegularExpression regularExpression() const;
    void setRegularExpression(const QRegularExpression &re);

    State validate(QString &input, int &pos) const override;

private:
    QRegularExpression m_re;
};

} // namespace QtTUI

#endif // QTTUI_QTUIVALIDATOR_H
