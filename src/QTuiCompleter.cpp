#include <QtTUI/QTuiCompleter.h>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiLineEdit.h>
#include <QtTUI/QTuiMainWindow.h>
#include <QtTUI/QtTuiUnicode.h>
#include <QtTUI/RenderUtils.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/TuiEvents.h>
#include <QAbstractItemModel>
#include <QStringListModel>

namespace QtTUI {

// The list of matches. It is a popup, so it floats above other widgets,
// and it has no focus policy, so the line edit keeps focus.
class CompleterPopup : public QTuiWidget {
public:
    CompleterPopup(QTuiCompleter *completer, QTuiWidget *parent)
        : QTuiWidget(parent)
        , m_completer(completer)
    {
        setPopup(true);
        setVisible(false);
    }

    int firstRow = 0; // first match shown (scroll position)

    void paintEvent(ScreenBuffer &buffer) override
    {
        const QRect geo = geometry();
        if (!isVisible() || geo.isEmpty())
            return;
        clearBackground(buffer, geo);
        const auto theme = themeProperties();
        const QStringList matches = m_completer->completions();
        for (int i = 0; i < geo.height(); ++i) {
            const int row = firstRow + i;
            if (row >= matches.size())
                break;
            const bool current = row == m_completer->currentRow();
            Cell style;
            // The popup uses the inverted surface colors, and the current
            // row uses the focus colors
            style.foreground = current ? theme.focusForeground : theme.background;
            style.background = current ? theme.focusBackground : theme.foreground;
            fillRect(buffer, QRect(geo.x(), geo.y() + i, geo.width(), 1), style);
            renderTextTruncated(buffer, geo.x() + 1, geo.y() + i,
                                matches.at(row), geo.width() - 1, style);
        }
    }

    void popupOutsideClick() override { m_completer->hidePopup(); }

protected:
    void mousePressEvent(TuiMouseEvent *event) override
    {
        if (event->mouseType() == TuiMouseEvent::Press)
            m_completer->activateRow(firstRow + event->y() - geometry().y());
    }

private:
    QTuiCompleter *m_completer;
};

QTuiCompleter::QTuiCompleter(const QStringList &list, QObject *parent)
    : QObject(parent)
{
    setModel(new QStringListModel(list, this));
}

QTuiCompleter::QTuiCompleter(QAbstractItemModel *model, QObject *parent)
    : QObject(parent)
{
    setModel(model);
}

QTuiCompleter::~QTuiCompleter()
{
    delete m_popup;
}

QAbstractItemModel *QTuiCompleter::model() const
{
    return m_model;
}

void QTuiCompleter::setModel(QAbstractItemModel *model)
{
    m_model = model;
    m_currentRow = -1;
}

Qt::CaseSensitivity QTuiCompleter::caseSensitivity() const
{
    return m_caseSensitivity;
}

void QTuiCompleter::setCaseSensitivity(Qt::CaseSensitivity sensitivity)
{
    m_caseSensitivity = sensitivity;
}

QTuiCompleter::FilterMode QTuiCompleter::filterMode() const
{
    return m_filterMode;
}

void QTuiCompleter::setFilterMode(FilterMode mode)
{
    m_filterMode = mode;
}

int QTuiCompleter::maxVisibleItems() const
{
    return m_maxVisibleItems;
}

void QTuiCompleter::setMaxVisibleItems(int count)
{
    m_maxVisibleItems = qMax(1, count);
}

QString QTuiCompleter::completionPrefix() const
{
    return m_prefix;
}

void QTuiCompleter::setCompletionPrefix(const QString &prefix)
{
    m_prefix = prefix;
    m_currentRow = completionCount() > 0 ? 0 : -1;
}

QStringList QTuiCompleter::completions() const
{
    QStringList result;
    if (!m_model)
        return result;
    for (int row = 0; row < m_model->rowCount(); ++row) {
        const QString text = m_model->index(row, 0).data(Qt::DisplayRole).toString();
        const bool match = m_filterMode == StartsWith
            ? text.startsWith(m_prefix, m_caseSensitivity)
            : text.contains(m_prefix, m_caseSensitivity);
        if (match)
            result.append(text);
    }
    return result;
}

int QTuiCompleter::completionCount() const
{
    return completions().size();
}

int QTuiCompleter::currentRow() const
{
    return m_currentRow;
}

void QTuiCompleter::setCurrentRow(int row)
{
    const QStringList matches = completions();
    if (row < 0 || row >= matches.size() || row == m_currentRow)
        return;
    m_currentRow = row;

    if (m_popup) {
        // Scroll so that the current row is visible
        const int rows = m_popup->geometry().height();
        if (m_currentRow < m_popup->firstRow)
            m_popup->firstRow = m_currentRow;
        else if (rows > 0 && m_currentRow >= m_popup->firstRow + rows)
            m_popup->firstRow = m_currentRow - rows + 1;
        m_popup->markDirty();
    }
    emit highlighted(matches.at(m_currentRow));
}

QString QTuiCompleter::currentCompletion() const
{
    return completions().value(m_currentRow);
}

bool QTuiCompleter::isPopupVisible() const
{
    return m_popup && m_popup->isVisible();
}

void QTuiCompleter::updateForText(QTuiLineEdit *lineEdit, const QString &text)
{
    m_lineEdit = lineEdit;
    setCompletionPrefix(text);
    const QStringList matches = completions();

    // Nothing to offer: no text, no match, or only the text itself
    if (text.isEmpty() || matches.isEmpty()
        || (matches.size() == 1 && matches.first() == text)) {
        hidePopup();
        return;
    }

    if (!m_popup)
        m_popup = new CompleterPopup(this, lineEdit);
    else if (m_popup->parent() != lineEdit)
        m_popup->setParent(lineEdit);

    // Below the line edit, or above it if there is no space below
    const QRect le = lineEdit->geometry();
    const int rows = qMin(int(matches.size()), m_maxVisibleItems);
    QRect screen(0, 0, 0x7fff, 0x7fff);
    if (auto *app = QTuiApplication::instance(); app && app->mainWindow())
        screen = app->mainWindow()->geometry();
    int y = le.bottom() + 1;
    if (y + rows - 1 > screen.bottom() && le.y() - rows >= screen.y())
        y = le.y() - rows;

    m_popup->firstRow = 0;
    m_popup->setGeometry(QRect(le.x(), y, le.width(), rows));
    m_popup->setVisible(true);
    m_popup->markDirty();
}

bool QTuiCompleter::handleKey(int key)
{
    if (!isPopupVisible())
        return false;
    switch (key) {
    case Qt::Key_Down:
        setCurrentRow(qMin(m_currentRow + 1, completionCount() - 1));
        return true;
    case Qt::Key_Up:
        setCurrentRow(qMax(m_currentRow - 1, 0));
        return true;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        activateRow(m_currentRow);
        return true;
    case Qt::Key_Escape:
        hidePopup();
        return true;
    default:
        return false;
    }
}

void QTuiCompleter::hidePopup()
{
    if (m_popup && m_popup->isVisible())
        m_popup->setVisible(false);
}

void QTuiCompleter::activateRow(int row)
{
    const QString text = completions().value(row);
    hidePopup();
    if (text.isEmpty())
        return;
    if (m_lineEdit)
        m_lineEdit->setText(text);
    emit activated(text);
}

} // namespace QtTUI
