#ifndef QTTUI_QTUIFORMLAYOUT_H
#define QTTUI_QTUIFORMLAYOUT_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiLayout.h>
#include <QList>
#include <QString>

namespace QtTUI {

class QTuiWidget;
class QTuiLabel;

class QTuiFormLayout : public QTuiLayout {
    Q_OBJECT
public:
    explicit QTuiFormLayout(QObject *parent = nullptr);
    ~QTuiFormLayout() override;

    void addRow(const QString &labelText, QTuiWidget *field);
    void addRow(QTuiWidget *label, QTuiWidget *field);

    int rowCount() const;

    void apply(const QRect &availableArea) override;

private:
    struct FormRow {
        QTuiWidget *label;
        QTuiWidget *field;
        bool ownsLabel;
    };
    QList<FormRow> m_rows;
};

} // namespace QtTUI

#endif // QTTUI_QTUIFORMLAYOUT_H
