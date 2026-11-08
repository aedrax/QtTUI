#ifndef QTTUI_QTUILABEL_H
#define QTTUI_QTUILABEL_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiFrame.h>
#include <QtTUI/QtTuiUnicode.h>
#include <QString>
#include <QStringList>
#include <QVector>
#include <Qt>
#include <functional>

namespace QtTUI {

class QTuiLabel : public QTuiFrame {
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(Qt::Alignment alignment READ alignment WRITE setAlignment)
    Q_PROPERTY(bool wordWrap READ wordWrap WRITE setWordWrap)
    Q_PROPERTY(Qt::TextFormat textFormat READ textFormat WRITE setTextFormat)
public:
    explicit QTuiLabel(const QString &text = QString(), QTuiWidget *parent = nullptr);

    QString text() const;
    void setText(const QString &text);

    Qt::Alignment alignment() const;
    void setAlignment(Qt::Alignment alignment);

    bool wordWrap() const;
    void setWordWrap(bool on);

    // How the text is read, as in QLabel. Qt::PlainText (the default)
    // shows the text as it is. Qt::MarkdownText reads **bold**, *italic*,
    // `code` and ~~strikethrough~~ (__ and _ also work; \ escapes).
    // Qt::RichText reads the HTML tags <b> <strong> <i> <em> <u> <s>
    // <del> <code> <br> <p>, and &lt; &gt; &amp; &quot; &#39; &nbsp;. Other
    // tags are removed. Code shows in reverse video. Qt::AutoText is
    // plain text (Qt's detection is in Qt GUI).
    //
    // Links: [text](url) in Markdown and <a href="url">text</a> in rich
    // text. A link shows underlined in the accent color. A label with
    // links takes the focus: Left, Right, Up, Down and Tab move between
    // the links, and Enter or a left click emits linkActivated().
    Qt::TextFormat textFormat() const;
    void setTextFormat(Qt::TextFormat format);
    // The text as it shows, without markup
    QString displayText() const;

    // The buddy widget, as in QLabel. With a buddy, an & in the text marks
    // the next character as the mnemonic: the label does not show the &,
    // it underlines the character, and Alt+character gives the focus to
    // the buddy (QTuiMainWindow does this). Only the first mnemonic
    // counts. && shows one &. Without a buddy, an & is normal text. In
    // Qt::RichText an & starts an entity, so there is no mnemonic.
    QTuiWidget *buddy() const;
    void setBuddy(QTuiWidget *buddy);
    // The mnemonic character, or a null QChar if there is none
    QChar mnemonic() const;

    QSize minimumSize() const override;
    void paintEvent(ScreenBuffer &buffer) override;

signals:
    void textChanged(const QString &text);
    // The user activated a link; link is its target (the url)
    void linkActivated(const QString &link);

protected:
    void keyPressEvent(TuiKeyEvent *event) override;
    void mousePressEvent(TuiMouseEvent *event) override;

private:
    QStringList wrapText(const QString &text, int width) const;

    QString m_text;
    Qt::Alignment m_alignment = Qt::AlignLeft | Qt::AlignVCenter;
    bool m_wordWrap = false;
    Qt::TextFormat m_textFormat = Qt::PlainText;

    // Style per UTF-16 unit of displayText(): the low byte has the style
    // bits, the high byte has the number of the link (from 1; 0: no link)
    enum StyleBit : quint8 { Bold = 1, Italic = 2, Underline = 4, Strike = 8, Code = 16 };
    // Returns the mnemonic character
    QChar parse(QString *shown, QVector<quint16> *styles, QStringList *links = nullptr) const;
    // Calls visit for each character that shows, with its screen cell and
    // its style. The paint and the mouse use the same positions.
    void layoutText(const std::function<void(int x, int y, const QtTuiUnicode::Cluster &c,
                                             quint16 style)> &visit) const;
    // Reads the links again after a change of the text or the format
    void updateLinks();

    QStringList m_links;   // the target of each link in the text
    int m_currentLink = 0; // the link that Enter activates
    QPointer<QTuiWidget> m_buddy;
    QMetaObject::Connection m_buddyDestroyed;
};

} // namespace QtTUI

#endif // QTTUI_QTUILABEL_H
