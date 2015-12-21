#ifndef MESON_SRC
#error MESON_SRC must be defined
#endif // ifndef MESON_SRC
#ifndef MESON_DST
#error MESON_DST must be defined
#endif // ifndef MESON_DST

#define MESON_SRC_FILE(file) MESON_SRC file
#define MESON_DST_FILE(file) MESON_DST file

#define CATCH_CONFIG_PREFIX_ALL // all CATCH macros are prefixed with CATCH_
#include "catch.hpp"
