#ifndef QTTUI_QTUISTACKEDWIDGET_H
#define QTTUI_QTUISTACKEDWIDGET_H

#include <QtTUI/QTuiWidget.h>
#include <QList>

namespace QtTUI {

class QTuiStackedWidget : public QTuiWidget {
    Q_OBJECT
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentChanged)
public:
    explicit QTuiStackedWidget(QTuiWidget *parent = nullptr);

    int addWidget(QTuiWidget *widget);
    void removeWidget(QTuiWidget *widget);
    int count() const;

    QTuiWidget *widget(int index) const;
    int indexOf(QTuiWidget *widget) const;

    int currentIndex() const;
    void setCurrentIndex(int index);
    QTuiWidget *currentWidget() const;
    void setCurrentWidget(QTuiWidget *widget);

    QSize minimumSize() const override;
    void paintEvent(ScreenBuffer &buffer) override;

signals:
    void currentChanged(int index);

private:
    QList<QTuiWidget *> m_widgets;
    int m_currentIndex = -1;
};

} // namespace QtTUI

#endif // QTTUI_QTUISTACKEDWIDGET_H
