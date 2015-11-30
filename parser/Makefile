THIS_MAKEFILE := $(lastword $(MAKEFILE_LIST))
GRAMMAR_TOPLEVEL ?= toplevel
GRAMMAR_OPTIONS ?= output=AST
GRAMMAR_PATH ?= .
ANTLR_M4_PATH ?= ../m4ntlr
ANTLR ?= java -classpath .:$(dir $(THIS_MAKEFILE))/../contrib/antlr-3.4-complete.jar:$(CLASSPATH) org.antlr.Tool
export CLASSPATH ANTLR ANTLR_M4_PATH GRAMMAR_TOPLEVEL GRAMMAR_OPTIONS GRAMMAR_PATH

CFLAGS += -fPIC
CFLAGS += -I$(GRAMMAR_PATH)/include
LDLIBS += -lantlr3c

default: all

dwarfidlSimpleCParser.c dwarfidlSimpleCLexer.c dwarfidlNewCParser.c dwarfidlNewCParser.c: sources

# We have to ask the Makerules to build both the sources *and*
# the test parser (since it links in the lexer and parser .o files).
sources:
	$(MAKE) -f $(ANTLR_M4_PATH)/Makerules GRAMMAR_BASENAME=dwarfidlSimple \
	    dwarfidlSimpleCParser.c dwarfidlSimpleCLexer.c dwarfidlSimple_test_parser
	$(MAKE) -f $(ANTLR_M4_PATH)/Makerules GRAMMAR_BASENAME=dwarfidlNew \
	      dwarfidlNewCParser.c dwarfidlNewCParser.c dwarfidlNew_test_parser

all: sources dwarfidlSimpleCParser.o dwarfidlSimpleCLexer.o \
   dwarfidlNewCParser.o dwarfidlNewCParser.o

clean::
	$(MAKE) -f $(ANTLR_M4_PATH)/Makerules GRAMMAR_BASENAME=dwarfidlSimple \
	      clean
	$(MAKE) -f $(ANTLR_M4_PATH)/Makerules GRAMMAR_BASENAME=dwarfidlNew \
	      clean
