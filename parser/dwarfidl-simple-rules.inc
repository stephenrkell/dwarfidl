/* These are new rules for a "simple" minimalist version of the dwarfidl
 * language. Eventually I will merge them with the Cake-style language, 
 * perhaps with some tweaks or simplifications to that. */

/* Simple test case: 

      structure_type { member : baz; member : array_type [type = blah] {} };

 */

toplevel : dieDecl*
        -> ^( DIES dieDecl* )
        ;

dieDecl     :  diePhrase SEMICOLON
                -> diePhrase
            ;

diePhrase   :  dieLabel? tagKeyword nameClause? typeClause? attrsClause? dieChildContent? 
            -> ^( DIE tagKeyword ^( ATTRS nameClause? typeClause? attrsClause? ) ^( CHILDREN dieChildContent? ) dieLabel? )
            ;

dieLabel    : '@' IDENT ':'
                -> ^( LABEL IDENT )
            ;

/* We can specify DIE content with explicit attributes
   and/or with special syntax for "name" and "type" attrs. */

nameClause : IDENT 
                -> ^( ATTR 'name' IDENT )
            ;

typeClause : ':' dieRef 
                -> ^( ATTR 'type' dieRef )
            ;

dieRef : IDENT
        | '(' diePhrase ')'
            -> diePhrase
        ;

attrsClause : '[' attrPhraseList? ']'
                -> attrPhraseList?
            ;
            
attrPhraseList : attrPhrase ( ',' attrPhrase ) *
                -> attrPhrase*
                ;

dieChildContent : '{' ( diePhrase SEMICOLON )* '}'
                    -> diePhrase*
                ;

attrPhrase : attrKeyword '=' attrDef
                -> ^( ATTR attrKeyword attrDef )
           ;

attrDef : dieRef
        | STRING_LIT
        | INT
        | KEYWORD_TRUE
        | KEYWORD_FALSE
        ;
