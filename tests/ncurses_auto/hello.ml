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
