ncurses_auto.ml: ../../src/dwarf-ocaml-ctypesgen
	/bin/echo -e 'initscr\nnewwin\nendwin\nrefresh\nwrefresh\naddstr\nmvwaddch\nmvwaddstr\nbox\ncbreak' | tee /dev/stderr | \
../../src/dwarf-ocaml-ctypesgen /usr/lib/debug/lib/x86_64-linux-gnu/libncurses.so.5.* > "$@"

../../src/dwarf-ocaml-ctypesgen:

ncurses_auto: ncurses_auto.ml hello.ml
	ocamlfind ocamlopt -o "$@" -linkpkg -package ctypes -package ctypes.foreign -cclib -Wl,--no-as-needed -cclib -lncurses ncurses_auto.ml hello.ml

debug:
	gdb ../../src/dwarf-ocaml-ctypesgen --eval-command \
"run /usr/lib/debug/lib/x86_64-linux-gnu/libncurses.so.5.9 <<<\$$'initscr\nnewwin\nendwin\nrefresh\nwrefresh\naddstr\nmvwaddch\nmvwaddstr\nbox\ncbreak'" \
