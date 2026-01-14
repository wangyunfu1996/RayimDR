#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(IRAYDETECTOR_LIB)
#  define IRAYDETECTOR_EXPORT Q_DECL_EXPORT
# else
#  define IRAYDETECTOR_EXPORT Q_DECL_IMPORT
# endif
#else
# define IRAYDETECTOR_EXPORT
#endif
