# bugs in C++ generation

* bool versus _Bool
* DONE: noopgen: supply a body
* wchar_t?
* bitfields

# noopgen completion

* DONE: supplying a body
* DONE: supplying attributes
* DONE: we don't noop-prefix stuff in the preload library or in librunt or in libdlbind
  -- need to get our pubsyms from a more realistic link of the preload library
* yesops are UNDs -- OK, caused by wrong position of .a file on command line
* link errors about protected yesops and others "not defined locally"
  -- the data symbols are from librunt but these are missing from make rules
  -- librunt functions should be yesop'd just like others,
        but since they are not in the _preload.a, they are not. What to do?
        There's no .a-making command that resembles an ordinary link command.
        We only need the .a so we can objcopy it.
        Maybe we need another linker plugin?
        It would enumerate all the link inputs, output an archive and then exit(0).
        It's a bit like -Wl,-r but it does not combine sections or use a linker script in any way.

# Library cleanup

DONE: Make the naming function a virtual member function.

Most of libdwarfidl actually has nothing to do with the dwarfidl language.
The following files should be excised

cxx_model.cpp -- refactor s.t. we have
 * cxx_generator
 * template<class Model> cxx_generator_from_model
 * cxx_generator_from_dwarf : public cxx_generator_from_model<DwarfModel>

... the latter conceptually taking most code from dwarfidl_cxx_target.cpp,
although write_ordered_output and transitively_close can probably be pulled up
to the templated superclass.

DONE: deduplicate code ("emit_model" vs "cxx_decl_from_type_die" vs "make_declaration_of_type")
                               -- open dispatch on tag seems nice but really is overkill

then move cxx_generator and cxx_generator_from_model to libcxxgen
* cxx_model was originally in libdwarfpp! until 2013
* libcxxgen (already uses DWARF but) should only have utility code,
    e.g. maybe the thing that prints declarators, not
    tool-building code

This could be by a hg repo-slicing
operation, perhaps... need to check how that goes.

then reconsider dwarf_interface_walk.cpp -- can eliminate? move to libdwarfpp? rename as slice-interface?
    gather_interface_dies which is one possible "slicer" -- seems redundant? who else uses it?

Emitters can already take different approaches to naming
    e.g. of anonymous DIEs, or prefixing names, or...

Things we have:

 - emitting a C++ declaration modelling a DWARF-described entity
 - emitting a C++ *definition* modelling a DWARF-described entity
      (for stuff existing at run time: variables, subprograms, anything else?)
      (what about for struct types etc? the forward-decl vs full decl are not
        quite the same distinction as for decl-vs-def of functions etc.)

 - generating names for anonymous DIEs -- utility
 - extracting an 'interface', i.e. a collection of DIEs, from a bigger collection of DIEs
    -- it's not really an interface! it's just a dependency subgraph,
       i.e. a coherent "slice"
        -- "declaration slice" would be different from "implementation slice",
            it if ever makes sense to output code for internals e.g. .local variables
        ** rename 'dwarf_interface_walk' 'dwarf_interface_slice'?
           factor out a generic 'slice' helper(s)?
           This is possibly just a transitive closure operation
            plus an "immediate dependencies" notion
            ... where this "dependency" notion is the essence of the slicer
                    e.g. if a struct type is never instantiated in the interface,
                         it can be left a forward-decl and its members need not be
                         included. So following an edge needs to come with the "sense"
                         of the follow.
            When we do DWARF-slicing, we should remember the edges
            so we can topsort later. The edges define a graph.
                ** I used to have this working with BGL!
 - outputting a whole C++ model, i.e. set of declarations,
    of a DWARF interface, defined by a collection of DIEs,
    using a particular approach to naming/referencing anonymous DIEs
        -- i.e. we use synthetic names aggressively, rather than
              using some kind of topsorting / containment analysis
              to reproduce exactly what was written e.g. in the case of
                   typedef struct { ... } blah_t;
    ACTUALLY
        noopgen emits an "implementation model"
        dwarfhpp emits an "interface model"


# Merge grammars

The dwarfidl language has a complicated history, and remains something
of a mess. Originally, dwarfidl was a fragment of the Cake language. It
was born as a separate entity in 2011 by isolating the part of that
language used to describe binary interfaces by augmentation of (or
substitution for missing) debugging information.

The Cake-derived grammar was modified in 2014 (ed95f92) to yield what is
now called the 'dwarfidlSimple' grammar, intended to temporarily replace
the Cake-derived one and to be more uniform (intending improvements to
be back-ported, but this still hasn't happened). This is the grammar
used by liballocs to describe composite types implied by code like
`malloc(sizeof (T) + sizeof (S))`, processed by the `create_dies`
function. There is an unfinished dumper in this syntax, `dwarfidldump`,
in examples/ and the underlying print code is in src/print.cpp. Examples
of the syntax can be generated by liballocs's tools in the '.allocs'
files; after building liballocs, see
`/usr/lib/meta/path/to/liballocs/tests/offsetof/offsetof.allocs` for one
example.

In 2015 Jon French wrote a new grammar to serve the libfootprints
project, which consists of tools for specifying and checking the memory
access footprints of system calls and the like. This grammar is now
called 'dwarfidlNew' and its last significant changes were in 50ca031.
The printing code is in src/dwarfprint.cpp and does work. A test case
called `dwarfprint' (added in 2020) exercises this code. A sample of the
syntax as dumped is in tests/dwarfprint/sample-output.txt.

A long-neglected goal has been to unite these two strands, possibly
as part of publishing the libfootprints work.

## Summary of language grammars, parsers and printers

| syntax |  print code        | test/example dumper       | example syntax                     |
|--------|--------------------|---------------------------|------------------------------------||
| simple | src/print.cpp      | examples/dwarfidldump.cpp | see liballocs                      |
| new    | src/dwarfprint.cpp | tests/dwarfprint          | tests/dwarfprint/sample-output.txt |

# Augmenters

One problem with DWARF that comes out of compilers is that it omits some details, such as
those implied by the calling convention. It's be good for dwarfidl descriptions to include
these details, and that means recovering them by means other than simply reading what the
compiler generated... in short we'd like other ways of obtaining information and merging it
together.

DWARF augmenters can work at the dwarfidl (text) or libdwarfpp (binary) level, but dwarfidl
seems a reasonable place to keep them either way.

* Here "text" means "generate dwarfidl
source". In liballocs, the dumpallocs CIL tool can (just about) be considered a
dwarfidl-generating augmenter, because it generates dwarfidl syntax. This is merged in by
one of the other tools.

* Meanwhile, "binary" means adding DIEs to a libdwarfpp `root_die` data structure;
liballocstool does some of this, including

** using dwarfidl's `create_dies()` for the synthetic
** turning the text into binary DIEs
** creating the DIEs for void, for void* as existentially quantified generic pointer, etc.

Other augmenters we want:

 * calling convention augmenter of location exprs
 * back-propagator for kernel interface DWARF (if kernel still does typeinfo-erasure)
 * call site augmenter for syscall sites, including "constant argument" info
 * (ideally) call site augmenter for allocation sites -- to add the type info from `dumpallocs`

(SK: see also my mail to Guillaume of Wed, 29 May 2019.)

What's the best way to do augmenters? is "generate textual dwarfidl" a good way?

* `create_dies()` is OK for things that fit best in a separate compilation unit (CU),
less when they need to be mindled in an existing CU.
* whereas e.g. the backpropagator may want intra-CU additions
** for this, need to refactor libdwarfpp e.g. to have rational offsets

# Related libdwarfpp changes

* migation to libdw
* switch from `Dwarf_Off` to rational offsets, to allow insertion without breaking order invariants

# dwarfidl changes
DONE: merge changes on ernest-6 dwarfidl.hg
DONE: parser and printer stock-taking (did Jon make a general dumper + parser? yes)
DONE: clarify which grammar is which:  ("Simple" is old / Cake-derived, "New" is Jon's)
DONE: printer: make it print a whole big file
SKIP: parser merging
