include(head.g.m4)

include(dwarfidl-tokens.inc)
tokens {
dwarfidl_tokens
}

antlr_m4_begin_rules

dnl recursion back-edge defines -- allow "virtually recursive rules" a.k.a. rule overriding
define(m4_value_description_production,dwarfValueDescription)

include(dwarfidl-simple-rules.inc)

tagKeyword : 'root' 
include(dwarfidl-tag-keywords-dwarf3.inc)
;
attrKeyword : 'no_attr' 
include(dwarfidl-attr-keywords-dwarf3.inc)
;

include(dwarfidl-abbrev.inc)
include(dwarfidl-lex.inc)
