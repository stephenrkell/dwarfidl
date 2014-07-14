#include "dwarfidl/create.hpp"

using namespace dwarf;
using namespace dwarf::core;

using std::cout;

namespace dwarfidl
{
	void create_one_die(const iterator_base& parent, antlr::tree::Tree *d)
	{
		INIT;
		BIND2(d, tag_keyword);
		BIND3(d, attrs, ATTRS);
		BIND3(d, children, CHILDREN);
		
		const char *tagstr = CCP(GET_TEXT(tag_keyword));
		
		Dwarf_Half tag = spec::DEFAULT_DWARF_SPEC.tag_for_name((string("DW_TAG_") + tagstr).c_str());
		
		auto created = parent.get_root().make_new(parent, tag);
		
		/* FIXME: create attributes */
	}

	void find_or_create_die(const iterator_base& parent, antlr::tree::Tree *ast)
	{
		create_one_die(parent, ast);
	}
	
	void create_dies(const iterator_base& parent, antlr::tree::Tree *ast)
	{
		/* Walk the tree. Create any DIE we see. We also have to
		 * scan attrs and create any that are inlined and do not
		 * already exist. */
		cout << "Got AST: " << CCP(TO_STRING_TREE(ast)) << endl;
		FOR_ALL_CHILDREN(ast)
		{
			cout << "Got a node: " << CCP(TO_STRING_TREE(n)) << endl;
			SELECT_ONLY(DIE);
			antlr::tree::Tree *die = n;
			
			/* scan for nested non-child DIEs */
			{
				INIT;
				BIND2(n, tag_keyword);
				BIND3(n, attrs, ATTRS);
				BIND3(n, children, CHILDREN);
				FOR_ALL_CHILDREN(attrs)
				{
					INIT;
					BIND2(n, attr);
					BIND2(n, value);

					switch (GET_TYPE(value))
					{
						case TOKEN(DIE):
							/* It's an inline DIE: we need to find a matching DIE or create it.
							 * If we create it, create it as a sibling */
							find_or_create_die(parent, value);
							break;
						// deal with the other cases later
						// case TOKEN(IDENT):
						default: break;
					}
				}
			}
			
			cout << "Creating a DIE from " << CCP(TO_STRING_TREE(die)) << endl;
			
			create_one_die(parent, n);
		}
	}
}
