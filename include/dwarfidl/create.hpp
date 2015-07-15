/* Code to create DIEs from ASTs. */
#ifndef DWARFIDL_CREATE_HPP_
#define DWARFIDL_CREATE_HPP_

#include "dwarfidl/lang.hpp"
#include <dwarfpp/lib.hpp>
#include <vector>
#include <map>

namespace dwarfidl
{
	using namespace dwarf;
	using namespace dwarf::core;

	iterator_base create_dies(antlr::tree::Tree *ast);
	iterator_base create_dies(iterator_base& parent, antlr::tree::Tree *ast);
	iterator_base create_dies(iterator_base& parent, const string& some_dwarfidl);
	iterator_base create_one_die(iterator_base& parent, antlr::tree::Tree *ast, 
								 std::vector<std::pair<iterator_base&, antlr::tree::Tree*> > &postpone,
								 const std::map<antlr::tree::Tree *, iterator_base>& nested
								 = std::map<antlr::tree::Tree *, iterator_base>());
	 iterator_base create_one_die_with_children(iterator_base& parent, antlr::tree::Tree *ast,
												std::vector<std::pair<iterator_base&, antlr::tree::Tree*> > &postpone);
	 iterator_base find_or_create_die(iterator_base& parent, antlr::tree::Tree *ast,
									  std::vector<std::pair<iterator_base&, antlr::tree::Tree*> > &postpone);

	encap::attribute_value make_attribute_value(antlr::tree::Tree *d, 
		const iterator_base& context, 
		Dwarf_Half attr,
		const std::map<antlr::tree::Tree *, iterator_base>& nested);
}



#endif
