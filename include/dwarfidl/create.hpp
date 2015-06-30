/* Code to create DIEs from ASTs. */
#ifndef DWARFIDL_CREATE_HPP_
#define DWARFIDL_CREATE_HPP_

#include "dwarfidl/lang.hpp"
#include <dwarfpp/lib.hpp>

namespace dwarfidl
{
	using namespace dwarf;
	using namespace dwarf::core;

	iterator_base create_dies(antlr::tree::Tree *ast);
	iterator_base create_dies(const iterator_base& parent, antlr::tree::Tree *ast);
	iterator_base create_dies(const iterator_base& parent, const string& some_dwarfidl);
	iterator_base create_one_die(const iterator_base& parent, antlr::tree::Tree *ast, 
		const std::map<antlr::tree::Tree *, iterator_base>& nested
		 = std::map<antlr::tree::Tree *, iterator_base>()
	);
	iterator_base create_one_die_with_children(const iterator_base& parent, antlr::tree::Tree *ast);
	iterator_base find_or_create_die(const iterator_base& parent, antlr::tree::Tree *ast);

	encap::attribute_value make_attribute_value(antlr::tree::Tree *d, 
		const iterator_base& context, 
		Dwarf_Half attr,
		const std::map<antlr::tree::Tree *, iterator_base>& nested);
}



#endif
