#ifndef QTTUI_SCREENBUFFER_H
#define QTTUI_SCREENBUFFER_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QtTuiUnicode.h>
#include <QHash>
#include <QMetaType>
#include <QPoint>
#include <QString>
#include <QStringList>
#include <QVector>

#include <algorithm>
#include <cstdint>

namespace QtTUI {

struct Color {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;

    constexpr Color() = default;
    constexpr Color(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b) {}
    // From 0xRRGGBB
    static constexpr Color fromRgb(uint32_t rgb)
    {
        return Color(uint8_t(rgb >> 16), uint8_t(rgb >> 8), uint8_t(rgb));
    }

    bool operator==(const Color &other) const
    {
        return r == other.r && g == other.g && b == other.b;
    }
    bool operator!=(const Color &other) const { return !(*this == other); }
};

/**
 * One terminal cell.
 *
 * Unicode model (notcurses/tmux style): a cell stores one base code point
 * (`ch`), the number of terminal columns it occupies (`width`), and an
 * optional string of the code points that join the base (`combining`, null
 * for the overwhelmingly common case). These are combining marks and the
 * other parts of an emoji sequence (see QtTuiUnicode::Cluster).
 *
 * Wide glyphs (width == 2) occupy their own cell plus a CONTINUATION cell
 * immediately to the right: ch == 0, width == 0 (see isContinuation()).
 * Diffing and rendering treat the pair atomically.
 *
 * Use setChar() rather than assigning `ch` directly: it keeps `width` in
 * sync and clears stale combining marks. Wide characters written into the
 * buffer should go through ScreenBuffer::putChar(), which also maintains
 * the continuation cell and repairs half-overwritten wide pairs.
 */
struct Cell {
    char32_t ch = U' ';
    quint8 width = 1;    ///< 0 = continuation of a wide glyph, 1 = narrow, 2 = wide
    QString combining;   ///< code points that join ch (rare; null by default)
    Color foreground = Color(255, 255, 255);
    Color background = Color(0, 0, 0);
    bool bold = false;
    bool italic = false;
    bool underline = false;
    bool strikethrough = false;
    bool inverse = false;
    /**
     * The hyperlink of the cell: an id from ScreenBuffer::linkId(), or 0
     * for no link. The id is valid only in the buffer that gave it.
     */
    quint16 link = 0;

    /** True for the placeholder cell to the right of a wide glyph. */
    bool isContinuation() const { return width == 0; }

    /**
     * The cell's textual content (base code point plus combining marks) as a
     * QString; a continuation cell yields a single space. Read-side
     * convenience for tests and text extraction.
     */
    QString text() const
    {
        if (ch == 0)
            return QStringLiteral(" ");
        const char32_t c = ch;
        QString s = QString::fromUcs4(&c, 1);
        if (!combining.isEmpty())
            s += combining;
        return s;
    }

    /**
     * Sets the cell's character, keeping `width` consistent and clearing any
     * stale combining marks. Note: this cannot maintain the neighboring
     * continuation cell for wide characters — text that may contain wide
     * glyphs must be written via ScreenBuffer::putChar().
     */
    void setChar(char32_t c)
    {
        ch = c;
        width = (c < 0x80) ? quint8(1)
                           : quint8(std::max(1, QtTuiUnicode::charWidth(c)));
        if (!combining.isNull())
            combining.clear();
    }

    bool operator==(const Cell &other) const;
    bool operator!=(const Cell &other) const;
};

class ScreenBuffer {
public:
    explicit ScreenBuffer(int width = 1, int height = 1);

    void resize(int width, int height);
    int width() const;
    int height() const;

    Cell &backCell(int x, int y);
    const Cell &backCell(int x, int y) const;
    const Cell &frontCell(int x, int y) const;

    /**
     * Writes one glyph (base code point + optional combining marks) into the
     * back buffer with the colors/attributes of `style`.
     *
     * This is the sanctioned path for characters that may be wide: it sets
     * the continuation cell for width-2 glyphs (the width is
     * QtTuiUnicode::clusterWidth(ch, combining)), and when either the target
     * cell or the continuation cell half-overwrites an existing wide pair,
     * the orphaned half is reset to a space (classic cell-grid correctness
     * rule). A wide glyph that would straddle the right buffer edge is
     * replaced by a space.
     */
    void putChar(int x, int y, char32_t ch, const Cell &style);
    void putChar(int x, int y, char32_t ch, const QString &combining, const Cell &style);

    /**
     * Returns the id of the hyperlink to url, for Cell::link. The same URL
     * always gives the same id, so a repaint with the same link changes no
     * cell. The function removes control characters from the URL. It
     * returns 0 (no link) for an empty URL and when the table is full.
     */
    quint16 linkId(const QString &url);
    /** Returns the URL of a link id. Returns a null string for an unknown id. */
    QString linkUrl(quint16 id) const;

    QVector<QPoint> diff() const;
    void swapBuffers();
    void invalidateAll();

    bool resizedDuringRender() const;
    void setResizedDuringRender(bool value);

private:
    int m_width;
    int m_height;
    QVector<Cell> m_front;
    QVector<Cell> m_back;
    bool m_resizedDuringRender = false;
    // One-time out-of-bounds warning flag (per buffer): OOB accesses can
    // happen thousands of times per frame, so only the first one warns.
    mutable bool m_oobWarned = false;
    // Hyperlink table: the URL of id n is m_linkUrls[n - 1]. The table
    // only grows, because cells and widgets can hold an id for a long
    // time. Its limit is 65535 different URLs for the life of the buffer.
    QStringList m_linkUrls;
    QHash<QString, quint16> m_linkIds;

    static Cell s_defaultCell;

    int index(int x, int y) const;
    bool inBounds(int x, int y) const;
    const Cell &outOfBoundsCell(int x, int y) const;
    void clearWidePairAt(int x, int y);
};

} // namespace QtTUI

// A model can return a Color in a QVariant (Qt::ForegroundRole and
// Qt::BackgroundRole, see QTuiAbstractItemView::itemStyle())
Q_DECLARE_METATYPE(QtTUI::Color)

#endif // QTTUI_SCREENBUFFER_H
