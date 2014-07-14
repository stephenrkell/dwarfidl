/* Code to create DIEs from ASTs. */
#ifndef DWARFIDL_CREATE_HPP_
#define DWARFIDL_CREATE_HPP_

#define LEXER_INCLUDE "dwarfidl/dwarfidlSimpleCLexer.h"
#define PARSER_INCLUDE "dwarfidl/dwarfidlSimpleCParser.h"
#include <antlr3cxx/parser.hpp>

#include <dwarfpp/lib.hpp>

namespace dwarfidl
{
	using namespace dwarf;
	using namespace dwarf::core;
	
	void create_dies(const iterator_base& parent, antlr::tree::Tree *ast);
	void create_one_die(const iterator_base& parent, antlr::tree::Tree *ast);
}



#endif
