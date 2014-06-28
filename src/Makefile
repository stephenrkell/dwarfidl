THIS_MAKEFILE := $(realpath $(lastword $(MAKEFILE_LIST)))

SRC := $(wildcard *.cpp)
DEPS = $(patsubst %.cpp,.%.d,$(SRC))
HDRS := $(wildcard *.hpp)

OBJS += $(patsubst %.cpp,%.o,$(SRC)) 

LDLIBS := -lantlr3c
LDLIBS += -lcxxgen -ldwarfpp -ldwarf -lelf
LDLIBS += -lsrk31c++ -lc++fileno 
LDLIBS += -lboost_filesystem -lboost_regex -lboost_system -lboost_serialization

LDFLAGS += -L. -Wl,-R`pwd`

CXXFLAGS += -Iparser/include -I../include
CXXFLAGS += -g3 -O0 -Wall -std=c++0x 

# temporary HACK
CXXFLAGS += -Wno-deprecated-declarations

.PHONY: default
default: libdwarfidl.a dwarfhpp dwarf-ocaml-ctypesgen printer/dwarfidldump #parser/test_parser

.PHONY: clean
clean:: 
	rm -f dwarfhpp *.o *.so
	rm -f $(DEPS)

$(DEPS) : .%.d : %.cpp # parser/include/dwarfidlCParser.h parser/include/dwarfidlCLexer.h
	$(CXX) -MM $(CXXFLAGS) "$<"  > "$@"

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPS)
endif

#$(warning OBJS is $(OBJS))

.PHONY: parser
parser: $(OBJS) parser/dwarfidlCParser.o parser/dwarfidlCLexer.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) \
	 $(OBJS) parser/dwarfidlCParser.o parser/dwarfidlCLexer.o $(LDLIBS) -o "$@" 

dwarfhpp: dwarfhpp.o libdwarfidl.a
	$(CXX) -o "$@" "$<" $(LDFLAGS) -L. -ldwarfidl $(LDLIBS) 

dwarf-ocaml-ctypesgen: dwarf-ocaml-ctypesgen.o libdwarfidl.a
	$(CXX) -o "$@" "$<" $(LDFLAGS) -L. -ldwarfidl $(LDLIBS) 

printer/dwarfidldump.o: CFLAGS += -Iprinter

printer/dwarfidldump: printer/dwarfidldump.o printer/print.o
	$(CXX) -o "$@" $+ $(LDFLAGS) -L. -ldwarfidl $(LDLIBS) 

libdwarfidl.a: cxx_model.o cxx_dependency_order.o
	$(AR) r "$@" $^

# Begin parser rules 
ANTLR_M4_PATH ?= ../../antlr-m4
M4PATH := $(ANTLR_M4_PATH):$(dir $(THIS_MAKEFILE))/parser:$(M4_PATH)
$(warning M4PATH is $(M4PATH))
export M4PATH
GRAMMAR_OPTIONS += backtrack=true output=AST
GRAMMAR_BASENAME := dwarfidl
GRAMMAR_PATH := parser
-include $(ANTLR_M4_PATH)/Makerules

test:: test-samples

.PHONY: test-samples
test-samples: parser/dwarfidlPyParser.py 
	for fname in parser/samples/test.dwarfidl ; do \
		python parser/dwarfidlPyParser.py --rule=toplevel < "$$fname"; \
	done | $(ANTLR_M4_PATH)/parens-filter.sh

.PHONY: test-lexer-samples
test-lexer-samples: parser/dwarfidlPyLexer.py # samples/ephy.cake samples/xcl.cake
	for fname in parser/samples/test.dwarfidl ; do \
		python parser/dwarfidlPyLexer.py < "$$fname"; \
	done 
