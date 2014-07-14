CXXFLAGS += -std=c++11
CXXFLAGS += -I../include
LDFLAGS += -L../lib
LDLIBS += -ldwarfidl -ldwarfpp -lsrk31c++ -lc++fileno -ldwarf -lelf -lboost_regex

default: dwarfidldump 

dwarfidldump: dwarfidldump.cpp print.o
	$(CXX) -o "$@" $(CXXFLAGS) $+ $(LDFLAGS) $(LDLIBS)

clean:
	rm -f dwarfidldump *.o
