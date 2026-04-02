#ifndef QTTUI_QTUIWIDGETACTION_H
#define QTTUI_QTUIWIDGETACTION_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiAction.h>
#include <QList>

namespace QtTUI {

class QTuiWidget;

class QTuiWidgetAction : public QTuiAction {
    Q_OBJECT
public:
    explicit QTuiWidgetAction(QObject *parent = nullptr);
    explicit QTuiWidgetAction(const QString &text, QObject *parent = nullptr);
    ~QTuiWidgetAction() override;

    virtual QTuiWidget *createWidget(QTuiWidget *parent);
    virtual void deleteWidget(QTuiWidget *widget);

    QTuiWidget *requestWidget(QTuiWidget *parent);
    void releaseWidget(QTuiWidget *widget);

    QList<QTuiWidget *> createdWidgets() const;

private:
    QList<QTuiWidget *> m_createdWidgets;
};

} // namespace QtTUI

#endif // QTTUI_QTUIWIDGETACTION_H
