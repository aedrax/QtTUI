#ifndef QTTUI_QTUITABBAR_H
#define QTTUI_QTUITABBAR_H

#include <QtTUI/QTuiWidget.h>
#include <QStringList>

namespace QtTUI {

class QTuiTabBar : public QTuiWidget {
    Q_OBJECT
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentChanged)
public:
    enum TabPosition { North, South };

    explicit QTuiTabBar(QTuiWidget *parent = nullptr);

    int addTab(const QString &label);
    void removeTab(int index);
    int count() const;

    QString tabText(int index) const;
    void setTabText(int index, const QString &text);

    int currentIndex() const;
    void setCurrentIndex(int index);

    TabPosition tabPosition() const;
    void setTabPosition(TabPosition position);

    void paintEvent(ScreenBuffer &buffer) override;
    QSize minimumSize() const override;

signals:
    void currentChanged(int index);

protected:
    void keyPressEvent(TuiKeyEvent *event) override;
    void mousePressEvent(TuiMouseEvent *event) override;

private:
    // The full tab row "Tab1 | Tab2 | Tab3", with the start column and
    // length of each tab label in it
    QString tabLine(QList<QPair<int, int>> *ranges) const;
    // First column of the tab row that shows at the left edge. If the row
    // is wider than the bar, the offset keeps the current tab visible.
    int scrollOffset(const QList<QPair<int, int>> &ranges, int lineLength, int width) const;

    QStringList m_tabs;
    int m_currentIndex = -1;
    TabPosition m_tabPosition = North;
};

} // namespace QtTUI

#endif // QTTUI_QTUITABBAR_H
