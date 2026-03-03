#ifndef QTTUI_QTUISTACKEDLAYOUT_H
#define QTTUI_QTUISTACKEDLAYOUT_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiLayout.h>
#include <QList>

namespace QtTUI {

class QTuiWidget;

class QTuiStackedLayout : public QTuiLayout {
    Q_OBJECT
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentChanged)
public:
    explicit QTuiStackedLayout(QObject *parent = nullptr);

    int addWidget(QTuiWidget *widget);
    QTuiWidget *widget(int index) const;

    int currentIndex() const;
    void setCurrentIndex(int index);
    QTuiWidget *currentWidget() const;
    void setCurrentWidget(QTuiWidget *widget);

    void apply(const QRect &availableArea) override;

signals:
    void currentChanged(int index);

private:
    QList<QTuiWidget *> m_widgets;
    int m_currentIndex = -1;
};

} // namespace QtTUI

#endif // QTTUI_QTUISTACKEDLAYOUT_H
