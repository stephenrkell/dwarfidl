#ifndef DWARFIDL_LANG_HPP_
#define DWARFIDL_LANG_HPP_

/* In-tree client code will define these differently, because during build
 * we haven't yet copied the generated header files into the include directory. */
#ifndef LEXER_INCLUDE
#define LEXER_INCLUDE "dwarfidl/dwarfidlNewCLexer.h"
#endif
#ifndef PARSER_INCLUDE
#define PARSER_INCLUDE "dwarfidl/dwarfidlNewCParser.h"
#endif

#include <antlr3cxx/parser.hpp>

#include <string>

namespace dwarfidl
{
	using std::string;

	string unescape_ident(const string& s);
	string unescape_string_lit(const std::string& lit);
}

#endif
