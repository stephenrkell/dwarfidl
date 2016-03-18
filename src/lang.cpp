/* Because we're in-tree, when we're built we haven't yet copied 
 * the generated header files into the include directory. So the 
 * include path is a bit different. */
#ifndef LEXER_INCLUDE
#define LEXER_INCLUDE "dwarfidlNewCLexer.h"
#endif
#ifndef PARSER_INCLUDE
#define PARSER_INCLUDE "dwarfidlNewCParser.h"
#endif
#include "dwarfidl/lang.hpp"

using std::string;

namespace dwarfidl
{
	std::string unescape_ident(const std::string& ident)
	{
		std::ostringstream o;
		enum { BEGIN, ESCAPE } state = BEGIN;

		for (auto i = ident.begin(); i != ident.end(); i++)
		{
			switch(state)
			{
				case BEGIN:
					switch (*i)
					{
						case '\\':
							state = ESCAPE;
							break;
						default: 
							o << *i;
							break;
					}
					break;
				case ESCAPE:
					o << *i;
					state = BEGIN;
					break;
				default: break; // illegal state
			}
		}
		return o.str();
	}

	std::string unescape_string_lit(const std::string& lit)
	{
		if (lit.length() < 2 || *lit.begin() != '"' || *(lit.end() - 1) != '"')
		{
			// string is not quoted, so just return it
			return lit;
		}

		std::ostringstream o;
		enum state { BEGIN, ESCAPE, OCTAL, HEX } state = BEGIN;
		std::string::const_iterator octal_begin;
		std::string::const_iterator octal_end;
		std::string::const_iterator hex_begin;
		std::string::const_iterator hex_end;

		for (std::string::const_iterator i = lit.begin() + 1; i < lit.end() - 1; i++)
		{
			// HACK: end-of-string also terminates oct/hex escape sequences
			if (state == HEX && i + 1 == lit.end()) { hex_end = i + 1; goto end_hex; }
			if (state == OCTAL && i + 1 == lit.end()) { octal_end = i + 1; goto end_oct; }
			switch(state)
			{
				case BEGIN:
					switch (*i)
					{
						case '\\':
							state = ESCAPE;
							break;
						case '\"':
							/* We have an embedded '"', which shouldn't happen... so
							 * we're free to do what we like. Let's skip over it silently. */						
							break;
						default: 
							o << *i;
							break;
					}
					break;
				case ESCAPE:
					switch (*i)
					{
						case 'n': o << '\n'; state = BEGIN; break;
						case 't': o << '\t'; state = BEGIN; break;
						case 'v': o << '\v'; state = BEGIN; break;
						case 'b': o << '\b'; state = BEGIN; break;
						case 'r': o << '\r'; state = BEGIN; break;
						case 'f': o << '\f'; state = BEGIN; break;
						case 'a': o << '\a'; state = BEGIN; break;
						case '\\': o << '\\'; state = BEGIN; break;
						case '?': o << '\?'; state = BEGIN; break;
						case '\'':o << '\''; state = BEGIN; break;
						case '\"':o << '\"'; state = BEGIN; break;
						case '0': case '1': case '2': case '3':
						case '4': case '5': case '6': 
						case '7': case '8': case '9':
							state = OCTAL;
							octal_begin = i;
							break;
						case 'x':
							state = HEX;
							hex_begin = i + 1;
							break;
						default:
							// illegal escape sequence -- ignore this
							break;
					}
					break;
				case OCTAL:
					switch (*i)
					{
						case '0': case '1': case '2': case '3':
						case '4': case '5': case '6': 
						case '7': case '8': case '9':
							continue;
						default: 
							// this is the end of octal digits
							octal_end = i;
							end_oct:
							{ 
								std::istringstream s(std::string(octal_begin, octal_end));
								char char_val;
								s >> std::oct >> char_val;
								o << char_val;
							}
							if (octal_end == i) o << *i;
							state = BEGIN;
							break;
					}
					break;
				case HEX:
					switch (*i)
					{
						case '0': case '1': case '2': case '3':
						case '4': case '5': case '6': case '7': 
						case '8': case '9': case 'a': case 'A':
						case 'b': case 'B': case 'c': case 'C':
						case 'd': case 'D': case 'e': case 'E': case 'f': case 'F':
							continue;
						default: 
							// end of hex digits
							hex_end = i;
							end_hex:
							{ 
								std::istringstream s(std::string(hex_begin, hex_end));
								char char_val;
								s >> std::hex >> char_val;
								o << char_val;
							}
							if (hex_end == i) o << *i;
							state = BEGIN;
							break;
					}
					break;					
				default: break; // illegal state
			}
		}
		return o.str();
	}

}
