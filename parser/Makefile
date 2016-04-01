THIS_MAKEFILE := $(lastword $(MAKEFILE_LIST))
GRAMMAR_TOPLEVEL ?= toplevel
GRAMMAR_OPTIONS ?= output=AST
GRAMMAR_PATH ?= .

-include $(THIS_MAKEFILE)/../config.mk

ANTLR_M4_PATH ?= ../m4ntlr
ANTLR ?= java -classpath .:$(dir $(THIS_MAKEFILE))/../contrib/antlr-3.4-complete.jar:$(CLASSPATH) org.antlr.Tool
export CLASSPATH ANTLR ANTLR_M4_PATH GRAMMAR_TOPLEVEL GRAMMAR_OPTIONS GRAMMAR_PATH

CFLAGS += -fPIC -g
CFLAGS += -I$(GRAMMAR_PATH)/include
LDLIBS += -lantlr3c

default: all

dwarfidlSimpleCParser.c dwarfidlSimpleCLexer.c \
dwarfidlNewCParser.c dwarfidlNewCParser.c \
dwarfidlSimplePyParser.py dwarfidlSimplePyLexer.py \
dwarfidlNewPyParser.py dwarfidlNewPyParser.py \
: sources

# We have to ask the Makerules to build both the sources *and*
# the test parser (since it links in the lexer and parser .o files).
export CFLAGS
sources:
	$(MAKE) -f $(ANTLR_M4_PATH)/Makerules GRAMMAR_BASENAME=dwarfidlSimple \
	    dwarfidlSimpleCParser.c dwarfidlSimpleCLexer.c dwarfidlSimple_test_parser \
	    dwarfidlSimplePyParser.py dwarfidlSimplePyLexer.py
	$(MAKE) -f $(ANTLR_M4_PATH)/Makerules GRAMMAR_BASENAME=dwarfidlNew \
	      dwarfidlNewCParser.c dwarfidlNewCParser.c dwarfidlNew_test_parser \
	      dwarfidlNewPyParser.py dwarfidlNewPyLexer.py

all: sources dwarfidlSimpleCParser.o dwarfidlSimpleCLexer.o \
   dwarfidlNewCParser.o dwarfidlNewCParser.o \
   dwarfidlSimplePyParser.py dwarfidlSimplePyLexer.py \
   dwarfidlNewPyParser.py dwarfidlNewPyLexer.py

clean::
	$(MAKE) -f $(ANTLR_M4_PATH)/Makerules GRAMMAR_BASENAME=dwarfidlSimple \
	      clean
	$(MAKE) -f $(ANTLR_M4_PATH)/Makerules GRAMMAR_BASENAME=dwarfidlNew \
	      clean
