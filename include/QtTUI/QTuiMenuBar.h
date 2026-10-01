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

    // Opens the menu whose &-mnemonic (or, failing that, first title letter)
    // matches ch; returns false if no menu matches. Used by the bar's own
    // Alt+letter handling and by QTuiMainWindow's global Alt+mnemonic
    // dispatch, which works without the bar having focus.
    bool triggerMnemonic(QChar ch);

    void paintEvent(ScreenBuffer &buffer) override;
    QSize minimumSize() const override;

protected:
    void keyPressEvent(TuiKeyEvent *event) override;
    void mousePressEvent(TuiMouseEvent *event) override;
    QRect hoverRect() const override;

private:
    // The menu title at a screen column, and its columns; -1 if none
    int menuAt(int x, int *start = nullptr, int *width = nullptr) const;
    void registerMenu(QTuiMenu *menu);
    void openMenu(int index);
    void closeCurrentMenu();
    int findMnemonicIndex(QChar ch) const;

    QList<QTuiMenu *> m_menus;
    int m_activeMenuIndex = -1;
    bool m_menuOpen = false;
};

} // namespace QtTUI

#endif // QTTUI_QTUIMENUBAR_H
