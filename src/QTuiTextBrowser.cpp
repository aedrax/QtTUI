#include <QtTUI/QTuiTextBrowser.h>
#include <QtTUI/QTuiLabel.h>
#include <QtTUI/QTuiScrollBar.h>
#include <QtTUI/TuiEvents.h>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QUrl>

namespace QtTUI {

QTuiTextBrowser::QTuiTextBrowser(QTuiWidget *parent)
    : QTuiScrollArea(parent)
    , m_label(new QTuiLabel(QString(), this))
{
    m_label->setWordWrap(true);
    m_label->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    setWidget(m_label);
    // A left click on a link comes from the label, Enter comes through it
    connect(m_label, &QTuiLabel::linkActivated, this, &QTuiTextBrowser::activate);
    // The text wraps at the new width
    connect(this, &QTuiWidget::geometryChanged, this, &QTuiTextBrowser::relayout);
}

QString QTuiTextBrowser::toPlainText() const
{
    return m_label->displayText();
}

QString QTuiTextBrowser::source() const
{
    return m_history.value(m_historyPos);
}

bool QTuiTextBrowser::openLinks() const
{
    return m_openLinks;
}

void QTuiTextBrowser::setOpenLinks(bool open)
{
    m_openLinks = open;
}

bool QTuiTextBrowser::isBackwardAvailable() const
{
    return m_backwardAvailable;
}

bool QTuiTextBrowser::isForwardAvailable() const
{
    return m_forwardAvailable;
}

void QTuiTextBrowser::setMarkdown(const QString &markdown)
{
    showText(Qt::MarkdownText, markdown);
}

void QTuiTextBrowser::setHtml(const QString &html)
{
    showText(Qt::RichText, html);
}

void QTuiTextBrowser::setPlainText(const QString &text)
{
    showText(Qt::PlainText, text);
}

void QTuiTextBrowser::showText(Qt::TextFormat format, const QString &text)
{
    // Empty the label first, so that it does not read the old text in the
    // new format
    m_label->setText(QString());
    m_label->setTextFormat(format);
    m_label->setText(text);
    // The label takes the focus when it has links. The browser keeps the
    // focus and the keys, and gives the link keys to the label.
    m_label->setFocusPolicy(Qt::NoFocus);
    // ponytail: new text always starts at the top; the history does not
    // keep the scroll position of each file.
    // Through the scroll bars: a new range must not bring back the old
    // position
    verticalScrollBar()->setValue(0);
    horizontalScrollBar()->setValue(0);
    m_scrollOffset = 0;
    m_scrollX = 0;
    relayout();
}

void QTuiTextBrowser::relayout()
{
    const QSize size = geometry().size();
    // All the width; if the text is then too high, one column is for the
    // scroll bar
    int width = size.width();
    // A text with more line breaks than rows is too high at each width;
    // then the wrap at the full width is not necessary
    const bool lines = m_label->displayText().count(QLatin1Char('\n')) >= size.height();
    int height = lines ? size.height() + 1 : m_label->heightForWidth(width);
    if (height > size.height() && width > 1)
        height = m_label->heightForWidth(--width);
    m_label->setGeometry(QRect(0, 0, width, height));
    updateScrollBars();
    markDirty();
}

void QTuiTextBrowser::setSource(const QString &path)
{
    if (path != source()) {
        // A new file removes the history after the current file
        m_history.resize(m_historyPos + 1);
        m_history.append(path);
        ++m_historyPos;
    }
    load(path);
}

void QTuiTextBrowser::backward()
{
    if (m_historyPos > 0)
        load(m_history.at(--m_historyPos));
}

void QTuiTextBrowser::forward()
{
    if (m_historyPos + 1 < m_history.size())
        load(m_history.at(++m_historyPos));
}

void QTuiTextBrowser::home()
{
    // A copy: setSource() changes the list
    if (!m_history.isEmpty())
        setSource(QString(m_history.constFirst()));
}

void QTuiTextBrowser::load(const QString &path)
{
    // Only a regular file, so a link cannot make the browser read a
    // device or a pipe that never ends.
    // ponytail: at most 16 MiB of a file show. Read and parse in parts if
    // larger files are necessary.
    const qint64 maxBytes = 16 * 1024 * 1024;
    const QFileInfo info(path);
    QFile file(path);
    QString text;
    if (info.isFile() && file.open(QIODevice::ReadOnly))
        text = QString::fromUtf8(file.read(maxBytes));
    else
        qWarning() << "QTuiTextBrowser: cannot read" << path;

    const QString suffix = info.suffix().toLower();
    Qt::TextFormat format = Qt::PlainText;
    if (suffix == QLatin1String("md") || suffix == QLatin1String("markdown"))
        format = Qt::MarkdownText;
    else if (suffix == QLatin1String("html") || suffix == QLatin1String("htm"))
        format = Qt::RichText;
    showText(format, text);
    emit sourceChanged(path);
    updateAvailable();
}

void QTuiTextBrowser::updateAvailable()
{
    const bool back = m_historyPos > 0;
    const bool next = m_historyPos + 1 < m_history.size();
    if (back != m_backwardAvailable)
        emit backwardAvailable(m_backwardAvailable = back);
    if (next != m_forwardAvailable)
        emit forwardAvailable(m_forwardAvailable = next);
}

void QTuiTextBrowser::activate(const QString &link)
{
    // A copy: the link is in the label, and a slot can change the text
    const QString target = link;
    emit anchorClicked(target);
    if (!m_openLinks)
        return;

    const QString scheme = QTuiLabel::linkScheme(target);
    QString path;
    if (scheme == QLatin1String("file")) {
        path = QUrl(target).toLocalFile();
    } else if (scheme.isEmpty()) {
        // ponytail: the label has no anchors, so the browser removes
        // "#fragment" and does not scroll to it. A link that is only
        // "#fragment" does nothing.
        path = target.section(QLatin1Char('#'), 0, 0);
    }
    // All other schemes are for the application: no network, no program
    if (path.isEmpty())
        return;
    // A relative path starts at the directory of the file that shows
    if (!source().isEmpty())
        path = QFileInfo(source()).dir().filePath(path);
    setSource(QDir::cleanPath(path));
}

bool QTuiTextBrowser::moveLink(bool next)
{
    // The label knows the links: give it the key, and look for a change.
    // ponytail: the current link does not follow the scroll position, so
    // after a long scroll the next link can be far away.
    const int before = m_label->currentLink();
    TuiKeyEvent key(next ? Qt::Key_Right : Qt::Key_Left, Qt::NoModifier);
    QCoreApplication::sendEvent(m_label, &key);
    if (m_label->currentLink() == before)
        return false;
    const QRect link = m_label->linkRect(m_label->currentLink());
    if (!link.isEmpty()) {
        ensureVisible(link.right(), link.bottom());
        ensureVisible(link.left(), link.top());
    }
    markDirty();
    return true;
}

bool QTuiTextBrowser::focusNextPrevChild(bool next)
{
    return moveLink(next) || QTuiScrollArea::focusNextPrevChild(next);
}

void QTuiTextBrowser::keyPressEvent(TuiKeyEvent *event)
{
    const int key = event->key();
    const bool alt = event->modifiers() & Qt::AltModifier;
    const bool ctrl = event->modifiers() == Qt::ControlModifier;
    // The scroll keys of vim, as in the item views: g, G, Ctrl+F, Ctrl+B.
    // The scroll area knows j and k.
    int scrollKey = 0;
    if (event->text() == QLatin1String("g"))
        scrollKey = Qt::Key_Home;
    else if (event->text() == QLatin1String("G"))
        scrollKey = Qt::Key_End;
    else if (ctrl && key == Qt::Key_F)
        scrollKey = Qt::Key_PageDown;
    else if (ctrl && key == Qt::Key_B)
        scrollKey = Qt::Key_PageUp;

    if (alt && key == Qt::Key_Left) {
        backward();
    } else if (alt && key == Qt::Key_Right) {
        forward();
    } else if (key == Qt::Key_Backspace) {
        backward();
    } else if (key == Qt::Key_Left || key == Qt::Key_Backtab
               || (key == Qt::Key_Tab && (event->modifiers() & Qt::ShiftModifier))) {
        moveLink(false);
    } else if (key == Qt::Key_Right || key == Qt::Key_Tab) {
        moveLink(true);
    } else if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        // The label emits linkActivated() for its current link
        QCoreApplication::sendEvent(m_label, event);
    } else if (scrollKey) {
        TuiKeyEvent scroll(scrollKey, Qt::NoModifier);
        QTuiScrollArea::keyPressEvent(&scroll);
    } else {
        QTuiScrollArea::keyPressEvent(event);
    }
}

void QTuiTextBrowser::paintContent(ScreenBuffer &buffer, const QRect &viewport)
{
    // Only the rows that show: the label does not paint all a long text
    m_label->render(buffer, viewport.topLeft() - QPoint(m_scrollX, m_scrollOffset), viewport,
                    hasFocus());
}

} // namespace QtTUI
