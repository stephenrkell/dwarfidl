#!/bin/bash

program_ml=$(mktemp --suffix=.ml)

cat > "$program_ml" <<EOF
open Ncurses_auto
open Unsigned

let () =
  let main_window = initscr () in
  ignore(cbreak ());
  let small_window = newwin (Int32.of_int 10) (Int32.of_int 10) (Int32.of_int 5) (Int32.of_int 5) in 
  mvwaddstr main_window (Int32.of_int 1) (Int32.of_int 2) "Hello";
  mvwaddstr small_window (Int32.of_int 2) (Int32.of_int 2) "World";
  box small_window (UInt64.of_int 0) (UInt64.of_int 0);
  refresh ();
  Unix.sleep 1;
  wrefresh small_window;
  Unix.sleep 5;
  endwin(); ()
EOF

ncurses_ml=$(mktemp --suffix=.ml)
	/bin/echo -e 'initscr\nnewwin\nendwin\nrefresh\nwrefresh\naddstr\nmvwaddch\nmvwaddstr\nbox\ncbreak' | tee /dev/stderr | \
../examples/generate-ocaml-ctypes /usr/lib/debug/lib/x86_64-linux-gnu/libncurses.so.5.* > "$ncurses_ml" &&
	ocamlfind ocamlopt -linkpkg -package ctypes -package ctypes.foreign -cclib -Wl,--no-as-needed -cclib -lncurses "$ncurses_ml" "$program_ml"

# FIXME: put this somewhere:
# ncurses_auto/ncurses_auto: LDLIBS += -Wl,--no-as-needed -lncurses


# to debug the generate step:
#	gdb ../examples/generate-ocaml-ctypes --eval-command \
"run /usr/lib/debug/lib/x86_64-linux-gnu/libncurses.so.5.9 <<<\$$'initscr\nnewwin\nendwin\nrefresh\nwrefresh\naddstr\nmvwaddch\nmvwaddstr\nbox\ncbreak'" \
