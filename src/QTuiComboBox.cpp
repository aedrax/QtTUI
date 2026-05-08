#include <QtTUI/QTuiComboBox.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>
#include <QAbstractItemModel>

namespace QtTUI {

QTuiComboBox::QTuiComboBox(QTuiWidget *parent)
    : QTuiWidget(parent)
{
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
    m_dropdownOpen = false;
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
    m_dropdownOpen = true;
    m_dropdownHighlight = qMax(0, m_currentIndex);
}

void QTuiComboBox::closeDropdown()
{
    m_dropdownOpen = false;
}

QSize QTuiComboBox::minimumSize() const
{
    // Find widest item text
    int maxTextWidth = 5; // minimum reasonable width
    for (const auto &item : m_items) {
        maxTextWidth = qMax(maxTextWidth, static_cast<int>(item.length()));
    }
    // Content: text + " ▼" indicator + borders + padding
    int width = maxTextWidth + 4 + 2; // 2 for " ▼", 2 for borders, 2 for padding
    int height = 3; // top border + content row + bottom border
    if (m_dropdownOpen && !m_items.isEmpty()) {
        height += m_items.size(); // dropdown rows
    }
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

    // Determine colors based on focus state
    Color fg = theme.foreground;
    Color bg = theme.background;

    if (hasFocus()) {
        fg = theme.focusForeground;
        bg = theme.focusBackground;
    }

    // Clear the entire widget area with background
    int totalH = m_dropdownOpen ? geoH : qMin(geoH, 3);
    for (int row = 0; row < totalH; ++row) {
        for (int col = 0; col < geoW; ++col) {
            int px = geoX + col;
            int py = geoY + row;
            if (px < buffer.width() && py < buffer.height()) {
                Cell &cell = buffer.backCell(px, py);
                cell.character = QLatin1Char(' ');
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
                    cell.character = border.topLeft;
                else if (col == geoW - 1)
                    cell.character = border.topRight;
                else
                    cell.character = border.horizontal;
                cell.foreground = fg;
                cell.background = bg;
            }
        }
    }

    // Render bottom border of the main combo area (row 2)
    if (geoH >= 3) {
        int py = geoY + 2;
        // If dropdown is open, this becomes a separator, not bottom border
        if (!m_dropdownOpen) {
            for (int col = 0; col < geoW; ++col) {
                int px = geoX + col;
                if (px < buffer.width() && py < buffer.height()) {
                    Cell &cell = buffer.backCell(px, py);
                    if (col == 0)
                        cell.character = border.bottomLeft;
                    else if (col == geoW - 1)
                        cell.character = border.bottomRight;
                    else
                        cell.character = border.horizontal;
                    cell.foreground = fg;
                    cell.background = bg;
                }
            }
        } else {
            // Separator between combo display and dropdown
            for (int col = 0; col < geoW; ++col) {
                int px = geoX + col;
                if (px < buffer.width() && py < buffer.height()) {
                    Cell &cell = buffer.backCell(px, py);
                    if (col == 0)
                        cell.character = border.vertical;
                    else if (col == geoW - 1)
                        cell.character = border.vertical;
                    else
                        cell.character = border.horizontal;
                    cell.foreground = fg;
                    cell.background = bg;
                }
            }
        }
    }

    // Render left/right borders for the content row (row 1)
    if (geoH >= 2) {
        int py = geoY + 1;
        if (py < buffer.height()) {
            int pxLeft = geoX;
            if (pxLeft < buffer.width()) {
                buffer.backCell(pxLeft, py).character = border.vertical;
                buffer.backCell(pxLeft, py).foreground = fg;
                buffer.backCell(pxLeft, py).background = bg;
            }
            int pxRight = geoX + geoW - 1;
            if (pxRight < buffer.width()) {
                buffer.backCell(pxRight, py).character = border.vertical;
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
                cell.character = truncated.at(i);
                cell.foreground = fg;
                cell.background = bg;
                cell.bold = theme.bold || hasFocus();
            }
        }

        // Write dropdown indicator at the right end
        QChar indicator = m_dropdownOpen ? QChar(0x25B2) : QChar(0x25BC); // ▲ or ▼
        int indicatorPos = geoX + geoW - 2;
        if (indicatorPos > geoX && indicatorPos < buffer.width() && contentRow < buffer.height()) {
            Cell &cell = buffer.backCell(indicatorPos, contentRow);
            cell.character = indicator;
            cell.foreground = fg;
            cell.background = bg;
        }
    }

    // Render dropdown items if open
    if (m_dropdownOpen && geoH >= 3 && geoW >= 3) {
        int innerWidth = geoW - 2;

        for (int i = 0; i < m_items.size(); ++i) {
            int rowY = geoY + 3 + i;
            if (rowY >= buffer.height())
                break;

            // Left/right borders for dropdown rows
            if (geoX < buffer.width()) {
                buffer.backCell(geoX, rowY).character = border.vertical;
                buffer.backCell(geoX, rowY).foreground = fg;
                buffer.backCell(geoX, rowY).background = bg;
            }
            int pxRight = geoX + geoW - 1;
            if (pxRight < buffer.width()) {
                buffer.backCell(pxRight, rowY).character = border.vertical;
                buffer.backCell(pxRight, rowY).foreground = fg;
                buffer.backCell(pxRight, rowY).background = bg;
            }

            // Highlight the currently navigated item
            Color itemFg = fg;
            Color itemBg = bg;
            bool isHighlighted = (i == m_dropdownHighlight);
            if (isHighlighted) {
                // Swap fg/bg for highlight
                itemFg = bg;
                itemBg = fg;
            }

            // Clear and write item text
            QString itemStr = m_items.at(i).left(innerWidth);
            for (int col = 0; col < innerWidth; ++col) {
                int px = geoX + 1 + col;
                if (px < buffer.width() && rowY < buffer.height()) {
                    Cell &cell = buffer.backCell(px, rowY);
                    cell.character = col < itemStr.length() ? itemStr.at(col) : QLatin1Char(' ');
                    cell.foreground = itemFg;
                    cell.background = itemBg;
                    cell.bold = isHighlighted;
                }
            }
        }

        // Bottom border of dropdown
        int bottomRow = geoY + 3 + m_items.size();
        if (bottomRow < buffer.height()) {
            for (int col = 0; col < geoW; ++col) {
                int px = geoX + col;
                if (px < buffer.width()) {
                    Cell &cell = buffer.backCell(px, bottomRow);
                    if (col == 0)
                        cell.character = border.bottomLeft;
                    else if (col == geoW - 1)
                        cell.character = border.bottomRight;
                    else
                        cell.character = border.horizontal;
                    cell.foreground = fg;
                    cell.background = bg;
                }
            }
        }
    }
}

void QTuiComboBox::keyPressEvent(TuiKeyEvent *event)
{
    const int key = event->key();

    if (m_dropdownOpen) {
        // Dropdown is open: navigate or select
        if (key == Qt::Key_Up) {
            if (m_dropdownHighlight > 0)
                m_dropdownHighlight--;
        } else if (key == Qt::Key_Down) {
            if (m_dropdownHighlight < m_items.size() - 1)
                m_dropdownHighlight++;
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
            }
        } else if (m_editable && key == Qt::Key_Backspace) {
            if (!m_editText.isEmpty()) {
                m_editText.chop(1);
                emit currentTextChanged(m_editText);
            }
        }
    }
}

void QTuiComboBox::mousePressEvent(TuiMouseEvent *event)
{
    if (event->mouseType() != TuiMouseEvent::Press)
        return;

    const QRect geo = geometry();

    if (!m_dropdownOpen) {
        openDropdown();
    } else {
        // Dropdown is open - check if click is in the dropdown area
        // Dropdown items start at geo.y() + 3
        int itemIdx = event->y() - geo.y() - 3;
        if (itemIdx >= 0 && itemIdx < m_items.size()) {
            setCurrentIndex(itemIdx);
            closeDropdown();
        } else {
            closeDropdown();
        }
    }
    markDirty();
}

} // namespace QtTUI
