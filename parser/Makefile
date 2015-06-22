GRAMMAR_BASENAME ?= dwarfidlSimple
GRAMMAR_TOPLEVEL ?= toplevel
GRAMMAR_OPTIONS ?= output=AST
GRAMMAR_PATH ?= .

LIBDWARFPP_PATH ?= ../../libdwarfpp.hg
ANTLR_M4_PATH ?= ../../antlr-m4
M4PATH := $(ANTLR_M4_PATH):$(dir $(THIS_MAKEFILE)):$(M4_PATH)
$(warning M4PATH is $(M4PATH))
export M4PATH

CFLAGS += -fPIC

default: include/dwarfidlSimpleCParser.h include/dwarfidlSimpleCLexer.h dwarfidlSimpleCParser.o dwarfidlSimpleCLexer.o dwarfidlSimplyPyParser.py

include $(ANTLR_M4_PATH)/Makerules


#test:: test-samples
#
#.PHONY: test-samples
#test-samples: dwarfidlPyParser.py 
#	for fname in samples/test.dwarfidl ; do \
#		python dwarfidlPyParser.py --rule=toplevel < "$$fname"; \
#	done | $(ANTLR_M4_PATH)/parens-filter.sh
#
#.PHONY: test-lexer-samples
#test-lexer-samples: dwarfidlPyLexer.py # samples/ephy.cake samples/xcl.cake
#	for fname in samples/test.dwarfidl ; do \
#		python dwarfidlPyLexer.py < "$$fname"; \
#	done 

dwarfidlSimple.g: dwarfidl-tag-keywords-dwarf3.inc dwarfidl-attr-keywords-dwarf3.inc

dwarfidl-tag-keywords-%.inc: $(LIBDWARFPP_PATH)/spec/%.py
	/bin/echo -e "for tag in tag_map.keys():\n\tprint tag\n\n" | python -i "$<" /dev/stdin | \
sed "s/.*/| '&'/" > "$@" || (rm -f "$@"; false)

dwarfidl-attr-keywords-%.inc: $(LIBDWARFPP_PATH)/spec/%.py
	/bin/echo -e "for attr in attr_type_map.keys():\n\tprint attr\n\n" | python -i "$<" /dev/stdin | \
sed "s/.*/| '&'/" > "$@" || (rm -f "$@"; false)
