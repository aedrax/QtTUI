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
    // Virtual so concrete layouts that keep their own item bookkeeping
    // (box/grid/form/stacked) report their real contents; nested-layout
    // wrappers rely on count() to decide whether the layout is empty.
    virtual QTuiLayoutItem *itemAt(int index) const;
    virtual int count() const;

    // Aggregated minimum size of the layout's contents, including margins.
    // The base implementation takes the maximum over the base item list;
    // concrete layouts override with their real aggregation rules.
    virtual QSize minimumSize() const;

    // Schedules a re-apply of the last applied rect on the next event-loop
    // iteration (coalesced). Does nothing if the layout has never been
    // applied — the initial show()/relayout pass covers that case.
    void invalidate();

    void setContentsMargins(int left, int top, int right, int bottom);
    void contentsMargins(int *left, int *top, int *right, int *bottom) const;

protected:
    QRect contentsRect(const QRect &availableArea) const;

    // rect mirrored horizontally inside area if the layout's widget (the
    // nearest widget among its parents) is right-to-left; else rect
    QRect visualRect(const QRect &rect, const QRect &area) const;

    // Concrete apply() implementations must call this first so that
    // invalidate() can re-apply the same area later.
    void recordAppliedRect(const QRect &availableArea);

    QList<QTuiLayoutItem *> m_items;
    int m_marginLeft = 0, m_marginTop = 0;
    int m_marginRight = 0, m_marginBottom = 0;
    QRect m_lastAppliedRect;
    bool m_everApplied = false;

private:
    bool m_invalidatePending = false;
};

} // namespace QtTUI

#endif // QTTUI_QTUILAYOUT_H
