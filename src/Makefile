THIS_MAKEFILE := $(realpath $(lastword $(MAKEFILE_LIST)))

SRC := $(wildcard *.cpp)
DEPS = $(patsubst %.cpp,.%.d,$(SRC))

LIB_OBJS := cxx_model.o cxx_dependency_order.o create.o 

LDLIBS := -lantlr3c
LDLIBS += -lcxxgen -ldwarfpp
LDLIBS += -lsrk31c++ -lc++fileno 
LDLIBS += -ldwarf -lelf
LDLIBS += -lboost_filesystem -lboost_regex -lboost_system -lboost_serialization

LDFLAGS += -L. -Wl,-R`pwd`

CXXFLAGS += -I../include
CXXFLAGS += -g3 -O0 -Wall -fPIC -std=c++0x 

# temporary HACK
CXXFLAGS += -Wno-deprecated-declarations

.PHONY: default
default: libdwarfidl.a libdwarfidl.so dwarfhpp dwarf-ocaml-ctypesgen

.PHONY: clean
clean:: 
	rm -f dwarfhpp *.o *.so *.a
	rm -f $(DEPS)
	$(MAKE) -C parser clean

$(DEPS) : .%.d : %.cpp # parser/include/dwarfidlCParser.h parser/include/dwarfidlCLexer.h
	$(CXX) -MM $(CXXFLAGS) "$<"  > "$@"

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPS)
endif

dwarfhpp: dwarfhpp.o libdwarfidl.a
	$(CXX) -o "$@" "$<" $(LDFLAGS) -L. -ldwarfidl $(LDLIBS) 

dwarf-ocaml-ctypesgen: dwarf-ocaml-ctypesgen.o libdwarfidl.a
	$(CXX) -o "$@" "$<" $(LDFLAGS) -L. -ldwarfidl $(LDLIBS) 

#printer/dwarfidldump.o: CFLAGS += -Iprinter
#
#printer/dwarfidldump: printer/dwarfidldump.o printer/print.o
#	$(CXX) -o "$@" $+ $(LDFLAGS) -L. -ldwarfidl $(LDLIBS) 

libdwarfidl.a: $(LIB_OBJS)
	$(AR) r "$@" $^

libdwarfidl.so: LDLIBS := -ldwarfpp -lsrk31c++ -lcxxgen -lboost_serialization
libdwarfidl.so: $(LIB_OBJS)
	$(CXX) -shared -o "$@" $(CXXFLAGS) $+ ../parser/dwarfidlSimpleCParser.o ../parser/dwarfidlSimpleCLexer.o $(LDFLAGS) $(LDLIBS)
