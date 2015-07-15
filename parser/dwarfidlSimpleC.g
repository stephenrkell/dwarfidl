grammar dwarfidlSimpleC;

options {
    language=C;
    output=AST;
}

tokens {
    DIES;
    DIE;
    ATTR;
    ATTRS;
    CHILDREN;
    NAME='name';
    TYPE='type';
    RELATIVE_OFFSET;
    ABSOLUTE_OFFSET;
    OPCODE;
    OPCODE_LIST;
    IDENTS;
}

fragment ALPHA : ('A'..'Z'|'a'..'z');
fragment NONZERO_DIGIT : ('1'..'9');
fragment DIGIT : ('0'..'9');
fragment HEX_DIGIT : ('0'..'9'|'a'..'f'|'A'..'F');
fragment SPACE : ' ';

fragment HEX_INT : '0x' HEX_DIGIT+;
fragment OCTAL_INT : '0' NONZERO_DIGIT DIGIT*;
fragment UNSIGNED_INT : (ZERO | NONZERO_DIGIT DIGIT*) ('U'|'u');
fragment SIGNED_INT : '-'? NONZERO_DIGIT DIGIT*;
fragment ZERO : '0';

OPEN_BRACE : '{';
CLOSE_BRACE : '}';
OPEN : '(';
CLOSE : ')';
OPEN_SQUARE : '[';
CLOSE_SQUARE : ']';
SINGLE_QUOTE : '\'';
DOUBLE_QUOTE : '"';
ESCAPE_CHAR : '\\';

//ESCAPE : (ESCAPE_CHAR (('U'|'u') HEX_DIGIT HEX_DIGIT HEX_DIGIT HEX_DIGIT | ('A'..'T'|'V'..'Z'|'a'..'t'|'v'..'z') | SPACE | SINGLE_QUOTE | DOUBLE_QUOTE));

COMMA : ',';
COLON : ':';
SEMICOLON : ';';
UNDERSCORE : '_';
HYPHEN : '-';
EQUALS : '=';
AT : '@';
PLUS : '+';

KEYWORD_TRUE : 'true';
KEYWORD_FALSE : 'false';

KEYWORD_TAG : 'root' 
| 'inlined_subroutine'
| 'ptr_to_member_type'
| 'restrict_type'
| 'compile_unit'
| 'file_type'
| 'namespace'
| 'interface_type'
| 'subprogram'
| 'string_type'
| 'module'
| 'variable'
| 'entry_point'
| 'enumeration_type'
| 'imported_declaration'
| 'dwarf_procedure'
| 'thrown_type'
| 'subrange_type'
| 'pointer_type'
| 'inheritance'
| 'catch_block'
| 'union_type'
| 'reference_type'
| 'volatile_type'
| 'label'
| 'member'
| 'common_block'
| 'base_type'
| 'structure_type'
| 'try_block'
| 'constant'
| 'const_type'
| 'namelist'
| 'subroutine_type'
| 'enumerator'
| 'class_type'
| 'packed_type'
| 'variant'
| 'typedef'
| 'set_type'
| 'variant_part'
| 'shared_type'
| 'array_type'
| 'imported_module'
| 'condition'
| 'rvalue_reference_type'
| 'partial_unit'
| 'formal_parameter'
| 'unspecified_parameters'
| 'unspecified_type'
| 'with_stmt'
| 'common_inclusion'
| 'friend'
| 'imported_unit'
| 'access_declaration'
| 'namelist_item'
| 'template_type_parameter'
| 'template_value_parameter'
| 'lexical_block';

KEYWORD_ATTR : 'no_attr' 
| 'small'
| 'discr_list'
| 'producer'
| 'encoding'
| 'call_file'
| 'byte_stride'
| 'declaration'
| 'accessibility'
| 'sibling'
| 'call_column'
| 'const_value'
| 'data_member_location'
| 'vtable_elem_location'
| 'bit_offset'
| 'is_optional'
| 'associated'
| 'allocated'
| 'decimal_sign'
| 'call_line'
| 'use_location'
| 'static_link'
| 'count'
| 'default_value'
| 'priority'
| 'calling_convention'
| 'ordering'
| 'specification'
| 'extension'
| 'endianity'
| 'artificial'
| 'containing_type'
| 'location'
| 'pure'
| 'bit_stride'
| 'entry_pc'
| 'import'
| 'variable_parameter'
| 'decl_file'
| 'object_pointer'
| 'friend'
| 'string_length'
| 'binary_scale'
| 'address_class'
| 'description'
| 'elemental'
| 'return_addr'
| 'lower_bound'
| 'decl_column'
| 'trampoline'
| 'visibility'
| 'bit_size'
| 'ranges'
| 'upper_bound'
| 'threads_scaled'
| 'external'
| 'start_scope'
| 'base_types'
| 'abstract_origin'
| 'use_UTF8'
| 'segment'
| 'comp_dir'
| 'data_location'
| 'stmt_list'
| 'frame_base'
| 'language'
| 'identifier_case'
| 'picture_string'
| 'byte_size'
| 'explicit'
| 'common_reference'
| 'high_pc'
| 'low_pc'
| 'decl_line'
| 'discr'
| 'decimal_scale'
| 'digit_count'
| 'virtuality'
| 'prototyped'
| 'inline'
| 'mutable'
| 'namelist_item'
| 'macro_info'
| 'discr_value';

fragment BEGIN_IDENT_CHAR : ALPHA | UNDERSCORE;
fragment IDENT_CHAR : ALPHA | UNDERSCORE | HYPHEN | DIGIT;
fragment END_IDENT_CHAR : ALPHA | UNDERSCORE | DIGIT;

IDENT : BEGIN_IDENT_CHAR (IDENT_CHAR* END_IDENT_CHAR)?;

STRING_LIT : (SINGLE_QUOTE (~SINGLE_QUOTE | ESCAPE_CHAR SINGLE_QUOTE)* SINGLE_QUOTE) | (DOUBLE_QUOTE (~DOUBLE_QUOTE | ESCAPE_CHAR DOUBLE_QUOTE)* DOUBLE_QUOTE);

NEWLINE:'\r'? '\n' { $channel=HIDDEN; $line = $line + 1; };
BLANKS : ('\t'|' ')+ { $channel=HIDDEN; };
LINE_COMMENT : ('//' .* '\r'? '\n') { $channel=HIDDEN; };
BLOCK_COMMENT : ('/*' .* '*/') { $channel=HIDDEN; };

INT : HEX_INT | OCTAL_INT | UNSIGNED_INT | SIGNED_INT | ZERO;

boolean_value : KEYWORD_TRUE | KEYWORD_FALSE;

absolute_offset : AT INT
                -> ^(ABSOLUTE_OFFSET INT)
    ;
relative_offset : PLUS INT
        -> ^(RELATIVE_OFFSET INT)
    ;
offset : absolute_offset | relative_offset;

// Explicitly support keywords in identifiers
identifier : (i+=IDENT | i+=KEYWORD_ATTR | i+=NAME | i+=TYPE | i+=KEYWORD_TAG | i+=KEYWORD_TRUE | i+=KEYWORD_FALSE)+
        -> ^(IDENTS $i+)
    ;

//identifier : IDENT;

loc_opcode : IDENT (OPEN INT (COMMA INT)? CLOSE)? SEMICOLON
        -> ^(OPCODE IDENT INT*)
    ;
loc_list : OPEN_BRACE loc_opcode* CLOSE_BRACE
        -> ^(OPCODE_LIST loc_opcode*)
    ;
attr_key : KEYWORD_ATTR | NAME | TYPE | INT;
attr_value : (boolean_value | INT | offset | loc_list | STRING_LIT);
attr : attr_key EQUALS attr_value
    -> ^(ATTR attr_key attr_value);
attr_list : OPEN_SQUARE attr (COMMA attr)* CLOSE_SQUARE
        -> attr+
    ;

die_tag : KEYWORD_TAG | INT;
die_name: identifier -> ^(ATTR NAME identifier);
die_type : identifier -> ^(ATTR TYPE identifier)
    | offset -> ^(ATTR TYPE offset);

die : offset? die_tag die_name? (COLON die_type)? attr_list? (OPEN_BRACE die* CLOSE_BRACE)? SEMICOLON
        -> ^(DIE die_tag ^(ATTRS die_name die_type attr_list) ^(CHILDREN die*) offset)
    ;

toplevel : die* -> ^(DIES die*);
