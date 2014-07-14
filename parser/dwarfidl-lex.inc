/* Fallback (interesting) tokens */
INT :   ('1'..'9''0'..'9'*)|'0''x'('0'..'9'|'a'..'f'|'A'..'F')+|'0';
FLOAT : '0'..'9'+ '.''0'..'9'+ ;
NEWLINE:'\r'? '\n' {antlr_m4_newline_action} ;
WS  :   (' '|'\t')+ {antlr_m4_skip_action} ;
LINECOMMENT : '/' '/'( ~ '\n' )* {antlr_m4_skip_action} ;
BLOCKCOMMENT : '/' '*' ( ~ '/' | ( ~ '*' ) '/' )* '*' '/' {antlr_m4_skip_action} ;
STRING_LIT : '\"' ( ~'\"'|'\\\"' )* '\"' ;
//IDENT  :   ('a'..'z'|'A'..'Z'|'_''a'..'z'|'_''A'..'Z'|'_''0'..'9'|'\\'.) /* begin with a-zA-Z or non-terminating '_' */
//(
//	('a'..'z'|'A'..'Z'|'0'..'9'|'\\'.|'_'|'-'|('.'('0'..'9')+))*('a'..'z'|'A'..'Z'|'0'..'9'|'\\'.|'_')
//   /*|('.''0'..'9') /* ending with dot-digit is okay */
//)? ;
/* FIXME: scrapped the fancy IDENT rule until antlr does maximum munch. GRR! */
IDENT : ('\\'.|'a'..'z'|'A'..'Z'|'_'+'a'..'z'|'_'+'A'..'Z'|'_'+'0'..'9')('a'..'z'|'A'..'Z'|'0'..'9'|'_'|'\\'.)*;
PATTERN_IDENT : '/'('a'..'z'|'A'..'Z'|'_''a'..'z'|'_''A'..'Z'|'_''0'..'9')('a'..'z'|'A'..'Z'|'0'..'9'|'_'|'|'|'*'|'('|')'|'.'|'?')*'/';
METAVAR	: '@'('a'..'z'|'A'..'Z')('a'..'z'|'A'..'Z'|'0'..'9'|'_')* ;
/* The ident rule is a bit different from the conventional -- idents must be at
 * least two characters, and may embed '-' characters, and '.' preceding a number.
 * (but not at the start or end). 
 * The first of these quirks reduces ambiguity, since '_' is given a unique and special
 * meaning. On the other hand, symbols which are only one character will cause problems.
 * The second quirk is really odd, but I'm running with it so that we can use library names
 * in a natural fashion, e.g. glib-2.0 and so on. Note that the ambiguity is less than
 * you think, in the common case that the dots fall in between digits, since a digit
 * can't begin an ident anyway.
 * FIXME: at the moment, to support the allow-digits-after-dots rule, 
 * we require extra spaces in a name.name.name expression. Fix ANTLR's lexing 
 * behaviour so that we don't need this (i.e. that blah.blah.blah works as expected). */
/* FIXME: semantic action to process backslash-escapes within IDENT? */
