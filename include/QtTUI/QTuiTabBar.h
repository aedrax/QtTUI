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

private:
    QStringList m_tabs;
    int m_currentIndex = -1;
    TabPosition m_tabPosition = North;
};

} // namespace QtTUI

#endif // QTTUI_QTUITABBAR_H
