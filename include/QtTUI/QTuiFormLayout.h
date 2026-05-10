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
    void removeRow(int row);

    int rowCount() const;

    int count() const override;
    QSize minimumSize() const override;

    void apply(const QRect &availableArea) override;

private:
    struct FormRow {
        QTuiWidget *label;
        QTuiWidget *field;
        bool ownsLabel;
    };

    void watchRowWidgets(const FormRow &row);

    QList<FormRow> m_rows;
};

} // namespace QtTUI

#endif // QTTUI_QTUIFORMLAYOUT_H
