ACLOCAL_AMFLAGS = -I m4

AM_CXXFLAGS = -std=c++11 -ggdb -O2 -Wall -Wno-deprecated-declarations -Iinclude -Iinclude/dwarfidl -Iparser $(LIBSRK31CXX_CFLAGS) $(LIBCXXFILENO_CFLAGS) $(LIBANTLR3CXX_CFLAGS) $(LIBDWARFPP_CFLAGS) $(LIBCXXGEN_CFLAGS) -fPIC -DPIC

AM_CFLAGS = -fPIC -DPIC -g -O2 -Iinclude -Iinclude/dwarfidl -Iparser

AM_LDFLAGS = -lstdc++ -lm

extra_DIST = dwarfidl.pc.in
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = dwarfidl.pc

dwarfidl_includedir = $(includedir)/dwarfidl
dwarfidl_include_HEADERS = include/dwarfidl/create.hpp include/dwarfidl/cxx_model.hpp \
  include/dwarfidl/dwarfidl_cxx_target.hpp include/dwarfidl/dwarf_interface_walk.hpp \
  include/dwarfidl/lang.hpp include/dwarfidl/dwarfidlNewCParser.h include/dwarfidl/dwarfidlNewCLexer.h \
  include/dwarfidl/dwarfidlNewCLexer.h include/dwarfidl/dwarfidlNewCParser.h

lib_LTLIBRARIES = src/libdwarfidl.la
src_libdwarfidl_la_SOURCES = src/cxx_model.cpp src/dwarfidl_cxx_target.cpp src/dwarf_interface_walk.cpp src/create.cpp src/lang.cpp src/print.cpp parser/dwarfidlNewCLexer.c parser/dwarfidlNewCParser.c
src_libdwarfidl_la_LIBADD = -lantlr3c -lboost_filesystem -lboost_regex -lboost_system -lboost_serialization $(LIBSRK31CXX_LIBS) $(LIBCXXFILENO_LIBS) $(LIBANTLR3CXX_LIBS) $(LIBDWARFPP_LIBS) $(LIBCXXGEN_LIBS)
src_libdwarfidl_la_CFLAGS = $(AM_CFLAGS)
src_libdwarfidl_la_CXXFLAGS = $(AM_CXXFLAGS)

SUBDIRS = parser

all: include/dwarfidl/dwarfidlNewCLexer.h include/dwarfidl/dwarfidlNewCParser.h

include/dwarfidl/dwarfidlNewCLexer.h: parser
	cd include/dwarfidl && ln -sf ../../parser/dwarfidlNewCLexer.h .

include/dwarfidl/dwarfidlNewCParser.h: parser
	cd include/dwarfidl && ln -sf ../../parser/dwarfidlNewCParser.h .

lib/libdwarfidl.so: $(lib_LTLIBRARIES)
	mkdir -p lib && cd lib && ln -sf ../src/.libs/libdwarfidl.so .

lib/libdwarfidl.so.0: $(lib_LTLIBRARIES)
	mkdir -p lib && cd lib && ln -sf ../src/.libs/libdwarfidl.so.0 .

all: lib/libdwarfidl.so lib/libdwarfidl.so.0

bin_PROGRAMS = src/dwarfidldump src/dwarfhpp src/dwarf-ocaml-ctypesgen

src_dwarfidldump_SOURCES = src/dwarfidldump.cpp src/print.cpp
src_dwarfidldump_LDADD = src/libdwarfidl.la $(src_libdwarfidl_la_LIBADD) $(PARSER_OBJS) -ldwarf -lelf


src_dwarfhpp_SOURCES = src/dwarfhpp.cpp
src_dwarfhpp_LDADD = src/libdwarfidl.la $(src_libdwarfidl_la_LIBADD) $(PARSER_OBJS) -ldwarf -lelf


src_dwarf_ocaml_ctypesgen_SOURCES = src/dwarf-ocaml-ctypesgen.cpp
src_dwarf_ocaml_ctypesgen_LDADD = src/libdwarfidl.la $(src_libdwarfidl_la_LIBADD) $(PARSER_OBJS) -ldwarf -lelf

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

export LIBCXXFILENO_CXXFLAGS LIBCXXFILENO_LIBS \
LIBSRK31CXX_CXXFLAGS LIBSRK31CXX_LIBS \
LIBDWARFPP_CXXFLAGS LIBDWARFPP_LIBS \
LIBCXXGEN_CXXFLAGS LIBCXXGEN_LIBS \
LIBANTLR3CXX_CXXFLAGS LIBANTLR3CXX_LIBS
