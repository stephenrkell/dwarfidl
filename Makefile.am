ACLOCAL_AMFLAGS = -I m4

AM_CXXFLAGS = -std=c++11 -ggdb3 -O0 -Wall -Wno-deprecated-declarations -Wp,-w -Iinclude -Iinclude/dwarfidl -Iparser $(LIBSRK31CXX_CFLAGS) $(LIBCXXFILENO_CFLAGS) $(LIBANTLR3CXX_CFLAGS) $(LIBDWARFPP_CFLAGS) $(LIBCXXGEN_CFLAGS)

AM_LDFLAGS = -lstdc++ -lm


extra_DIST = dwarfidl.pc.in
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = dwarfidl.pc

dwarfidl_includedir = $(includedir)/dwarfidl
dwarfidl_include_HEADERS = include/dwarfidl/create.hpp include/dwarfidl/cxx_model.hpp include/dwarfidl/dwarfidl_cxx_target.hpp include/dwarfidl/dwarf_interface_walk.hpp include/dwarfidl/lang.hpp parser/dwarfidlSimpleCParser.h parser/dwarfidlSimpleCLexer.h

lib_LTLIBRARIES = src/libdwarfidl.la
src_libdwarfidl_la_SOURCES = src/cxx_model.cpp src/dwarfidl_cxx_target.cpp src/dwarf_interface_walk.cpp src/create.cpp src/lang.cpp src/print.cpp parser/dwarfidlSimpleCLexer.c parser/dwarfidlSimpleCParser.c
src_libdwarfidl_la_LIBADD = -lantlr3c -lboost_filesystem -lboost_regex -lboost_system -lboost_serialization $(LIBSRK31CXX_LIBS) $(LIBCXXFILENO_LIBS) $(LIBANTLR3CXX_LIBS) $(LIBDWARFPP_LIBS) $(LIBCXXGEN_LIBS)
src_libdwarfidl_la_CFLAGS = $(AM_CFLAGS)
src_libdwarfidl_la_CXXFLAGS = $(AM_CXXFLAGS)

PARSER_OBJS = parser/dwarfidlSimpleCLexer.o parser/dwarfidlSimpleCParser.o


noinst_PROGRAMS = parser/test_parser src/dwarfidldump src/test_dwarfprint src/dwarfhpp src/dwarf-ocaml-ctypesgen src/test_create

parser_test_parser_SOURCES = parser/test_parser.c parser/dwarfidlSimpleCLexer.c parser/dwarfidlSimpleCParser.c
parser_test_parser_LDADD = -lantlr3c 


src_dwarfidldump_SOURCES = src/dwarfidldump.cpp src/print.cpp
src_dwarfidldump_LDADD = src/libdwarfidl.la $(src_libdwarfidl_la_LIBADD) $(PARSER_OBJS)


src_test_dwarfprint_SOURCES = src/test_dwarfprint.cpp src/dwarfprint.cpp src/print.cpp
src_test_dwarfprint_LDADD = src/libdwarfidl.la $(src_libdwarfidl_la_LIBADD) $(PARSER_OBJS)


src_dwarfhpp_SOURCES = src/dwarfhpp.cpp
src_dwarfhpp_LDADD = src/libdwarfidl.la $(src_libdwarfidl_la_LIBADD) $(PARSER_OBJS)


src_dwarf_ocaml_ctypesgen_SOURCES = src/dwarf-ocaml-ctypesgen.cpp
src_dwarf_ocaml_ctypesgen_LDADD = src/libdwarfidl.la $(src_libdwarfidl_la_LIBADD) $(PARSER_OBJS)


src_test_create_SOURCES = src/create.cpp src/test_create.cpp src/dwarfprint.cpp src/print.cpp
src_test_create_LDADD = src/libdwarfidl.la $(src_libdwarfidl_la_LIBADD) $(PARSER_OBJS)


BUILT_SOURCES = parser/dwarfidlSimpleCLexer.c parser/dwarfidlSimpleCParser.c parser/dwarfidlSimpleCLexer.h parser/dwarfidlSimpleCParser.h
CLEANFILES = $(BUILT_SOURCES)

ANTLR = java -classpath contrib/antlr-3.4-complete.jar:$(CLASSPATH) org.antlr.Tool

parser/dwarfidlSimpleCLexer.c parser/dwarfidlSimpleCParser.c parser/include/dwarfidlSimpleCLexer.h parser/include/dwarfidlSimpleCParser.h: parser/dwarfidlSimpleC.g 
	$(ANTLR) "$<"	

