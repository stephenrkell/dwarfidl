#include "dwarfidl/create.hpp"
#include <boost/algorithm/string/case_conv.hpp>

using namespace dwarf;
using namespace dwarf::core;
using encap::attribute_value;

using std::cerr;
using boost::to_lower_copy;

namespace dwarfidl
{
	attribute_value make_attribute_value(antlr::tree::Tree *d, 
		const iterator_base& context, 
		Dwarf_Half attr,
		const std::map<antlr::tree::Tree *, iterator_base>& nested)
	{
		switch (GET_TYPE(d))
		{
			case TOKEN(DIE): {
				/* nested DIE; we should already have created it */
				auto found = nested.find(d);
				assert(found != nested.end());
				
				return attribute_value(attribute_value::weak_ref(found->second.get_root(), 
					found->second.offset_here(), false, 
					context.offset_here(), attr));
			} break;
			case TOKEN(IDENT): {
				if (attr != DW_AT_name)
				{
					/* unless we're naming something, resolve this ident */
					std::vector<string> name(1, CCP(GET_TEXT(d)));
					auto found = context.root().scoped_resolve(context,
						name.begin(), name.end());
					assert(found);
					return attribute_value(attribute_value::weak_ref(found.get_root(), 
						found.offset_here(), false, 
						context.offset_here(), attr));
					assert(found);
				}
				else
				{
					string name = CCP(GET_TEXT(d));
					return attribute_value(name);
				}
			} break;
			case TOKEN(INT): {
				std::istringstream is(CCP(GET_TEXT(d)));
				int i;
				is >> i;
				return attribute_value(static_cast<Dwarf_Unsigned>(i));
			} break;
			case TOKEN(STRING_LIT): {
				//return attribute
			} break;
			default:  // FIXME: support more
				assert(false);
		}
	}

	iterator_base create_one_die(const iterator_base& parent, antlr::tree::Tree *d,
		const std::map<antlr::tree::Tree *, iterator_base>& nested /* = ... */)
	{
		INIT;
		BIND2(d, tag_keyword);
		BIND3(d, attrs, ATTRS);
		BIND3(d, children, CHILDREN);
		
		const char *tagstr = CCP(GET_TEXT(tag_keyword));
		
		Dwarf_Half tag = spec::DEFAULT_DWARF_SPEC.tag_for_name((string("DW_TAG_") + tagstr).c_str());
		
		auto created = parent.get_root().make_new(parent, tag);
		
		/* create attributes */
		FOR_ALL_CHILDREN(attrs)
		{
			INIT;
			BIND2(n, attr);
			BIND2(n, value);
			
			string attrstr = CCP(TO_STRING(attr));
			/*
			 * HACK HACK HACK: 
			 * 
			 * antlr is STUPID and doesn't let us get the actual token text
			 * that generated an AST node via a rule of the form
			 
			       nameClause : IDENT 
			           -> ^( ATTR 'name' IDENT )
			 
			 * ... instead, it helpfully fills in a string representation of the 
			 * token number for 'name', e.g. "160". This is never what anyone wants.
			 * 
			 * My workaround for now is to define imaginary tokens NAME and TYPE, 
			 * which means instead of "160" we get "NAME" etc., 
			 * and then to_lower() on the string.
			 */
			
			Dwarf_Half attrnum = created.spec_here().attr_for_name(("DW_AT_" + to_lower_copy(attrstr)).c_str());
			encap::attribute_value v = make_attribute_value(value, created, attrnum, nested);
			
			dynamic_cast<core::in_memory_abstract_die&>(created.dereference())
				.attrs(opt<root_die&>())
					.insert(make_pair(attrnum, v));
		}
		
		return created;
	}
	
	iterator_base create_one_die_with_children(const iterator_base& parent, antlr::tree::Tree *d)
	{
		cerr << "Creating a DIE from " << CCP(TO_STRING_TREE(d)) << endl;

		INIT;
		BIND2(d, tag_keyword);
		BIND3(d, attrs, ATTRS);
		BIND3(d, children, CHILDREN);
		map<antlr::tree::Tree *, iterator_base> nested;
		/* scan for nested non-child DIEs */
		{
			FOR_ALL_CHILDREN(attrs)
			{
				INIT;
				BIND2(n, attr);
				BIND2(n, value);

				switch (GET_TYPE(value))
				{
					case TOKEN(DIE): {
						/* It's an inline DIE: we need to find a matching DIE or create it.
						 * If we create it, create it as a sibling */
						iterator_base sub_die = find_or_create_die(parent, value);
						nested[value] = sub_die;
					} break;
					case TOKEN(IDENT): {
						/* If we're an ident, then either we're giving a new name to a thing, 
						 * or we're referencing an existing thingby name . We only care about 
						 * the latter case. */
						if (to_lower_copy(string(CCP(GET_TEXT(attr)))) == "name") break;
					
						vector<string> name(1, string(CCP(GET_TEXT(value))));
						iterator_base found = parent.root().scoped_resolve(parent, 
							name.begin(), name.end());
						if (!found) goto name_lookup_error;
						nested[value] = found;
						break;
					}
					name_lookup_error: 
						cerr << "Could not resolve name " << CCP(TO_STRING_TREE(value)) << endl;
						break;
					default:
						cerr << "Subtree " << CCP(TO_STRING_TREE(value)) 
							<< " is not a nested DIE" << endl;
						break;
				}
			}
		}

		auto created = create_one_die(parent, d, nested);
		
		FOR_ALL_CHILDREN(children)
		{
			create_one_die_with_children(created, n);
		}
		
		return created;
	}

	iterator_base find_or_create_die(const iterator_base& parent, antlr::tree::Tree *ast)
	{
		/* Sometimes we can search for an existing DIE. But currently we blindly
		 * re-create DIEs that might already be existing. */
		// auto found = parent.root().scoped_resolve(
		return create_one_die_with_children(parent, ast);
	}
	
	iterator_base create_dies(const iterator_base& parent, antlr::tree::Tree *ast)
	{
		/* Walk the tree. Create any DIE we see. We also have to
		 * scan attrs and create any that are inlined and do not
		 * already exist. */
		cerr << "Got AST: " << CCP(TO_STRING_TREE(ast)) << endl;
		iterator_base first_created;
		FOR_ALL_CHILDREN(ast)
		{
			cerr << "Got a node: " << CCP(TO_STRING_TREE(n)) << endl;
			SELECT_ONLY(DIE);
			antlr::tree::Tree *die = n;
			
			auto created = create_one_die_with_children(parent, n);
			if (!first_created) first_created = created;
			
			if (getenv("DEBUG_CC")) cerr << "Created one DIE and its children; we now have: " << endl << parent.root();
		}
		return first_created;
	}
	iterator_base create_dies(const iterator_base& parent, const string& some_dwarfidl)
	{
		
		/* Also open a dwarfidl file, read some DIE definitions from it. */
		auto str = antlr3StringStreamNew(
			const_cast<unsigned char *>(reinterpret_cast<const unsigned char *>(some_dwarfidl.c_str())), 
			ANTLR3_ENC_8BIT,
			strlen(some_dwarfidl.c_str()),
			const_cast<unsigned char *>(reinterpret_cast<const unsigned char *>("blah"))
		);
		assert(str);
		auto lexer = dwarfidlSimpleCLexerNew(str);
		auto tokenStream = antlr3CommonTokenStreamSourceNew(ANTLR3_SIZE_HINT, TOKENSOURCE(lexer));
		auto parser = dwarfidlSimpleCParserNew(tokenStream);
		dwarfidlSimpleCParser_toplevel_return ret = parser->toplevel(parser);
		antlr::tree::Tree *tree = ret.tree;

		iterator_base first_created = create_dies(parent, tree);

		if (getenv("DEBUG_CC")) cerr << "Created some more stuff; whole tree is now: " << endl << parent.get_root();
		
		return first_created;
	}
}
