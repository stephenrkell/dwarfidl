grammar dwarfidlSimpleC;


options {
    language=C;
    output=AST;
    backtrack=true;
}

tokens {
    DIES;
    DIE;
    ATTR;
    ATTRS;
    CHILDREN;
    NAME='name';
    TYPE='type';
    FOOTPRINT='footprint';
    RELATIVE_OFFSET;
    ABSOLUTE_OFFSET;
    OPCODE;
    OPCODE_LIST;
    IDENTS;

    FP_DIRECTBYTES;
    FP_DEREFSIZES;
    FP_DEREFBYTES;
    FP_UNION;
    FP_FOR; // for_loop
    FP_IF; // if_cond
    FP_GT; // binary
    FP_LT; // bin
    FP_GTE; // bin
    FP_LTE; // bin
    FP_EQ; // bin
    FP_NE; // bin
    FP_AND; // bin
    FP_OR; // bin
    FP_NOT; // unary
    FP_ADD; // bin
    FP_SUB; // bin
    FP_MUL; // bin
    FP_DIV; // bin
    FP_MOD; // bin
	FP_NEG; // unary
    FP_SHL; // bin
    FP_SHR; // bin
    FP_BITAND; // bin
    FP_BITOR; // bin
    FP_BITXOR; // bin
    FP_BITNOT; // bin
    FP_MEMBER; // bin
    FP_CLAUSES;
    FP_CLAUSE;
    FP_SUBSCRIPT;
	FP_TRUE; // const
	FP_FALSE; // const
    FP_SIZEOF; // unary
    FP_DEREF; // unary
    FP_VOID;
    FP_FUN;
    FP_ARGS;
    FP_APP; // binary
	SUBSCRIPT_SCALAR;
	SUBSCRIPT_RANGE;
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
OPEN_ANGLE : '<';
CLOSE_ANGLE : '>';
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
RANGE : '..';
EXCL : '!';
DOT : '.';

KEYWORD_TRUE : 'true';
KEYWORD_FALSE : 'false';
KEYWORD_FOR : 'for';
KEYWORD_IN : 'in';
KEYWORD_R : 'r';
KEYWORD_W : 'w';
KEYWORD_RW : 'rw';
KEYWORD_SIZEOF : 'sizeof';
KEYWORD_IF : 'if';
KEYWORD_THEN : 'then';
KEYWORD_ELSE : 'else';
KEYWORD_AND : 'and';
KEYWORD_OR : 'or';
KEYWORD_NOT : 'not';
KEYWORD_VOID: 'void';
KEYWORD_FUN: 'fun';
KEYWORD_FOOTPRINT_FUNCTION: 'footprint_function';


// // for debugging using antlrworks -- uncomment these and comment
// // the following normal KEYWORD_TAG and KEYWORD_ATTR definitions
// // because apparently Java can't deal with so many keywords in
// // a lexer as it produces too large a class file
// // (I know, seriously, right)
//KEYWORD_TAG : 'base_type' | 'pointer_type' | 'typedef' | 'structure_type' | 'const_type' | 'subprogram' | 'member' | 'formal_parameter';
//KEYWORD_ATTR :  'no_attr';

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
    | 'footprint'
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
identifier : (i+=IDENT | i+=KEYWORD_ATTR | i+=NAME | i+=TYPE | i+=KEYWORD_TAG | /*i+=KEYWORD_TRUE | i+=KEYWORD_FALSE | i+=KEYWORD_FOR | i+=KEYWORD_IN |*/ i+=KEYWORD_R | i+=KEYWORD_W | i+=KEYWORD_RW | i+=KEYWORD_SIZEOF /*| i+=KEYWORD_IF | i+=KEYWORD_THEN | i+=KEYWORD_ELSE | i+=KEYWORD_AND | i+=KEYWORD_OR | i+=KEYWORD_NOT*/ )+
        -> ^(IDENTS $i+)
    ;
//identifier : IDENT -> ^(IDENTS IDENT);

fp_clauses : OPEN_BRACE fp_clause* CLOSE_BRACE
        -> ^(FP_CLAUSES fp_clause+)  
    ;

fp_clause : expression SEMICOLON -> ^(FP_CLAUSE KEYWORD_RW expression)
    | (a=KEYWORD_R|a=KEYWORD_W|a=KEYWORD_RW) COLON expression SEMICOLON -> ^(FP_CLAUSE $a expression)
    ;

loc_opcode : IDENT (OPEN INT (COMMA INT)? CLOSE)? SEMICOLON
        -> ^(OPCODE IDENT INT*)
    ;
loc_list : OPEN_BRACE loc_opcode* CLOSE_BRACE
        -> ^(OPCODE_LIST loc_opcode*)
    ;
attr_key : KEYWORD_ATTR | NAME | TYPE | INT;
attr_value : (boolean_value | INT | offset | loc_list | STRING_LIT);
attr : FOOTPRINT EQUALS fp_clauses -> ^(ATTR FOOTPRINT fp_clauses)
	|	attr_key EQUALS attr_value -> ^(ATTR attr_key attr_value);
attr_list : OPEN_SQUARE (attr (COMMA attr)*)? CLOSE_SQUARE
        -> attr+
    ;

die_tag : KEYWORD_TAG | INT;
die_name: identifier -> ^(ATTR NAME identifier);
die_type : identifier -> ^(ATTR TYPE identifier)
    | offset -> ^(ATTR TYPE offset);

subprogram_arg
    : die_name (COLON die_type)?
    -> ^(DIE KEYWORD_TAG["formal_parameter"] ^(ATTRS die_name die_type?) CHILDREN)
    ;

subprogram_die
    : offset? 'subprogram' die_name? ((OPEN (args+=subprogram_arg (COMMA args+=subprogram_arg)*)? CLOSE ('->' die_type)?)| COLON die_type)? attr_list? (OPEN_BRACE die* CLOSE_BRACE)? SEMICOLON
    -> ^(DIE KEYWORD_TAG["subprogram"] ^(ATTRS die_name? die_type? attr_list) ^(CHILDREN $args * die*) offset)
    ;

other_die : offset? die_tag die_name? (COLON die_type)? attr_list? (OPEN_BRACE die* CLOSE_BRACE)? SEMICOLON
        -> ^(DIE die_tag ^(ATTRS die_name die_type attr_list) ^(CHILDREN die*) offset)
    ;

die : subprogram_die | other_die | toplevel_function;

toplevel : die* -> ^(DIES die*);

expression : union_expression;

union_expression
	: (for_expression->for_expression) ((COMMA for_expression)+ -> ^(FP_UNION $union_expression for_expression+))?
	;

for_expression
    : (conditional_expression->conditional_expression)
      (KEYWORD_FOR identifier KEYWORD_IN conditional_expression -> ^(FP_FOR $for_expression identifier conditional_expression))?
    ;


conditional_expression
	: (logical_or_expression->logical_or_expression)
    | (KEYWORD_IF logical_or_expression KEYWORD_THEN logical_or_expression KEYWORD_ELSE logical_or_expression)
        -> ^(FP_IF logical_or_expression+)
	;

logical_or_expression
	: (logical_and_expression->logical_and_expression)
      (KEYWORD_OR logical_and_expression -> ^(FP_OR $logical_or_expression logical_and_expression))*
	;

logical_and_expression
	: (inclusive_or_expression->inclusive_or_expression)
      (KEYWORD_AND inclusive_or_expression -> ^(FP_AND $logical_and_expression inclusive_or_expression))*
	;

inclusive_or_expression
	: (exclusive_or_expression->exclusive_or_expression)
        ('|' exclusive_or_expression -> ^(FP_BITOR $inclusive_or_expression exclusive_or_expression))*
	;

exclusive_or_expression
	: (and_expression->and_expression) ('^' and_expression -> ^(FP_BITXOR $exclusive_or_expression and_expression))*
	;

and_expression
	: (equality_expression->equality_expression) ('&' equality_expression -> ^(FP_BITAND $and_expression equality_expression))*
	;
equality_expression
	: (relational_expression->relational_expression) ('==' relational_expression -> ^(FP_EQ $equality_expression relational_expression)
                                    |'!=' relational_expression -> ^(FP_NE $equality_expression relational_expression))*
	;

relational_expression
	: (shift_expression->shift_expression) ('<' shift_expression -> ^(FP_LT $relational_expression shift_expression)
                               |'>' shift_expression -> ^(FP_GT $relational_expression shift_expression) 
                              |'<=' shift_expression -> ^(FP_LTE $relational_expression shift_expression)
                              |'>=' shift_expression -> ^(FP_GTE $relational_expression shift_expression))*
	;

shift_expression
	: (additive_expression->additive_expression) ('<<' additive_expression -> ^(FP_SHL $shift_expression additive_expression)
	                      |'>>' additive_expression -> ^(FP_SHR $shift_expression additive_expression))*
	;

additive_expression
	: (multiplicative_expression->multiplicative_expression) ('+' multiplicative_expression -> ^(FP_ADD $additive_expression multiplicative_expression)
	                                    |'-' multiplicative_expression -> ^(FP_SUB $additive_expression multiplicative_expression))*
	;

multiplicative_expression
	: (unary_expression->unary_expression) ('*' unary_expression -> ^(FP_MUL $multiplicative_expression unary_expression)
                              | '/' unary_expression -> ^(FP_DIV $multiplicative_expression unary_expression)
		                      | '%' unary_expression -> ^(FP_MOD $multiplicative_expression unary_expression))*
;

unary_expression
	: '-' postfix_expression -> ^(FP_NEG postfix_expression)
	| '~' postfix_expression -> ^(FP_BITNOT postfix_expression)
    | '*' postfix_expression -> ^(FP_DEREF postfix_expression)
	| KEYWORD_NOT postfix_expression -> ^(FP_NOT postfix_expression)
	| KEYWORD_SIZEOF OPEN postfix_expression CLOSE -> ^(FP_SIZEOF postfix_expression)
    | postfix_expression
	;

postfix_expression
	:   (primary_expression->primary_expression)
        (   '[' expression (RANGE expression)? ']' -> ^(FP_SUBSCRIPT FP_DEREFSIZES $postfix_expression expression+)
		|   '{' expression (RANGE expression)? '}' -> ^(FP_SUBSCRIPT FP_DIRECTBYTES $postfix_expression expression+)
		|   '[{' expression (RANGE expression)? '}]' -> ^(FP_SUBSCRIPT FP_DEREFBYTES $postfix_expression expression+)
        |   '.' identifier -> ^(FP_MEMBER $postfix_expression identifier)
		|   OPEN args+=for_expression (COMMA args+=for_expression)* CLOSE -> ^(FP_APP $postfix_expression ^(FP_ARGS $args +))
        )*
	;

function_definition : name=identifier OPEN args+=identifier (COMMA args+=identifier)* CLOSE OPEN_BRACE expression CLOSE_BRACE
	  			-> ^(FP_FUN $name ^(FP_ARGS $args +) expression)
				;
			
primary_expression
	: identifier
	| INT
    | KEYWORD_TRUE -> FP_TRUE | KEYWORD_FALSE -> FP_FALSE
    | KEYWORD_VOID -> FP_VOID
	| (OPEN expression CLOSE -> expression)
    | KEYWORD_FUN function_definition -> function_definition
	; 

toplevel_function
 : KEYWORD_FOOTPRINT_FUNCTION function_definition -> function_definition;
   
