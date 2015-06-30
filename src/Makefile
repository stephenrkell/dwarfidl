-include config.mk

THIS_MAKEFILE := $(realpath $(lastword $(MAKEFILE_LIST)))

SRC := $(wildcard *.cpp)
DEPS = $(patsubst %.cpp,.%.d,$(SRC))

LIB_OBJS := cxx_model.o dwarfidl_cxx_target.o dwarf_interface_walk.o create.o lang.o

LDLIBS := -lantlr3c
LDLIBS += -lcxxgen -ldwarfpp
LDLIBS += -lsrk31c++ -lc++fileno 
LDLIBS += -ldwarf -lelf
LDLIBS += -lboost_filesystem -lboost_regex -lboost_system -lboost_serialization
LDLIBS += ../printer/dwarfprint.o

LDFLAGS += -L. -Wl,-R`pwd`

CXXFLAGS += -I ../include -I ../parser/include -I ../printer
CXXFLAGS += -g3 -O0 -Wall -fPIC -std=c++0x

# temporary HACK
CXXFLAGS += -Wno-deprecated-declarations

.PHONY: default
default: libdwarfidl.a libdwarfidl.so dwarfhpp dwarf-ocaml-ctypesgen test_parser test_create

.PHONY: clean
clean:: 
	rm -f dwarfhpp dwarf-ocaml-ctypesgen *.o *.so *.a
	rm -f $(DEPS)

$(DEPS) : .%.d : %.cpp parser/include/dwarfidlCParser.h parser/include/dwarfidlCLexer.h
	$(CXX) -MM $(CXXFLAGS) "$<"  > "$@"

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPS)
endif

dwarfhpp: dwarfhpp.o libdwarfidl.a
	$(CXX) -o "$@" "$<" $(LDFLAGS) -L. -ldwarfidl $(LDLIBS) 

dwarf-ocaml-ctypesgen: dwarf-ocaml-ctypesgen.o libdwarfidl.a
	$(CXX) -o "$@" "$<" $(LDFLAGS) -L. -ldwarfidl $(LDLIBS) 

test_parser: test_parser.o libdwarfidl.a libdwarfidl.so
	$(CXX) -o "$@" "$<" $(LDFLAGS) -L. -ldwarfidl $(LDLIBS)

test_create: test_create.o libdwarfidl.a libdwarfidl.so
	$(CXX) -o "$@" "$<" $(LDFLAGS) -L. -ldwarfidl $(LDLIBS)

#printer/dwarfidldump.o: CFLAGS += -Iprinter
#
#printer/dwarfidldump: printer/dwarfidldump.o printer/print.o
#	$(CXX) -o "$@" $+ $(LDFLAGS) -L. -ldwarfidl $(LDLIBS) 

libdwarfidl.a: $(LIB_OBJS)
	$(AR) r "$@" $^

libdwarfidl.so: LDLIBS := -lantlr3c -ldwarfpp -lsrk31c++ -lcxxgen -lboost_serialization
libdwarfidl.so: $(LIB_OBJS)
	$(CXX) -shared -o "$@" $(CXXFLAGS) $+ ../parser/dwarfidlSimpleCParser.o ../parser/dwarfidlSimpleCLexer.o $(LDFLAGS) $(LDLIBS)
