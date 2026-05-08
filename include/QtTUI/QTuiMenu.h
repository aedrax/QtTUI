#ifndef QTTUI_QTUIMENU_H
#define QTTUI_QTUIMENU_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiWidget.h>
#include <QtTUI/QTuiAction.h>
#include <QList>
#include <QPoint>
#include <QString>

namespace QtTUI {

class QTuiMenu : public QTuiWidget {
    Q_OBJECT
    Q_PROPERTY(QString title READ title WRITE setTitle)
public:
    explicit QTuiMenu(const QString &title = QString(), QTuiWidget *parent = nullptr);

    QString title() const;
    void setTitle(const QString &title);

    QTuiAction *addAction(const QString &text);
    QTuiAction *addAction(const QString &text, const QString &shortcut);
    void addSeparator();
    QTuiMenu *addMenu(const QString &title);
    void addMenu(QTuiMenu *menu);

    void popup(const QPoint &pos);
    void close();

    QList<QTuiAction *> actions() const;

    void paintEvent(ScreenBuffer &buffer) override;
    QSize minimumSize() const override;

signals:
    void aboutToShow();
    void aboutToHide();

protected:
    void keyPressEvent(TuiKeyEvent *event) override;
    void mousePressEvent(TuiMouseEvent *event) override;

private:
    struct MenuItem {
        QTuiAction *action = nullptr;
        QTuiMenu *submenu = nullptr;
        bool isSeparator = false;
    };

    void navigateToNextItem();
    void navigateToPrevItem();

    QString m_title;
    QList<MenuItem> m_menuItems;
    int m_highlightedIndex = 0;
    bool m_isPopup = false;
};

} // namespace QtTUI

#endif // QTTUI_QTUIMENU_H
