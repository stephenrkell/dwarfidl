ncurses_auto.ml: ../../src/dwarf-ocaml-ctypesgen
	echo -e 'initscr\nnewwin\nendwin\nrefresh\nwrefresh\naddstr\nmvwaddch\mvwaddstr\nbox\ncbreak' | \
../../src/dwarf-ocaml-ctypesgen /usr/lib/debug/lib/x86_64-linux-gnu/libncurses.so.5.* > "$@"

../../src/dwarf-ocaml-ctypesgen:
