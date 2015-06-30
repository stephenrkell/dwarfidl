-include config.mk

GRAMMAR_BASENAME ?= dwarfidlSimpleC
GRAMMAR_TOPLEVEL ?= toplevel
GRAMMAR_OPTIONS ?= output=AST
GRAMMAR_PATH ?= .

LIBDWARFPP_PATH ?= ../../libdwarfpp.hg
ANTLR_M4_PATH ?= ../../antlr-m4
CFLAGS += -fPIC

default: dwarfidlSimpleCParser.o dwarfidlSimpleCLexer.o test_parser

THIS_MAKEFILE := $(lastword $(MAKEFILE_LIST))

ANTLR ?= java -classpath .:$(CLASSPATH) org.antlr.Tool

CFLAGS += -I$(GRAMMAR_PATH)/include

$(GRAMMAR_PATH)/%CLexer.c \
$(GRAMMAR_PATH)/%CParser.c \
$(GRAMMAR_PATH)/include/%CLexer.h \
$(GRAMMAR_PATH)/include/%CParser.h: $(GRAMMAR_PATH)/%C.g 
	$(ANTLR) "$<"
	mkdir -p $(GRAMMAR_PATH)/include && \
	mv \
		$(GRAMMAR_PATH)/$*CLexer.h $(GRAMMAR_PATH)/$*CParser.h \
		$(GRAMMAR_PATH)/include/

%.o: %.c
	$(CC) $(CFLAGS) -o "$@" -c "$<"

clean::
	cd $(GRAMMAR_PATH)/ && (rm -f *.h *.o *.a *.d; \
	rm -f *Parser.c *Lexer.c; \
	rm -f $(GRAMMAR_BASENAME)/*.tokens; \
	rm -rf include; \
	rm -f test_parser)

LDLIBS += -L/home/ojno/antlr3.5/runtime/C/.libs -lantlr3c

test_parser: test_parser.o dwarfidlSimpleCParser.o dwarfidlSimpleCLexer.o
	$(CXX) -o "$@" "$<" $(LDFLAGS) -L. $(LDLIBS) dwarfidlSimpleCParser.o dwarfidlSimpleCLexer.o


