#ifndef QTTUI_QTUITABWIDGET_H
#define QTTUI_QTUITABWIDGET_H

#include <QtTUI/QTuiWidget.h>
#include <QtTUI/QTuiTabBar.h>
#include <QList>

namespace QtTUI {

class QTuiTabWidget : public QTuiWidget {
    Q_OBJECT
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentChanged)
public:
    explicit QTuiTabWidget(QTuiWidget *parent = nullptr);

    int addTab(QTuiWidget *page, const QString &label);
    void removeTab(int index);
    int count() const;

    QTuiWidget *widget(int index) const;
    int indexOf(QTuiWidget *widget) const;

    int currentIndex() const;
    void setCurrentIndex(int index);
    QTuiWidget *currentWidget() const;

    QTuiTabBar *tabBar() const;

    void paintEvent(ScreenBuffer &buffer) override;

signals:
    void currentChanged(int index);

protected:
    void resizeEvent(TuiResizeEvent *event) override;

private slots:
    void onTabChanged(int index);

private:
    void updatePageGeometries();

    QTuiTabBar *m_tabBar;
    QList<QTuiWidget *> m_pages;
    int m_currentIndex = -1;
};

} // namespace QtTUI

#endif // QTTUI_QTUITABWIDGET_H
