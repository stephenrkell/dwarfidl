THIS_MAKEFILE := $(lastword $(MAKEFILE_LIST))
OCAMLOPT ?= ocamlfind ocamlopt
OCAMLOPTFLAGS += -linkpkg -package ctypes -package ctypes.foreign

CFLAGS += -g3 -gstrict-dwarf -std=c99 -fno-eliminate-unused-debug-types -O2
CXXFLAGS += -I$(realpath $(dir $(THIS_MAKEFILE)))/../include

LDFLAGS += -L$(realpath $(dir $(THIS_MAKEFILE)))/../lib

ncurses_auto/ncurses_auto: LDLIBS += -Wl,--no-as-needed -lncurses
lang-make-dies/lang-make-dies lang-make-dies: LDLIBS += -ldwarfidl -lcxxgen -lantlr3c -ldwarfpp -lc++fileno -lsrk31c++ -ldwarf -lelf

cases := $(wildcard [-a-z_]*)

INCLUDE_MK_INC = `if test -e $(dir $(realpath $(THIS_MAKEFILE)))/$*/mk.inc; then /bin/echo -f mk.inc; else true; fi`

default: checkall

%: %.ml
	$(OCAMLOPT) $(OCAMLOPTFLAGS) -o "$@" "$<" $(patsubst %,-cclib %,$(LDLIBS))

%.mli: %.ml

checkall: $(patsubst %,checkrun-%,$(cases))

# Some cases are expected to fail
checkrun-fail-%:
	@echo "expected-fail case is fail-$*" && \
	out=$$( echo -n "out is " 1>&2; mktemp | tee /dev/stderr ) && \
	err=$$( echo -n "err is " 1>&2; mktemp | tee /dev/stderr ) && \
	$(MAKE) cleanrun-fail-$* 1>$$out 2>$$err && \
		true # FIXME: output check goes here

checkrun-%:
	@echo "case is $*" && \
	out=$$( echo -n "out is " 1>&2; mktemp | tee /dev/stderr ) && \
	err=$$( echo -n "err is " 1>&2; mktemp | tee /dev/stderr ) && \
	$(MAKE) cleanrun-$* 1>$$out 2>$$err && \
		true # FIXME: output check goes here

# Some cases are special and require only an exit status of zero
checkrun-ncurses_auto: 
	$(MAKE) cleanrun-ncurses_auto >/dev/null 2>&1

_onlyrun-%:
	./$*

_onlygdbrun-%:
	gdb --args ./$* ./$*

build-%:
	$(MAKE) -C "$*" $(INCLUDE_MK_INC) -f ../makefile "$*" 

run-%:
	$(MAKE) build-$* && ( $(MAKE) -C "$*" $(INCLUDE_MK_INC) -f ../makefile _onlyrun-$* )

cleanbuild-%:
	$(MAKE) -C $* $(INCLUDE_MK_INC) -f ../makefile clean && \
	$(MAKE) build-$* 

cleanrun-%: 
	$(MAKE) -C $* $(INCLUDE_MK_INC) -f ../makefile clean && \
	$(MAKE) run-$*

gdbrun-%: # run the test case with itself as input
	$(MAKE) -C "$*" $(INCLUDE_MK_INC) "$*" && ( cd "$*" && $(MAKE) $(INCLUDE_MK_INC) -f ../makefile _onlygdbrun-$* )

gdbcleanrun-%:
	$(MAKE) -C $* $(INCLUDE_MK_INC) -f ../makefile clean && $(MAKE) gdbrun-$*

clean-%:
	$(MAKE) -C "$*" $(INCLUDE_MK_INC) -f $(realpath $(THIS_MAKEFILE)) clean

default:
	for case in $(cases); do \
            $(MAKE) run-$$case; \
        done

# generic clean rule that we can run from test dirs too (with $(MAKE) -f ../makefile)
clean: # (delete anything whose name is a prefix of a .c file's and doesn't contain a dot)
	rm -f $(filter-out .,$(patsubst %.c,%,$(shell find -name '*.c'))) $(filter-out .,$(patsubst %.cpp,%,$(shell find -name '*.cpp')))
	find -name '*.cmx'  -o -name '*.cmi' -o -name '*.o' -o -name '*.s' -o -name '*.i' | xargs rm -f
