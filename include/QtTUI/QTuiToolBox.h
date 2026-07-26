#ifndef QTTUI_QTUITOOLBOX_H
#define QTTUI_QTUITOOLBOX_H

#include <QtTUI/QTuiFrame.h>
#include <QList>
#include <QString>

namespace QtTUI {

class QTuiToolBox : public QTuiFrame {
    Q_OBJECT
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentChanged)
public:
    explicit QTuiToolBox(QTuiWidget *parent = nullptr);

    int addItem(QTuiWidget *widget, const QString &label);
    void removeItem(int index);
    int count() const;

    QTuiWidget *widget(int index) const;
    QString itemText(int index) const;

    int currentIndex() const;
    void setCurrentIndex(int index);

    void paintEvent(ScreenBuffer &buffer) override;
    QSize minimumSize() const override;

signals:
    void currentChanged(int index);

protected:
    void keyPressEvent(TuiKeyEvent *event) override;
    void mousePressEvent(TuiMouseEvent *event) override;

private:
    struct ToolBoxItem {
        QString label;
        QTuiWidget *widget;
    };
    QList<ToolBoxItem> m_items;
    int m_currentIndex = -1;
    int m_focusedHeader = 0;
};

} // namespace QtTUI

#endif // QTTUI_QTUITOOLBOX_H
