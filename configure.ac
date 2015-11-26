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

# If the user (sanely) supplied _CXXFLAGS, and not _CFLAGS, 
# duplicate the latter to the former.  See rant about pkg-config in Makefile.am.
# We save the old _CFLAGS.
USER_LIBCXXFILENO_CFLAGS="$LIBCXXFILENO_CFLAGS"
LIBCXXFILENO_CFLAGS="$LIBCXXFILENO_CXXFLAGS"
USER_LIBDWARFPP_CFLAGS="$LIBDWARFPP_CFLAGS"
LIBDWARFPP_CFLAGS="$LIBDWARFPP_CXXFLAGS"
USER_LIBCXXGEN_CFLAGS="$LIBCXXGEN_CFLAGS"
LIBCXXGEN_CFLAGS="$LIBCXXGEN_CXXFLAGS"
USER_LIBSRK31CXX_CFLAGS="$LIBSRK31CXX_CFLAGS"
LIBSRK31CXX_CFLAGS="$LIBSRK31CXX_CXXFLAGS"
USER_LIBANTLR3CXX_CFLAGS="$LIBANTLR3CXX_CFLAGS"
LIBANTLR3CXX_CFLAGS="$LIBANTLR3CXX_CXXFLAGS"

# Now, all the information from pkg-config or from user _CXXFLAGS) is in _CFLAGS.
# We might also have USER_ _CFLAGS, if the user did what pkg.m4 stupidly told them.
# We use this information (i.e. propagate to _CXXFLAGS) only if the user *didn't* give us
# _CXXFLAGS.
AM_CONDITIONAL(SUPPLIED_LIBCXXFILENO_CFLAGS, [test x"$USER_LIBCXXFILENO_CFLAGS" != x && test x"$LIBCXXFILENO_CXXFLAGS" = x])
AM_CONDITIONAL(SUPPLIED_LIBDWARFPP_CFLAGS, [test x"$USER_LIBDWARFPP_CFLAGS" != x && test x"$LIBDWARFPP_CXXFLAGS" = x])
AM_CONDITIONAL(SUPPLIED_LIBCXXGEN_CFLAGS, [test x"$USER_LIBCXXGEN_CFLAGS" != x && test x"$LIBCXXGEN_CXXFLAGS" = x])
AM_CONDITIONAL(SUPPLIED_LIBSRK31CXX_CFLAGS, [test x"$USER_LIBSRK31CXX_CFLAGS" != x && test x"$LIBSRK31CXX_CXXFLAGS" = x])
AM_CONDITIONAL(SUPPLIED_LIBANTLR3CXX_CFLAGS, [test x"$USER_LIBANTLR3CXX_CFLAGS" != x && test x"$LIBANTLR3CXX_CXXFLAGS" = x])

# HACK: antlr3cxx doesn't need any libs, but PKG_CHECK_MODULES will 
# complain if we don't have LIBANTLR3CXX_LIBS set to something
# non-empty. So use an effect-free linker option.
# LIBANTLR3CXX_LIBS=-Wl,-g
# -- actually, the invoker of configure should do this.

PKG_PROG_PKG_CONFIG
PKG_CHECK_MODULES([LIBCXXFILENO], [libcxxfileno])
PKG_CHECK_MODULES([LIBSRK31CXX], [libsrk31cxx])
PKG_CHECK_MODULES([LIBANTLR3CXX], [libantlr3cxx])
PKG_CHECK_MODULES([LIBDWARFPP], [libdwarfpp])
PKG_CHECK_MODULES([LIBCXXGEN], [libcxxgen])

AC_OUTPUT([Makefile dwarfidl.pc])
