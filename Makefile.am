ACLOCAL_AMFLAGS = -I m4

AM_CXXFLAGS = -std=c++11 -ggdb -O2 -Wall -Wno-deprecated-declarations -Iinclude -Iinclude/dwarfidl -Iparser $(LIBSRK31CXX_CFLAGS) $(LIBCXXFILENO_CFLAGS) $(LIBANTLR3CXX_CFLAGS) $(LIBDWARFPP_CFLAGS) $(LIBCXXGEN_CFLAGS) -fPIC -DPIC

AM_CFLAGS = -fPIC -DPIC -g -O2 -Iinclude -Iinclude/dwarfidl -Iparser

AM_LDFLAGS = -lstdc++ -lm

extra_DIST = dwarfidl.pc.in
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = dwarfidl.pc

dwarfidl_includedir = $(includedir)/dwarfidl
dwarfidl_include_HEADERS = include/dwarfidl/create.hpp include/dwarfidl/cxx_model.hpp \
  include/dwarfidl/dependency_ordering_cxx_target.hpp include/dwarfidl/dwarf_interface_walk.hpp \
  include/dwarfidl/print.hpp include/dwarfidl/dwarfprint.hpp \
  include/dwarfidl/lang.hpp include/dwarfidl/dwarfidlNewCParser.h include/dwarfidl/dwarfidlNewCLexer.h \
  include/dwarfidl/dwarfidlNewCLexer.h include/dwarfidl/dwarfidlNewCParser.h

lib_LTLIBRARIES = src/libdwarfidl.la
src_libdwarfidl_la_SOURCES = src/cxx_model.cpp src/dependency_ordering_cxx_target.cpp src/dwarf_interface_walk.cpp src/create.cpp src/lang.cpp src/print.cpp src/dwarfprint.cpp parser/dwarfidlNewCLexer.c parser/dwarfidlNewCParser.c
src_libdwarfidl_la_LIBADD = -lantlr3c -lboost_filesystem -lboost_regex -lboost_system -lboost_serialization $(LIBANTLR3CXX_LIBS) $(LIBCXXGEN_LIBS) $(LIBDWARFPP_LIBS) $(LIBSRK31CXX_LIBS) $(LIBCXXFILENO_LIBS) -lz
src_libdwarfidl_la_LDFLAGS = -Wl,-rpath,$(realpath $(top_srcdir))/lib
src_libdwarfidl_la_CFLAGS = $(AM_CFLAGS)
src_libdwarfidl_la_CXXFLAGS = $(AM_CXXFLAGS)

SUBDIRS = parser include . lib

bin_PROGRAMS = examples/dwarfidldump examples/dwarfhpp examples/generate-ocaml-ctypes

examples_dwarfidldump_SOURCES = examples/dwarfidldump.cpp src/print.cpp
examples_dwarfidldump_LDADD = src/libdwarfidl.la $(src_libdwarfidl_la_LIBADD) $(PARSER_OBJS) -lelf

examples_dwarfhpp_SOURCES = examples/dwarfhpp.cpp
examples_dwarfhpp_LDADD = src/libdwarfidl.la $(src_libdwarfidl_la_LIBADD) $(PARSER_OBJS) -lelf

examples_generate_ocaml_ctypes_SOURCES = examples/generate-ocaml-ctypes.cpp
examples_generate_ocaml_ctypes_LDADD = src/libdwarfidl.la $(src_libdwarfidl_la_LIBADD) $(PARSER_OBJS) -lelf

# pkg-config doesn't understand PKG_CXXFLAGS, but I'm buggered
# if I'm going to have my Makefiles use _CFLAGS to mean _CXXFLAGS.
# So, if we find we have _CFLAGS set for these, either from pkg.m4
# or because pkg.m4 told the user to set this var, and if we *don't*
# have _CXXFLAGS for these things, propagate. Only one of the USER_*_CFLAGS
# and *_CFLAGS should be non-empty. Note that if we got this far,
# we *must* have a _CFLAGS, even if the user (sanely) didn't call it that,
# because pkg-config will have complained if we didn't fake it up.
if SUPPLIED_LIBCXXFILENO_CFLAGS
LIBCXXFILENO_CXXFLAGS = $(USER_LIBCXXFILENO_CFLAGS)
else
LIBCXXFILENO_CXXFLAGS = $(LIBCXXFILENO_CFLAGS)
endif
if SUPPLIED_LIBDWARFPP_CFLAGS
LIBDWARFPP_CXXFLAGS = $(USER_LIBDWARFPP_CFLAGS)
else
LIBDWARFPP_CXXFLAGS = $(LIBDWARFPP_CFLAGS)
endif
if SUPPLIED_LIBCXXGEN_CFLAGS
LIBCXXGEN_CXXFLAGS = $(USER_LIBCXXGEN_CFLAGS)
else
LIBCXXGEN_CXXFLAGS = $(LIBCXXGEN_CFLAGS)
endif
if SUPPLIED_LIBSRK31CXX_CFLAGS
LIBSRK31CXX_CXXFLAGS = $(USER_LIBSRK31CXX_CFLAGS)
else
LIBSRK31CXX_CXXFLAGS = $(LIBSRK31CXX_CFLAGS)
endif
if SUPPLIED_LIBANTLR3CXX_CFLAGS
LIBANTLR3CXX_CXXFLAGS = $(USER_LIBANTLR3CXX_CFLAGS)
else
LIBANTLR3CXX_CXXFLAGS = $(LIBANTLR3CXX_CFLAGS)
endif

# For libantlr3c, there's no pkg-config, so we just use CPPFLAGS and LDFLAGS
# to pass the configuration. We need to make sure that those variables
# are passed through to sub-makes.
export LIBCXXFILENO_CXXFLAGS LIBCXXFILENO_LIBS \
LIBSRK31CXX_CXXFLAGS LIBSRK31CXX_LIBS \
LIBDWARFPP_CXXFLAGS LIBDWARFPP_LIBS \
LIBCXXGEN_CXXFLAGS LIBCXXGEN_LIBS \
LIBANTLR3CXX_CXXFLAGS LIBANTLR3CXX_LIBS \
CPPFLAGS LDFLAGS
