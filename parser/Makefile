THIS_MAKEFILE := $(lastword $(MAKEFILE_LIST))
$(info THIS_MAKEFILE is $(THIS_MAKEFILE))
GRAMMAR_TOPLEVEL ?= toplevel
GRAMMAR_OPTIONS ?= output=AST
GRAMMAR_PATH ?= .

-include $(realpath $(dir $(THIS_MAKEFILE)))/../config.mk

M4NTLR_PATH ?= $(dir $(THIS_MAKEFILE))/../contrib/m4ntlr
ANTLR ?= java -classpath .:$(ANTLR3_JAR):$(CLASSPATH) org.antlr.Tool
export CLASSPATH ANTLR M4NTLR_PATH GRAMMAR_TOPLEVEL GRAMMAR_OPTIONS GRAMMAR_PATH

CFLAGS += -fPIC -g
CFLAGS += -I$(GRAMMAR_PATH)/include
LDLIBS += -lantlr3c

default: all

dwarfidlSimpleCParser.c dwarfidlSimpleCLexer.c dwarfidlSimpleCParser.h dwarfidlSimpleCLexer.h \
dwarfidlNewCParser.c dwarfidlNewCParser.c dwarfidlNewCParser.h dwarfidlNewCParser.h \
dwarfidlSimplePyParser.py dwarfidlSimplePyLexer.py \
dwarfidlNewPyParser.py dwarfidlNewPyLexer.py \
: sources

# We have to ask the Makerules to build both the sources *and*
# the test parser (since it links in the lexer and parser .o files).
export CFLAGS
sources:
	$(MAKE) -f $(M4NTLR_PATH)/Makerules GRAMMAR_BASENAME=dwarfidlSimple \
	    dwarfidlSimpleCParser.c dwarfidlSimpleCLexer.c dwarfidlSimple_test_parser \
	    dwarfidlSimplePyParser.py dwarfidlSimplePyLexer.py
	$(MAKE) -f $(M4NTLR_PATH)/Makerules GRAMMAR_BASENAME=dwarfidlNew \
	      dwarfidlNewCParser.c dwarfidlNewCParser.c dwarfidlNew_test_parser \
	      dwarfidlNewPyParser.py dwarfidlNewPyLexer.py

all: sources dwarfidlSimpleCParser.o dwarfidlSimpleCLexer.o \
   dwarfidlNewCParser.o dwarfidlNewCParser.o \
   dwarfidlSimplePyParser.py dwarfidlSimplePyLexer.py \
   dwarfidlNewPyParser.py dwarfidlNewPyLexer.py

clean::
	$(MAKE) -f $(M4NTLR_PATH)/Makerules GRAMMAR_BASENAME=dwarfidlSimple \
	      clean
	$(MAKE) -f $(M4NTLR_PATH)/Makerules GRAMMAR_BASENAME=dwarfidlNew \
	      clean
