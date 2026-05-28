#ifndef QTTUI_ANSIRENDERER_H
#define QTTUI_ANSIRENDERER_H

#include <QtTUI/ScreenBuffer.h>
#include <QByteArray>
#include <QPoint>

namespace QtTUI {

class AnsiRenderer {
public:
    // Color depth of the output. TrueColor writes 24-bit colors. Palette256
    // and Palette16 write the nearest palette color. NoColor writes no
    // colors; a cell with a background lighter than its foreground (focus,
    // selection) gets reverse video, so it stays visible.
    enum ColorMode { TrueColor, Palette256, Palette16, NoColor };

    static ColorMode colorMode();
    static void setColorMode(ColorMode mode);

    // Color mode for the terminal, from the environment, in this order:
    // QTTUI_COLOR (truecolor, 256, 16 or none), a non-empty NO_COLOR,
    // COLORTERM (truecolor or 24bit), then TERM (*-256color, dumb).
    static ColorMode detectColorMode();

    QByteArray render(const ScreenBuffer &buffer);

    // Serialization helpers
    static QByteArray sgrSequence(const Cell &cell);
    static QByteArray cursorPosition(int x, int y);  // 0-based to 1-based CUP
    static QByteArray cursorMove(int dx, int dy);     // Relative CU[ABCD]
    static QByteArray resetAttributes();

    // Parsing helpers (for round-trip testing)
    static Cell parseSgrSequence(const QByteArray &seq);
    static QPoint parseCupSequence(const QByteArray &seq);
};

} // namespace QtTUI

#endif // QTTUI_ANSIRENDERER_H
