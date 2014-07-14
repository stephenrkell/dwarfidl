#include "dwarfidl/create.hpp"

using namespace dwarf;
using namespace dwarf::core;

namespace dwarfidl
{
	void create_one_die(const iterator_base& parent, antlr::tree::Tree *ast)
	{
		INIT;
		BIND2(ast, tag_keyword);
		BIND3(ast, attrs, ATTRS);
		BIND3(ast, children, CHILDREN);
		
		const char *tagstr = CCP(GET_TEXT(tag_keyword));
		
		Dwarf_Half tag = spec::DEFAULT_DWARF_SPEC.tag_for_name((string("DW_TAG_") + tagstr).c_str());
		
		auto created = parent.get_root().make_new(parent, tag);
		
		/* FIXME: create attributes */
	}
	
	void create_dies(const iterator_base& parent, antlr::tree::Tree *ast)
	{
		/* Walk the tree. Create any DIE we see. We also have to
		 * scan attrs and create any that are inlined and do not
		 * already exist. */
		FOR_ALL_CHILDREN(ast)
		{
			SELECT_ONLY(DIE);
			
			/* FIXME: scan for nested non-child DIEs */
			
			create_one_die(parent, n);
		}
	}
}
