#ifndef QTTUI_QTUILCDNUMBER_H
#define QTTUI_QTUILCDNUMBER_H

#include <QtTUI/QTuiWidget.h>
#include <QString>
#include <QStringList>

namespace QtTUI {

class QTuiLCDNumber : public QTuiWidget {
    Q_OBJECT
    Q_PROPERTY(int digitCount READ digitCount WRITE setDigitCount)
    Q_PROPERTY(int intValue READ intValue)
public:
    enum Mode { Decimal, Hexadecimal, Octal, Binary };

    explicit QTuiLCDNumber(int digitCount = 5, QTuiWidget *parent = nullptr);

    void display(int value);
    void display(double value);
    void display(const QString &text);

    int digitCount() const;
    void setDigitCount(int count);

    Mode mode() const;
    void setMode(Mode mode);

    int intValue() const;
    double doubleValue() const;

    bool checkOverflow(int value) const;
    bool checkOverflow(double value) const;

    void paintEvent(ScreenBuffer &buffer) override;
    QSize minimumSize() const override;

signals:
    void overflow();

private:
    static QStringList segmentArt(QChar digit);

    int m_digitCount;
    Mode m_mode = Decimal;
    QString m_displayText;
    int m_intValue = 0;
    double m_doubleValue = 0.0;
};

} // namespace QtTUI

#endif // QTTUI_QTUILCDNUMBER_H
