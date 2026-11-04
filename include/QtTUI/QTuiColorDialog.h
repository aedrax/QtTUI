#ifndef QTTUI_QTUICOLORDIALOG_H
#define QTTUI_QTUICOLORDIALOG_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiDialog.h>
#include <QtTUI/ScreenBuffer.h>

namespace QtTUI {

class QTuiLabel;
class QTuiLineEdit;
class QTuiSlider;

// Dialog that lets the user choose a color, like QColorDialog. It has a
// grid of preset colors (the 16 basic colors and a palette), a preview, a
// "#RRGGBB" line edit and one slider for each of red, green and blue. All
// parts show the current color. getColor() is the usual way to use it.
//
// Keys: the arrows move in the grid and step a slider (Page Up and Page
// Down step a slider by 16), Tab moves between the parts, Enter accepts
// from the grid, the line edit and the sliders, and Escape rejects.
//
// Color has no invalid state, so getColor() reports a cancel through ok,
// and selectedColor() is black until the user accepts.
class QTuiColorDialog : public QTuiDialog {
    Q_OBJECT
public:
    explicit QTuiColorDialog(QTuiWidget *parent = nullptr);
    explicit QTuiColorDialog(const Color &initial, QTuiWidget *parent = nullptr);

    // The color that the dialog shows now
    Color currentColor() const;
    void setCurrentColor(const Color &color);

    // The color of the last accept
    Color selectedColor() const;

    // Returns the color that the user chose, or initial if the user
    // canceled; *ok (if given) is true if the user chose OK. An empty
    // title gives the default title.
    static Color getColor(const Color &initial, QTuiWidget *parent = nullptr,
                          const QString &title = QString(), bool *ok = nullptr);

signals:
    void currentColorChanged(const QtTUI::Color &color);
    // The user accepted the dialog with this color
    void colorSelected(const QtTUI::Color &color);

private:
    // Shows m_current in each part
    void syncParts();

    Color m_current;
    Color m_selected;
    QTuiWidget *m_grid = nullptr;
    QTuiWidget *m_swatch = nullptr;
    QTuiLineEdit *m_hexEdit = nullptr;
    QTuiSlider *m_sliders[3] = {};
    QTuiLabel *m_valueLabels[3] = {};
};

} // namespace QtTUI

#endif // QTTUI_QTUICOLORDIALOG_H
