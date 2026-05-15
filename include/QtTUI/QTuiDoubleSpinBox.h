#ifndef QTTUI_QTUIDOUBLESPINBOX_H
#define QTTUI_QTUIDOUBLESPINBOX_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiWidget.h>
#include <QString>

namespace QtTUI {

class QTuiDoubleSpinBox : public QTuiWidget {
    Q_OBJECT
    Q_PROPERTY(double value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(double minimum READ minimum WRITE setMinimum)
    Q_PROPERTY(double maximum READ maximum WRITE setMaximum)
    Q_PROPERTY(double singleStep READ singleStep WRITE setSingleStep)
    Q_PROPERTY(int decimals READ decimals WRITE setDecimals)
public:
    explicit QTuiDoubleSpinBox(QTuiWidget *parent = nullptr);

    double value() const;
    void setValue(double value);

    double minimum() const;
    void setMinimum(double min);
    double maximum() const;
    void setMaximum(double max);
    void setRange(double min, double max);

    double singleStep() const;
    void setSingleStep(double step);

    int decimals() const;
    void setDecimals(int decimals);

    QString prefix() const;
    void setPrefix(const QString &prefix);
    QString suffix() const;
    void setSuffix(const QString &suffix);

    void paintEvent(ScreenBuffer &buffer) override;
    QSize minimumSize() const override;
    // Bare printable-key shortcuts must not fire while this widget has focus
    bool acceptsTextInput() const override { return true; }

signals:
    void valueChanged(double value);

protected:
    void keyPressEvent(TuiKeyEvent *event) override;
    void mousePressEvent(TuiMouseEvent *event) override;

private:
    void stepUp();
    void stepDown();

    double m_value = 0.0;
    double m_minimum = 0.0;
    double m_maximum = 99.99;
    double m_singleStep = 1.0;
    int m_decimals = 2;
    QString m_prefix;
    QString m_suffix;
};

} // namespace QtTUI

#endif // QTTUI_QTUIDOUBLESPINBOX_H
