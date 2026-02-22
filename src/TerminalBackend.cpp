#include <QtTUI/TerminalBackend.h>

#ifdef _WIN32
#include "WindowsBackend.h"
#else
#include "PosixBackend.h"
#endif

namespace QtTUI {

std::unique_ptr<TerminalBackend> createTerminalBackend(QObject *parent)
{
#ifdef _WIN32
    return std::make_unique<WindowsBackend>(parent);
#else
    return std::make_unique<PosixBackend>(parent);
#endif
}

} // namespace QtTUI
