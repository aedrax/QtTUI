#include <QtTUI/QTuiComboBox.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>
#include <QAbstractItemModel>

namespace QtTUI {

// Internal dropdown list for QTuiComboBox. A popup child of the combo, so the
// item list paints in the topmost pass (above sibling widgets, escaping any
// parent clip rect) and receives its own key/mouse events while open. Key
// handling is forwarded back to the combo so keyboard behavior is identical
// whether events arrive here (real pipeline: the popup has focus) or at the
// combo directly (tests, programmatic dispatch).
class QTuiComboBoxPopup : public QTuiWidget {
    Q_OBJECT
public:
    explicit QTuiComboBoxPopup(QTuiComboBox *combo)
        : QTuiWidget(combo)
        , m_combo(combo)
    {
        setFocusPolicy(Qt::StrongFocus); // receives focus while open
        setPopup(true);
        setVisible(false);
    }

    void paintEvent(ScreenBuffer &buffer) override;

    // Clicking outside closes through the combo so m_dropdownOpen stays in sync
    void popupOutsideClick() override
    {
        m_combo->closeDropdown();
    }

protected:
    void keyPressEvent(TuiKeyEvent *event) override
    {
        m_combo->keyPressEvent(event);
    }

    void mousePressEvent(TuiMouseEvent *event) override;

private:
    QTuiComboBox *m_combo;
};

void QTuiComboBoxPopup::paintEvent(ScreenBuffer &buffer)
{
    const QRect geo = geometry();
    if (!isVisible() || geo.isEmpty())
        return;

    const auto theme = m_combo->themeProperties();
    const auto border = Theme_Engine::borderChars(theme.borderStyle);

    // Match the combo body's colors: while the dropdown is open it holds the
    // focus, so the pair reads as one focused control.
    Color fg = theme.foreground;
    Color bg = theme.background;
    if (hasFocus() || m_combo->hasFocus()) {
        fg = theme.focusForeground;
        bg = theme.focusBackground;
    }

    const int gx = geo.x();
    const int gy = geo.y();
    const int gw = geo.width();
    const int gh = geo.height();

    // Clear the popup area
    for (int row = 0; row < gh; ++row) {
        for (int col = 0; col < gw; ++col) {
            int px = gx + col;
            int py = gy + row;
            if (px < buffer.width() && py < buffer.height()) {
                Cell &cell = buffer.backCell(px, py);
                cell.setChar(U' ');
                cell.foreground = fg;
                cell.background = bg;
                cell.bold = false;
                cell.italic = false;
                cell.underline = false;
            }
        }
    }

    // Top row: separator joining the combo body above (replaces its bottom border)
    if (gy < buffer.height()) {
        for (int col = 0; col < gw && (gx + col) < buffer.width(); ++col) {
            Cell &cell = buffer.backCell(gx + col, gy);
            if (col == 0 || col == gw - 1)
                cell.setChar(border.vertical);
            else
                cell.setChar(border.horizontal);
            cell.foreground = fg;
            cell.background = bg;
        }
    }

    // Item rows between the separator and the bottom border
    const int innerWidth = gw - 2;
    const QStringList &items = m_combo->m_items;
    for (int i = 0; i < items.size(); ++i) {
        int rowY = gy + 1 + i;
        if (rowY >= buffer.height() || rowY >= gy + gh - 1)
            break;

        // Left/right borders
        if (gx < buffer.width()) {
            Cell &cell = buffer.backCell(gx, rowY);
            cell.setChar(border.vertical);
            cell.foreground = fg;
            cell.background = bg;
        }
        int rx = gx + gw - 1;
        if (rx < buffer.width()) {
            Cell &cell = buffer.backCell(rx, rowY);
            cell.setChar(border.vertical);
            cell.foreground = fg;
            cell.background = bg;
        }

        bool highlighted = (i == m_combo->m_dropdownHighlight);
        Color itemFg = highlighted ? bg : fg;
        Color itemBg = highlighted ? fg : bg;

        QString itemStr = items.at(i).left(innerWidth);
        for (int col = 0; col < innerWidth; ++col) {
            int px = gx + 1 + col;
            if (px >= buffer.width())
                break;
            Cell &cell = buffer.backCell(px, rowY);
            cell.setChar(col < itemStr.length() ? itemStr.at(col).unicode() : u' ');
            cell.foreground = itemFg;
            cell.background = itemBg;
            cell.bold = highlighted;
        }
    }

    // Bottom border
    int bottomY = gy + gh - 1;
    if (bottomY > gy && bottomY < buffer.height()) {
        for (int col = 0; col < gw && (gx + col) < buffer.width(); ++col) {
            Cell &cell = buffer.backCell(gx + col, bottomY);
            if (col == 0)
                cell.setChar(border.bottomLeft);
            else if (col == gw - 1)
                cell.setChar(border.bottomRight);
            else
                cell.setChar(border.horizontal);
            cell.foreground = fg;
            cell.background = bg;
        }
    }
}

void QTuiComboBoxPopup::mousePressEvent(TuiMouseEvent *event)
{
    if (!isVisible() || event->mouseType() != TuiMouseEvent::Press)
        return;

    // Items are rendered one per row starting at geometry().y() + 1 (row 0 is
    // the separator joining the combo body). Clicks on border rows are ignored.
    int itemIdx = event->y() - geometry().y() - 1;
    if (itemIdx >= 0 && itemIdx < m_combo->m_items.size()) {
        m_combo->setCurrentIndex(itemIdx);
        m_combo->closeDropdown();
    }
}

QTuiComboBox::QTuiComboBox(QTuiWidget *parent)
    : QTuiWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    // Hiding the combo must not leave its dropdown popup floating on screen
    connect(this, &QTuiWidget::visibilityChanged, this, [this](bool visible) {
        if (!visible)
            closeDropdown();
    });
}

void QTuiComboBox::addItem(const QString &text)
{
    m_items.append(text);
    if (m_currentIndex < 0 && !m_items.isEmpty()) {
        setCurrentIndex(0);
    }
}

void QTuiComboBox::addItems(const QStringList &texts)
{
    for (const auto &t : texts)
        addItem(t);
}

void QTuiComboBox::insertItem(int index, const QString &text)
{
    if (index < 0)
        index = 0;
    if (index > m_items.size())
        index = m_items.size();
    m_items.insert(index, text);

    // Adjust current index if insertion was before it
    if (m_currentIndex < 0 && !m_items.isEmpty()) {
        setCurrentIndex(0);
    } else if (index <= m_currentIndex) {
        m_currentIndex++;
    }
}

void QTuiComboBox::removeItem(int index)
{
    if (index < 0 || index >= m_items.size())
        return;

    m_items.removeAt(index);

    if (m_items.isEmpty()) {
        m_currentIndex = -1;
        emit currentIndexChanged(-1);
        emit currentTextChanged(QString());
    } else if (index == m_currentIndex) {
        int newIndex = qMin(m_currentIndex, m_items.size() - 1);
        m_currentIndex = -1; // force update
        setCurrentIndex(newIndex);
    } else if (index < m_currentIndex) {
        m_currentIndex--;
    }
}

void QTuiComboBox::clear()
{
    m_items.clear();
    if (m_currentIndex != -1) {
        m_currentIndex = -1;
        m_editText.clear();
        emit currentIndexChanged(-1);
        emit currentTextChanged(QString());
    }
    closeDropdown();
    m_dropdownHighlight = 0;
}

int QTuiComboBox::count() const
{
    return m_items.size();
}

QString QTuiComboBox::itemText(int index) const
{
    if (index < 0 || index >= m_items.size())
        return QString();
    return m_items.at(index);
}

int QTuiComboBox::currentIndex() const
{
    return m_currentIndex;
}

void QTuiComboBox::setCurrentIndex(int index)
{
    if (index < -1)
        index = -1;
    if (index >= m_items.size())
        index = m_items.size() - 1;

    if (m_currentIndex != index) {
        m_currentIndex = index;
        QString text = currentText();
        if (m_editable)
            m_editText = text;
        emit currentIndexChanged(m_currentIndex);
        emit currentTextChanged(text);
        markDirty();
    }
}

QString QTuiComboBox::currentText() const
{
    if (m_editable && !m_editText.isEmpty())
        return m_editText;
    if (m_currentIndex >= 0 && m_currentIndex < m_items.size())
        return m_items.at(m_currentIndex);
    return QString();
}

bool QTuiComboBox::isEditable() const
{
    return m_editable;
}

void QTuiComboBox::setEditable(bool editable)
{
    m_editable = editable;
    if (editable && m_currentIndex >= 0 && m_currentIndex < m_items.size())
        m_editText = m_items.at(m_currentIndex);
}

void QTuiComboBox::setModel(QAbstractItemModel *model)
{
    if (m_model) {
        disconnect(m_model, nullptr, this, nullptr);
    }
    m_model = model;
    if (m_model) {
        connect(m_model, &QAbstractItemModel::dataChanged, this, &QTuiComboBox::syncItemsFromModel);
        connect(m_model, &QAbstractItemModel::rowsInserted, this, &QTuiComboBox::syncItemsFromModel);
        connect(m_model, &QAbstractItemModel::rowsRemoved, this, &QTuiComboBox::syncItemsFromModel);
        connect(m_model, &QAbstractItemModel::modelReset, this, &QTuiComboBox::syncItemsFromModel);
        syncItemsFromModel();
    }
}

QAbstractItemModel *QTuiComboBox::model() const
{
    return m_model;
}

void QTuiComboBox::syncItemsFromModel()
{
    if (!m_model)
        return;

    m_items.clear();
    int rows = m_model->rowCount();
    for (int i = 0; i < rows; ++i) {
        QModelIndex idx = m_model->index(i, 0);
        m_items.append(m_model->data(idx, Qt::DisplayRole).toString());
    }

    if (m_currentIndex >= m_items.size()) {
        setCurrentIndex(m_items.isEmpty() ? -1 : m_items.size() - 1);
    } else if (m_currentIndex < 0 && !m_items.isEmpty()) {
        setCurrentIndex(0);
    }
}

void QTuiComboBox::openDropdown()
{
    if (m_items.isEmpty())
        return;
    if (!m_popup)
        m_popup = new QTuiComboBoxPopup(this);
    m_dropdownOpen = true;
    m_dropdownHighlight = qMax(0, m_currentIndex);

    // List box directly below the combo body: a separator row, the items, and
    // a bottom border. The separator overlaps the combo's bottom-border row so
    // the two read as one box. As a popup it paints in the topmost pass, above
    // any sibling widgets.
    const QRect geo = geometry();
    m_popup->setGeometry(QRect(geo.x(), geo.y() + 2, geo.width(), m_items.size() + 2));
    m_popup->setVisible(true);
    m_popup->setFocus();
    markDirty(); // indicator flips to ▲
}

void QTuiComboBox::closeDropdown()
{
    if (!m_dropdownOpen)
        return;
    m_dropdownOpen = false;
    if (m_popup)
        m_popup->setVisible(false); // popup focus-restore hands focus back to the combo
    markDirty(); // indicator flips to ▼, bottom border repaints
}

QSize QTuiComboBox::minimumSize() const
{
    // Find widest item text
    int maxTextWidth = 5; // minimum reasonable width
    for (const auto &item : m_items) {
        maxTextWidth = qMax(maxTextWidth, static_cast<int>(item.length()));
    }
    // Content: text + " ▼" indicator + borders + padding. The open dropdown
    // is a separate popup widget and does not affect the combo's own size.
    int width = maxTextWidth + 4 + 2; // 2 for " ▼", 2 for borders, 2 for padding
    int height = 3; // top border + content row + bottom border
    return QSize(width, height);
}

void QTuiComboBox::paintEvent(ScreenBuffer &buffer)
{
    const QRect geo = geometry();
    if (!isVisible() || geo.isEmpty())
        return;

    const auto theme = themeProperties();
    const auto border = Theme_Engine::borderChars(theme.borderStyle);

    const int geoX = geo.x();
    const int geoY = geo.y();
    const int geoW = geo.width();
    const int geoH = geo.height();

    // Determine colors based on focus state. While the dropdown popup is open
    // it holds the focus, so treat the combo body as focused too.
    Color fg = theme.foreground;
    Color bg = theme.background;

    const bool focused = hasFocus() || m_dropdownOpen;
    if (focused) {
        fg = theme.focusForeground;
        bg = theme.focusBackground;
    }

    // Clear the combo body (3 rows: border, content, border)
    int totalH = qMin(geoH, 3);
    for (int row = 0; row < totalH; ++row) {
        for (int col = 0; col < geoW; ++col) {
            int px = geoX + col;
            int py = geoY + row;
            if (px < buffer.width() && py < buffer.height()) {
                Cell &cell = buffer.backCell(px, py);
                cell.setChar(U' ');
                cell.foreground = fg;
                cell.background = bg;
                cell.bold = theme.bold;
                cell.italic = theme.italic;
                cell.underline = theme.underline;
            }
        }
    }

    // Render top border
    if (geoH >= 1) {
        int py = geoY;
        for (int col = 0; col < geoW; ++col) {
            int px = geoX + col;
            if (px < buffer.width() && py < buffer.height()) {
                Cell &cell = buffer.backCell(px, py);
                if (col == 0)
                    cell.setChar(border.topLeft);
                else if (col == geoW - 1)
                    cell.setChar(border.topRight);
                else
                    cell.setChar(border.horizontal);
                cell.foreground = fg;
                cell.background = bg;
            }
        }
    }

    // Render bottom border of the combo body (row 2). When the dropdown popup
    // is open, its top separator row paints over this in the topmost pass.
    if (geoH >= 3) {
        int py = geoY + 2;
        for (int col = 0; col < geoW; ++col) {
            int px = geoX + col;
            if (px < buffer.width() && py < buffer.height()) {
                Cell &cell = buffer.backCell(px, py);
                if (col == 0)
                    cell.setChar(border.bottomLeft);
                else if (col == geoW - 1)
                    cell.setChar(border.bottomRight);
                else
                    cell.setChar(border.horizontal);
                cell.foreground = fg;
                cell.background = bg;
            }
        }
    }

    // Render left/right borders for the content row (row 1)
    if (geoH >= 2) {
        int py = geoY + 1;
        if (py < buffer.height()) {
            int pxLeft = geoX;
            if (pxLeft < buffer.width()) {
                buffer.backCell(pxLeft, py).setChar(border.vertical);
                buffer.backCell(pxLeft, py).foreground = fg;
                buffer.backCell(pxLeft, py).background = bg;
            }
            int pxRight = geoX + geoW - 1;
            if (pxRight < buffer.width()) {
                buffer.backCell(pxRight, py).setChar(border.vertical);
                buffer.backCell(pxRight, py).foreground = fg;
                buffer.backCell(pxRight, py).background = bg;
            }
        }
    }

    // Render current item text + dropdown indicator in the content row
    if (geoH >= 3 && geoW >= 3) {
        int contentRow = geoY + 1;
        int innerWidth = geoW - 2;

        QString displayText = m_editable ? m_editText : currentText();
        // Reserve 2 chars for " ▼" or " ▲"
        int textSpace = innerWidth - 2;
        if (textSpace < 0) textSpace = 0;
        QString truncated = displayText.left(textSpace);

        // Write text left-aligned
        for (int i = 0; i < truncated.length(); ++i) {
            int px = geoX + 1 + i;
            int py = contentRow;
            if (px < buffer.width() && py < buffer.height()) {
                Cell &cell = buffer.backCell(px, py);
                cell.setChar(truncated.at(i).unicode());
                cell.foreground = fg;
                cell.background = bg;
                cell.bold = theme.bold || focused;
            }
        }

        // Write dropdown indicator at the right end
        const char32_t indicator = m_dropdownOpen ? char32_t(0x25B2) : char32_t(0x25BC); // ▲ or ▼
        int indicatorPos = geoX + geoW - 2;
        if (indicatorPos > geoX && indicatorPos < buffer.width() && contentRow < buffer.height()) {
            Cell &cell = buffer.backCell(indicatorPos, contentRow);
            cell.setChar(indicator);
            cell.foreground = fg;
            cell.background = bg;
        }
    }

    // The open item list is rendered by the QTuiComboBoxPopup popup child in
    // the scheduler's topmost pass, not here.
}

void QTuiComboBox::keyPressEvent(TuiKeyEvent *event)
{
    const int key = event->key();

    if (m_dropdownOpen) {
        // Dropdown is open: navigate or select. The highlight is rendered by
        // the popup, so navigation dirties the popup rather than the combo.
        if (key == Qt::Key_Up) {
            if (m_dropdownHighlight > 0) {
                m_dropdownHighlight--;
                if (m_popup)
                    m_popup->markDirty();
            }
        } else if (key == Qt::Key_Down) {
            if (m_dropdownHighlight < m_items.size() - 1) {
                m_dropdownHighlight++;
                if (m_popup)
                    m_popup->markDirty();
            }
        } else if (key == Qt::Key_Return || key == Qt::Key_Enter) {
            // Select highlighted item and close
            setCurrentIndex(m_dropdownHighlight);
            closeDropdown();
        } else if (key == Qt::Key_Escape) {
            // Close without selecting
            closeDropdown();
        }
    } else {
        // Dropdown is closed
        if (key == Qt::Key_Return || key == Qt::Key_Enter || key == Qt::Key_Space) {
            openDropdown();
        } else if (key == Qt::Key_Up) {
            // Quick navigate without opening dropdown
            if (m_currentIndex > 0)
                setCurrentIndex(m_currentIndex - 1);
        } else if (key == Qt::Key_Down) {
            if (m_currentIndex < m_items.size() - 1)
                setCurrentIndex(m_currentIndex + 1);
        } else if (m_editable && !event->text().isEmpty()) {
            // Editable mode: printable characters modify the text
            QChar ch = event->text().at(0);
            if (ch.isPrint()) {
                m_editText.append(ch);
                emit currentTextChanged(m_editText);
                markDirty();
            }
        } else if (m_editable && key == Qt::Key_Backspace) {
            if (!m_editText.isEmpty()) {
                m_editText.chop(1);
                emit currentTextChanged(m_editText);
                markDirty();
            }
        }
    }
}

void QTuiComboBox::mousePressEvent(TuiMouseEvent *event)
{
    if (event->mouseType() != TuiMouseEvent::Press)
        return;

    // A click on the combo body toggles the dropdown. Clicks on the open item
    // list are handled by QTuiComboBoxPopup; while the dropdown is open, real
    // clicks elsewhere are consumed as outside-clicks by QTuiMainWindow before
    // reaching us, so the close branch only serves direct event dispatch.
    if (!m_dropdownOpen)
        openDropdown();
    else
        closeDropdown();
}

} // namespace QtTUI

#include "QTuiComboBox.moc"
