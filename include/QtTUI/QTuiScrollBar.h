#ifndef QTTUI_QTUISCROLLBAR_H
#define QTTUI_QTUISCROLLBAR_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiWidget.h>

namespace QtTUI {

class QTuiScrollBar : public QTuiWidget {
    Q_OBJECT
    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(Qt::Orientation orientation READ orientation)
public:
    explicit QTuiScrollBar(Qt::Orientation orientation, QTuiWidget *parent = nullptr);

    int value() const;
    void setValue(int value);

    int minimum() const;
    void setMinimum(int min);
    int maximum() const;
    void setMaximum(int max);
    void setRange(int min, int max);

    int singleStep() const;
    void setSingleStep(int step);
    int pageStep() const;
    void setPageStep(int step);

    Qt::Orientation orientation() const;

    void paintEvent(ScreenBuffer &buffer) override;
    QSize minimumSize() const override;

signals:
    void valueChanged(int value);

protected:
    void keyPressEvent(TuiKeyEvent *event) override;
    void mousePressEvent(TuiMouseEvent *event) override;

private:
    int m_value = 0;
    int m_minimum = 0;
    int m_maximum = 0;
    int m_singleStep = 1;
    int m_pageStep = 10;
    Qt::Orientation m_orientation;
};

} // namespace QtTUI

#endif // QTTUI_QTUISCROLLBAR_H
