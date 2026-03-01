#ifndef QTTUI_QTUILAYOUT_H
#define QTTUI_QTUILAYOUT_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiLayoutItem.h>
#include <QObject>
#include <QList>
#include <QRect>

namespace QtTUI {

class QTuiWidget;

class QTuiLayout : public QObject {
    Q_OBJECT
public:
    explicit QTuiLayout(QObject *parent = nullptr);
    ~QTuiLayout() override;

    virtual void apply(const QRect &availableArea) = 0;

    void addItem(QTuiLayoutItem *item);
    void addWidget(QTuiWidget *widget);
    QTuiLayoutItem *itemAt(int index) const;
    int count() const;

    void setContentsMargins(int left, int top, int right, int bottom);
    void contentsMargins(int *left, int *top, int *right, int *bottom) const;

protected:
    QRect contentsRect(const QRect &availableArea) const;

    QList<QTuiLayoutItem *> m_items;
    int m_marginLeft = 0, m_marginTop = 0;
    int m_marginRight = 0, m_marginBottom = 0;
};

} // namespace QtTUI

#endif // QTTUI_QTUILAYOUT_H
