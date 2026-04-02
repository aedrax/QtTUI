#ifndef QTTUI_QTUITOOLBAR_H
#define QTTUI_QTUITOOLBAR_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiWidget.h>
#include <QList>
#include <QString>

namespace QtTUI {

class QTuiAction;

class QTuiToolBar : public QTuiWidget {
    Q_OBJECT
    Q_PROPERTY(bool movable READ isMovable WRITE setMovable)
public:
    explicit QTuiToolBar(const QString &title = QString(), QTuiWidget *parent = nullptr);

    void addAction(QTuiAction *action);
    void addSeparator();
    void addWidget(QTuiWidget *widget);

    bool isMovable() const;
    void setMovable(bool movable);

    QString title() const;

    void paintEvent(ScreenBuffer &buffer) override;
    QSize minimumSize() const override;

protected:
    void keyPressEvent(TuiKeyEvent *event) override;

private:
    struct ToolBarItem {
        QTuiAction *action = nullptr;
        QTuiWidget *widget = nullptr;
        bool isSeparator = false;
    };
    QString m_title;
    QList<ToolBarItem> m_items;
    int m_focusedItem = 0;
    bool m_movable = true;
};

} // namespace QtTUI

#endif // QTTUI_QTUITOOLBAR_H
