#include <QtTUI/QTuiComboBox.h>
#include <QtTUI/QtTuiUnicode.h>
#include <QtTUI/RenderUtils.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>
#include <QStringListModel>

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
    const bool focused = hasFocus() || m_combo->hasFocus();
    Color fg = focused ? theme.focusForeground : theme.foreground;
    Color bg = focused ? theme.focusBackground : theme.background;

    const int gx = geo.x();
    const int gy = geo.y();
    const int gw = geo.width();
    const int gh = geo.height();

    fillRect(buffer, geo, cellStyle(fg, bg));
    drawBorder(buffer, geo, theme.borderStyle, cellStyle(fg, bg));

    // Top row: separator joining the combo body above (replaces its bottom border)
    for (int col = 0; col < gw; ++col)
        buffer.putChar(gx + col, gy, (col == 0 || col == gw - 1) ? border.vertical : border.horizontal,
                       cellStyle(fg, bg));

    // Item rows between the separator and the bottom border
    const int innerWidth = gw - 2;
    for (int i = 0; i < m_combo->count() && 1 + i < gh - 1; ++i) {
        const bool highlighted = (i == m_combo->m_dropdownHighlight);
        Cell itemStyle = highlighted ? cellStyle(bg, fg) : cellStyle(fg, bg);
        itemStyle.bold = highlighted;
        fillRect(buffer, QRect(gx + 1, gy + 1 + i, innerWidth, 1), itemStyle);
        renderText(buffer, gx + 1, gy + 1 + i, m_combo->itemText(i), innerWidth, itemStyle);
    }
}

void QTuiComboBoxPopup::mousePressEvent(TuiMouseEvent *event)
{
    if (!isVisible() || event->mouseType() != TuiMouseEvent::Press)
        return;

    // Items are rendered one per row starting at geometry().y() + 1 (row 0 is
    // the separator joining the combo body). Clicks on border rows are ignored.
    int itemIdx = event->y() - geometry().y() - 1;
    if (itemIdx >= 0 && itemIdx < m_combo->count()) {
        m_combo->setCurrentIndex(itemIdx);
        m_combo->closeDropdown();
    }
}

QTuiComboBox::QTuiComboBox(QTuiWidget *parent)
    : QTuiWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    // The items are in a model, as in QComboBox. setModel() replaces it.
    setModel(new QStringListModel(this));
    // Hiding the combo must not leave its dropdown popup floating on screen
    connect(this, &QTuiWidget::visibilityChanged, this, [this](bool visible) {
        if (!visible)
            closeDropdown();
    });
}

void QTuiComboBox::addItem(const QString &text)
{
    insertItem(count(), text);
}

void QTuiComboBox::addItems(const QStringList &texts)
{
    for (const auto &t : texts)
        addItem(t);
}

void QTuiComboBox::insertItem(int index, const QString &text)
{
    index = qBound(0, index, count());
    // Select the first item after it has its text
    m_inserting = true;
    if (m_model->insertRow(index))
        m_model->setData(m_model->index(index, 0), text);
    m_inserting = false;
    if (m_currentIndex < 0)
        setCurrentIndex(0);
}

void QTuiComboBox::removeItem(int index)
{
    if (index >= 0 && index < count())
        m_model->removeRow(index);
}

void QTuiComboBox::clear()
{
    closeDropdown();
    m_editText.clear();
    m_model->removeRows(0, count());
    m_dropdownHighlight = 0;
}

int QTuiComboBox::count() const
{
    return m_model->rowCount();
}

QString QTuiComboBox::itemText(int index) const
{
    return m_model->data(m_model->index(index, 0), Qt::DisplayRole).toString();
}

int QTuiComboBox::currentIndex() const
{
    return m_currentIndex;
}

void QTuiComboBox::setCurrentIndex(int index)
{
    if (index < -1)
        index = -1;
    if (index >= count())
        index = count() - 1;

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
    return itemText(m_currentIndex);
}

bool QTuiComboBox::isEditable() const
{
    return m_editable;
}

void QTuiComboBox::setEditable(bool editable)
{
    if (m_editable == editable)
        return;
    m_editable = editable;
    if (editable)
        m_editText = itemText(m_currentIndex);
    markDirty();
}

void QTuiComboBox::setModel(QAbstractItemModel *model)
{
    if (!model || model == m_model)
        return;
    if (m_model) {
        disconnect(m_model, nullptr, this, nullptr);
        // Delete the default model
        if (m_model->parent() == this)
            delete m_model;
    }
    m_model = model;
    connect(m_model, &QAbstractItemModel::dataChanged, this, [this] { markDirty(); });
    connect(m_model, &QAbstractItemModel::rowsInserted, this, &QTuiComboBox::onRowsInserted);
    connect(m_model, &QAbstractItemModel::rowsRemoved, this, &QTuiComboBox::onRowsRemoved);
    connect(m_model, &QAbstractItemModel::modelReset, this, &QTuiComboBox::onModelReset);
    // A deleted model: use an empty default model
    connect(m_model, &QObject::destroyed, this, [this] {
        m_model = nullptr;
        setModel(new QStringListModel(this));
    });
    onModelReset();
}

QAbstractItemModel *QTuiComboBox::model() const
{
    return m_model;
}

void QTuiComboBox::onRowsInserted(const QModelIndex &parent, int first, int last)
{
    if (parent.isValid())
        return;
    if (m_currentIndex < 0) {
        if (!m_inserting)
            setCurrentIndex(0);
    } else if (first <= m_currentIndex) {
        // The current item moves down
        m_currentIndex += last - first + 1;
        emit currentIndexChanged(m_currentIndex);
    }
    markDirty();
}

void QTuiComboBox::onRowsRemoved(const QModelIndex &parent, int first, int last)
{
    if (parent.isValid())
        return;
    if (m_currentIndex > last) {
        // The current item moves up
        m_currentIndex -= last - first + 1;
        emit currentIndexChanged(m_currentIndex);
    } else if (m_currentIndex >= first) {
        // The current item is removed: the next item becomes current
        m_currentIndex = -1;
        setCurrentIndex(qMin(first, count() - 1));
        if (m_currentIndex < 0) {
            emit currentIndexChanged(-1);
            emit currentTextChanged(QString());
        }
    }
    markDirty();
}

void QTuiComboBox::onModelReset()
{
    if (m_currentIndex >= count())
        setCurrentIndex(count() - 1);
    else if (m_currentIndex < 0 && count() > 0)
        setCurrentIndex(0);
    markDirty();
}

void QTuiComboBox::openDropdown()
{
    if (count() == 0)
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
    m_popup->setGeometry(QRect(geo.x(), geo.y() + 2, geo.width(), count() + 2));
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
    for (int i = 0; i < count(); ++i)
        maxTextWidth = qMax(maxTextWidth, QtTuiUnicode::stringWidth(itemText(i)));
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

    // Determine colors based on focus state. While the dropdown popup is open
    // it holds the focus, so treat the combo body as focused too.
    const bool focused = hasFocus() || m_dropdownOpen;
    Color fg = focused ? theme.focusForeground : theme.foreground;
    Color bg = focused ? theme.focusBackground : theme.background;

    // The combo body has 3 rows: border, content, border. When the dropdown
    // popup is open, its top separator row paints over the bottom border in
    // the topmost pass.
    const QRect body(geo.x(), geo.y(), geo.width(), qMin(geo.height(), 3));
    fillRect(buffer, body, cellStyle(theme, fg, bg));
    drawBorder(buffer, body, theme.borderStyle, cellStyle(theme, fg, bg));

    // Render current item text + dropdown indicator in the content row
    if (geo.height() >= 3 && geo.width() >= 3) {
        const int contentRow = geo.y() + 1;
        const QString displayText = m_editable ? m_editText : currentText();
        Cell textStyle = cellStyle(theme, fg, bg);
        textStyle.bold = theme.bold || focused;
        // Reserve 2 columns for " ▼" or " ▲"
        renderText(buffer, geo.x() + 1, contentRow, displayText, geo.width() - 4, textStyle);
        renderText(buffer, geo.x() + geo.width() - 2, contentRow,
                   m_dropdownOpen ? QStringLiteral("▲") : QStringLiteral("▼"), 1,
                   cellStyle(theme, fg, bg));
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
            if (m_dropdownHighlight < count() - 1) {
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
        // In an editable combo box, Space types a space
        if (key == Qt::Key_Return || key == Qt::Key_Enter || (key == Qt::Key_Space && !m_editable)) {
            openDropdown();
        } else if (key == Qt::Key_Up) {
            // Quick navigate without opening dropdown
            if (m_currentIndex > 0)
                setCurrentIndex(m_currentIndex - 1);
        } else if (key == Qt::Key_Down) {
            if (m_currentIndex < count() - 1)
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
