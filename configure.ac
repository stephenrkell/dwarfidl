AC_CONFIG_MACRO_DIR([m4])
AC_PREREQ([2.69])
AC_INIT([dwarfidl], [0.1], [srk31@cl.cam.ac.uk])
AC_CONFIG_SRCDIR([src/create.cpp])
CFLAGS="-I$prefix/include $CFLAGS"
CXXFLAGS="-I$prefix/include $CXXFLAGS"
CPPFLAGS="-I$prefix/include $CPPFLAGS"
LDFLAGS="-L$prefix/lib $LDFLAGS"
PKG_CONFIG_PATH="$prefix/lib/pkgconfig:$PKG_CONFIG_PATH"
export CFLAGS CXXFLAGS LDFLAGS PKG_CONFIG_PATH
LT_INIT
AC_LANG([C++])
AM_INIT_AUTOMAKE([foreign subdir-objects -Wno-portability])
AM_MAINTAINER_MODE
AC_PROG_CXX

AX_CXX_COMPILE_STDCXX_11
export CXXFLAGS

AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T
AC_CHECK_TYPES([ptrdiff_t])
AC_PROG_LN_S
AC_PROG_INSTALL

OLD_CPPFLAGS=$CPPFLAGS
CPPFLAGS="-std=c++11 $CPPFLAGS"
AC_CHECK_HEADERS([algorithm assert.h cassert cctype cmath cstddef cstdio cstdlib fstream functional iostream iterator limits map memory regex set sstream stdio.h string unordered_map unordered_set], [], [AC_MSG_FAILURE([required standard headers: algorithm assert.h cassert cctype cmath cstddef cstdio cstdlib fstream functional iostream iterator limits map memory regex set sstream stdio.h string unordered_map unordered_set])])

AX_BOOST_BASE

AC_CHECK_HEADER([boost/regex/concepts.hpp], [], [AC_MSG_FAILURE([boost::regex is required])])
AC_HAVE_LIBRARY(boost_regex, [], [AC_MSG_FAILURE([boost::regex is required])])

AC_CHECK_HEADER([antlr3.h], [], [AC_MSG_FAILURE([antlr3 is required])])
AC_HAVE_LIBRARY(antlr3c, [], [AC_MSG_FAILURE([antlr3 is required])])

CPPFLAGS=$OLD_CPPFLAGS

PKG_PROG_PKG_CONFIG
PKG_CHECK_MODULES([LIBCXXFILENO], [libcxxfileno])
PKG_CHECK_MODULES([LIBSRK31CXX], [libsrk31cxx])
PKG_CHECK_MODULES([LIBANTLR3CXX], [libantlr3cxx])
PKG_CHECK_MODULES([LIBDWARFPP], [libdwarfpp])
PKG_CHECK_MODULES([LIBCXXGEN], [libcxxgen])

AC_OUTPUT([Makefile dwarfidl.pc])
