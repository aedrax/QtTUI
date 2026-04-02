#ifndef QTTUI_QTUIMENUBAR_H
#define QTTUI_QTUIMENUBAR_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiWidget.h>
#include <QList>
#include <QString>

namespace QtTUI {

class QTuiMenu;

class QTuiMenuBar : public QTuiWidget {
    Q_OBJECT
public:
    explicit QTuiMenuBar(QTuiWidget *parent = nullptr);

    QTuiMenu *addMenu(const QString &title);
    void addMenu(QTuiMenu *menu);

    QList<QTuiMenu *> menus() const;

    void paintEvent(ScreenBuffer &buffer) override;
    QSize minimumSize() const override;

protected:
    void keyPressEvent(TuiKeyEvent *event) override;

private:
    void openMenu(int index);
    void closeCurrentMenu();
    int findMnemonicIndex(QChar ch) const;

    QList<QTuiMenu *> m_menus;
    int m_activeMenuIndex = -1;
    bool m_menuOpen = false;
};

} // namespace QtTUI

#endif // QTTUI_QTUIMENUBAR_H
