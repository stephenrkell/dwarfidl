/* FIXME: rename this to something more neutral
 * like "named element definition" */
/* FIXME: provide explicit compile units */
/* FIXME: allow offset labelling here using the "@" prefix e.g.
 * @ 11: _: compile_unit { ... }; */

include(dwarfidl-tokens.inc)


/* utility code -- defined here, shared by Cake */
memberNameExpr      : definiteMemberName -> ^( DEFINITE_MEMBER_NAME definiteMemberName )
                    | KEYWORD_VOID^ /* means "actually, no member" */
                    | INDEFINITE_MEMBER_NAME -> ANY_VALUE 
                    ;

/* DWARF version */
/* FIXME: do we ever want memberNameExpr and not a single name? */
dwarfNamedElement : 
          (memberNameExpr ':' KEYWORD_CLASS_OF)=>       /* named DWARF type */
                   memberNameExpr ':' KEYWORD_CLASS_OF valueDescriptionExpr SEMICOLON
                     -> ^( MEMBERSHIP_CLAIM memberNameExpr ^( KEYWORD_CLASS_OF  valueDescriptionExpr ) )
                 | 
          (memberNameExpr ':' KEYWORD_CONST)=>          /* named constant */
                   memberNameExpr ':' KEYWORD_CONST namedDwarfTypeDescription '=' constantValueDescription SEMICOLON
                     -> ^( MEMBERSHIP_CLAIM memberNameExpr ^( KEYWORD_CONST namedDwarfTypeDescription constantValueDescription ) )
                 | namedValueDescription SEMICOLON      /* named variable or subprogram */
                     -> ^( MEMBERSHIP_CLAIM namedValueDescription )
                 ;

/* FIXME: we want to unify this with restrictedPostfixExpression somehow.
 * The latter allows more stuff though. This should really just be a path
 * through a static structure, representable as a list of idents. 
 * BUT we want to be able to represent arrays. 
 * Maybe idents beginning with digits are okay? Maybe we just define 
 * "member selection on arrays" for digit-based? */
definiteMemberName : IDENT^ ( memberSuffix^ )*
                   ;
memberSuffix : '.' IDENT
         -> ^( '.' IDENT )
       | '[' constantIntegerArithmeticExpression ']'
         -> ^( ARRAY_SUBSCRIPT constantIntegerArithmeticExpression )
         ;

dwarfType : dwarfStructuredValueDescription^ ( valueDescriptionModifierSuffix^ )*
          | simpleValueDescription^ ( valueDescriptionModifierSuffix^ )*
          | enumValueDescription^ ( valueDescriptionModifierSuffix^ )*
          | KEYWORD_VOID^ ( valueDescriptionModifierSuffix^ )*
          | '('! m4_value_description_production ')'! ( valueDescriptionModifierSuffix^ )*
          ;

valueDescriptionModifierSuffix : KEYWORD_PTR
                               | '[' arraySizeExpr? ']' -> ^( ARRAY arraySizeExpr? )
                               ;

arraySizeExpr : constantIntegerArithmeticExpression;

simpleValueDescription      : namedDwarfTypeDescription^
                            | INDEFINITE_MEMBER_NAME -> ANY_VALUE
                            ;

byteSizeParameter           : '<'! INT '>'!
                            ;

namedDwarfTypeDescription   : KEYWORD_BASE^ dwarfBaseTypeDescription
                            | IDENT // FIXME: should be memberNameExpr
                            ;

dwarfBaseTypeDescription    : encoding=IDENT byteSizeParameter? dwarfBaseTypeAttributeList
                               -> ^( DWARF_BASE_TYPE $encoding dwarfBaseTypeAttributeList byteSizeParameter?  )
                            ;

dwarfBaseTypeAttributeList : ( '{' ( dwarfBaseTypeAttributeDefinition )* '}' )?
                                -> ^( DWARF_BASE_TYPE_ATTRIBUTE_LIST dwarfBaseTypeAttributeDefinition* )
                              ;

dwarfBaseTypeAttributeDefinition   : attr=IDENT '=' ( value=IDENT | value=INT ) SEMICOLON
                                     -> ^( DWARF_BASE_TYPE_ATTRIBUTE $attr $value )
                                  ;
enumValueDescription    : KEYWORD_ENUM^ dwarfBaseTypeDescription enumDefinition
                        | KEYWORD_ENUM enumDefinition
                         -> ^( KEYWORD_ENUM ^( DWARF_BASE_TYPE ) enumDefinition ) /* empty base type => int */
                        ;

enumDefinition  : '{'! enumElement* '}'!
                ;

enumElement : 
            (KEYWORD_ENUMERATOR IDENT EQ)=>
            KEYWORD_ENUMERATOR name=IDENT EQ constantIntegerArithmeticExpression SEMICOLON
            -> ^( KEYWORD_ENUMERATOR ^( KEYWORD_LET ^( IDENTS_TO_BIND $name ) constantIntegerArithmeticExpression ) )
            | ident=IDENT SEMICOLON
            -> ^( KEYWORD_ENUMERATOR IDENT ) /* using an ident defined elsewhere (must be constant value!) */
            ;

/* Top level descriptive expr */
valueDescriptionExpr        : m4_value_description_production 
    -> ^( VALUE_DESCRIPTION m4_value_description_production)
                            ;

/* DWARF version */
dwarfValueDescription :
 (dwarfMultiValueDescription FUNCTION_ARROW)=> 
       dwarfMultiValueDescription FUNCTION_ARROW dwarfValueDescription 
           -> ^(FUNCTION_ARROW dwarfMultiValueDescription dwarfValueDescription )
    | dwarfPrimitiveValueDescription
    ;

/* DWARF version */
dwarfConstantValueDescription: KEYWORD_CONST^ constantValueDescription
                             ;

/* DWARF version */
dwarfPrimitiveValueDescription   : dwarfConstantValueDescription
                                 | dwarfType
                                 ;
/* DWARF version: no modes! */
dwarfMultiValueDescription : '(' optionallyNamedValueDescription (',' optionallyNamedValueDescription )* ( ',' ELLIPSIS )? ')'
                                -> ^( MULTIVALUE optionallyNamedValueDescription* ELLIPSIS? )
                           ;

optionallyNamedValueDescription     : (namedValueDescription)=>namedValueDescription
                                      -> ^( MEMBERSHIP_CLAIM namedValueDescription ) 
                                    | m4_value_description_production
                                    ;

namedValueDescription   : memberNameExpr ':'! valueDescriptionExpr
                        ;

/* DWARF version */
dwarfStructuredValueDescription :  KEYWORD_OBJECT '{' dwarfNamedElement* '}'
                                -> ^(KEYWORD_OBJECT dwarfNamedElement* )
                            ;

/*********************************************************
 * literal values and compile-time constant expressions  *
 * -- could be useful in DWARF syntax? Or just for Cake? *
 * -- certainly useful in Dwarf Predicates....           *
 *********************************************************/

patternConstantValueDescription : STRING_LIT^
                                | INT ELLIPSIS^ INT
                                ;

constantOrVoidValueDescription  : constantValueDescription^
                                | KEYWORD_VOID^
                                ;

constantValueDescription    : STRING_LIT^ /* FIXME: need IDENTS here too? depends what we use these for */
                            | KEYWORD_NULL^
                            | constantSetExpression
                            | constantIntegerArithmeticExpression 
                                -> ^( CONST_ARITH constantIntegerArithmeticExpression )
                            ;
                            
/* FIXME: lists are like these but without the "set" keyword */
constantSetExpression   : KEYWORD_SET '[' ( constantValueDescription ( ',' constantValueDescription* )* )? ']' -> ^( SET_CONST constantValueDescription* )
                        ;
constantIntegerArithmeticExpression	: constantShiftingExpression^
									;
/* FIXME: rather than repeating these rules, just use stub expressions
 * and push the compile-time-const detection to semantic analysis
 * (where it has to be anyway, because we use memberNameExpr which
 * might actually be referencing non-compile-time-const values). */

primitiveIntegerArithmeticExpression	: INT^
                                        | memberNameExpr /* HACK: so we can use this rule in array suffix */
                                        | '('! constantIntegerArithmeticExpression^ ')'!
                                        ;

constantUnaryOperatorExpression	: (MINUS^|PLUS^)* primitiveIntegerArithmeticExpression
                        ;

constantMultiplicativeOperatorExpression  : constantUnaryOperatorExpression ( ( MULTIPLY^ | DIVIDE^ | MODULO^ ) constantUnaryOperatorExpression )*
                                          ;
constantAdditiveOperatorExpression : constantMultiplicativeOperatorExpression ( ( PLUS^ | MINUS^ ) constantMultiplicativeOperatorExpression )*
                                   ;
constantShiftingExpression : constantAdditiveOperatorExpression ( ( SHIFT_LEFT^ | SHIFT_RIGHT^ ) constantAdditiveOperatorExpression )* 
                           ;


