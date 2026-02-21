#ifndef QTTUI_ANSIRENDERER_H
#define QTTUI_ANSIRENDERER_H

#include <QtTUI/ScreenBuffer.h>
#include <QByteArray>
#include <QPoint>

namespace QtTUI {

class AnsiRenderer {
public:
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
