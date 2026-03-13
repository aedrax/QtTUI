#ifndef QTTUI_QTUIPROGRESSBAR_H
#define QTTUI_QTUIPROGRESSBAR_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiWidget.h>
#include <QString>
#include <QChar>

namespace QtTUI {

class QTuiProgressBar : public QTuiWidget {
    Q_OBJECT
    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(int minimum READ minimum WRITE setMinimum)
    Q_PROPERTY(int maximum READ maximum WRITE setMaximum)
    Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation)
public:
    explicit QTuiProgressBar(QTuiWidget *parent = nullptr);

    int value() const;
    void setValue(int value);

    int minimum() const;
    void setMinimum(int min);
    int maximum() const;
    void setMaximum(int max);
    void setRange(int min, int max);

    Qt::Orientation orientation() const;
    void setOrientation(Qt::Orientation orientation);

    QString format() const;
    void setFormat(const QString &format);

    QChar fillChar() const;
    void setFillChar(const QChar &ch);
    QChar emptyChar() const;
    void setEmptyChar(const QChar &ch);

    bool isIndeterminate() const;

    void paintEvent(ScreenBuffer &buffer) override;
    QSize minimumSize() const override;

signals:
    void valueChanged(int value);

private:
    int m_value = 0;
    int m_minimum = 0;
    int m_maximum = 100;
    Qt::Orientation m_orientation = Qt::Horizontal;
    QString m_format = QStringLiteral("%p%");
    QChar m_fillChar = QChar(0x2588);  // █ full block
    QChar m_emptyChar = QChar(0x2591); // ░ light shade
    int m_indeterminatePos = 0;        // animation counter for indeterminate mode
};

} // namespace QtTUI

#endif // QTTUI_QTUIPROGRESSBAR_H
