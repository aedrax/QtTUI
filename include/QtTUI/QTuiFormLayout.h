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
    // A row with a nested layout as the field. The form layout takes
    // ownership of the layout.
    void addRow(QTuiWidget *label, QTuiLayout *field);
    void removeRow(int row);

    int rowCount() const;

    // Cells between the label column and the field column (default 0)
    int horizontalSpacing() const;
    void setHorizontalSpacing(int spacing);

    int count() const override;
    QSize minimumSize() const override;

    void apply(const QRect &availableArea) override;

private:
    struct FormRow {
        QTuiWidget *label;
        QTuiWidget *field;
        bool ownsLabel;
        QTuiLayout *fieldLayout = nullptr;
    };

    void watchRowWidgets(const FormRow &row);

    QList<FormRow> m_rows;
    int m_horizontalSpacing = 0;
};

} // namespace QtTUI

#endif // QTTUI_QTUIFORMLAYOUT_H
