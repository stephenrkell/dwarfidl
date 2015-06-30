-include config.mk

CXXFLAGS += -std=c++11 -g3
CXXFLAGS += -I../include -Wno-deprecated-declarations
LDFLAGS += -L../lib
LDLIBS += -ldwarfidl -ldwarfpp -lsrk31c++ -lc++fileno -ldwarf -lelf -lboost_regex

default: dwarfidldump test_dwarfprint

dwarfidldump: dwarfidldump.cpp print.o
	$(CXX) -o "$@" $(CXXFLAGS) $+ $(LDFLAGS) $(LDLIBS)

test_dwarfprint: dwarfprint.o test_dwarfprint.o
	$(CXX) -o "$@" $(CXXFLAGS) $+ $(LDFLAGS) $(LDLIBS)

clean:
	rm -f dwarfidldump test_dwarfprint *.o
