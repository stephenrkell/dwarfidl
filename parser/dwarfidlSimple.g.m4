include(head.g.m4)

include(dwarfidl-tokens.inc)
tokens {
dwarfidl_tokens
NAME = 'name';
TYPE = 'type';
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

dnl Don't generate the lexer::header stuff, because antlr3.5 doesn't like us
dnl antlr_m4_before_lexer_rules
include(dwarfidl-lex.inc)
