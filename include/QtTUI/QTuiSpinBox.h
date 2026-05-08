#ifndef QTTUI_QTUISPINBOX_H
#define QTTUI_QTUISPINBOX_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiWidget.h>
#include <QString>

namespace QtTUI {

class QTuiSpinBox : public QTuiWidget {
    Q_OBJECT
    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(int minimum READ minimum WRITE setMinimum)
    Q_PROPERTY(int maximum READ maximum WRITE setMaximum)
    Q_PROPERTY(int singleStep READ singleStep WRITE setSingleStep)
    Q_PROPERTY(bool wrapping READ wrapping WRITE setWrapping)
public:
    explicit QTuiSpinBox(QTuiWidget *parent = nullptr);

    int value() const;
    void setValue(int value);

    int minimum() const;
    void setMinimum(int min);
    int maximum() const;
    void setMaximum(int max);
    void setRange(int min, int max);

    int singleStep() const;
    void setSingleStep(int step);

    bool wrapping() const;
    void setWrapping(bool wrapping);

    QString prefix() const;
    void setPrefix(const QString &prefix);
    QString suffix() const;
    void setSuffix(const QString &suffix);

    void paintEvent(ScreenBuffer &buffer) override;
    QSize minimumSize() const override;

signals:
    void valueChanged(int value);

protected:
    void keyPressEvent(TuiKeyEvent *event) override;
    void mousePressEvent(TuiMouseEvent *event) override;

private:
    void stepUp();
    void stepDown();

    int m_value = 0;
    int m_minimum = 0;
    int m_maximum = 99;
    int m_singleStep = 1;
    bool m_wrapping = false;
    QString m_prefix;
    QString m_suffix;
};

} // namespace QtTUI

#endif // QTTUI_QTUISPINBOX_H
