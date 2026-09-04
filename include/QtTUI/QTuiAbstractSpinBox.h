#ifndef QTTUI_QTUIABSTRACTSPINBOX_H
#define QTTUI_QTUIABSTRACTSPINBOX_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiWidget.h>
#include <QString>

namespace QtTUI {

// Base class of QTuiSpinBox and QTuiDoubleSpinBox, like QAbstractSpinBox:
// the prefix and the suffix, the box "▲ <prefix><value><suffix> ▼", and the
// keys, clicks and wheel events that step the value. A subclass gives the
// value text and does the step.
class QTuiAbstractSpinBox : public QTuiWidget {
    Q_OBJECT
public:
    explicit QTuiAbstractSpinBox(QTuiWidget *parent = nullptr);

    QString prefix() const;
    void setPrefix(const QString &prefix);
    QString suffix() const;
    void setSuffix(const QString &suffix);

    // Adds steps times the single step to the value
    virtual void stepBy(int steps) = 0;

    void paintEvent(ScreenBuffer &buffer) override;
    // Bare printable-key shortcuts must not fire while this widget has focus
    bool acceptsTextInput() const override { return true; }

protected:
    // The value as text, without the prefix and the suffix
    virtual QString valueText() const = 0;
    // The minimum size for the widest of the two value texts
    QSize minimumSizeFor(const QString &minimumText, const QString &maximumText) const;

    void keyPressEvent(TuiKeyEvent *event) override;
    void mousePressEvent(TuiMouseEvent *event) override;

private:
    // "▲ <prefix><value><suffix> ▼"
    QString content() const;

    QString m_prefix;
    QString m_suffix;
};

} // namespace QtTUI

#endif // QTTUI_QTUIABSTRACTSPINBOX_H
