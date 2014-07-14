include(head.g.m4)

include(dwarfidl-tokens.inc)
tokens {
dwarfidl_tokens
}

antlr_m4_begin_rules

dnl recursion back-edge defines -- allow "virtually recursive rules" a.k.a. rule overriding
define(m4_value_description_production,dwarfValueDescription)

include(dwarfidl-rules.inc)
include(dwarfidl-abbrev.inc)
include(dwarfidl-lex.inc)
