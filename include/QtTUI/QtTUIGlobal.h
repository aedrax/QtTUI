#ifndef QTTUIGLOBAL_H
#define QTTUIGLOBAL_H

#include <QtCore/QtGlobal>

// QtTUI is built as a STATIC library only (declared explicitly in
// src/CMakeLists.txt), so this header intentionally defines no QTTUI_EXPORT
// visibility macro. Supporting shared builds would require defining an export
// macro here (Q_DECL_EXPORT / Q_DECL_IMPORT switched on a build-side define)
// and decorating every public class across the ~80 public headers. Until that
// pass is done, do not enable BUILD_SHARED_LIBS for this library.

#endif // QTTUIGLOBAL_H
