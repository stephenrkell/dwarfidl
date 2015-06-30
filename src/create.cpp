#include "dwarfidl/create.hpp"
#include <boost/algorithm/string/case_conv.hpp>

using namespace dwarf;
using namespace dwarf::core;
using encap::attribute_value;
using encap::loc_expr;
using spec::DEFAULT_DWARF_SPEC;

using std::cerr;
using boost::to_lower_copy;

template <typename T> inline T node_text_to(antlr::tree::Tree *node) {
	std::istringstream is(CCP(GET_TEXT(node)));
	T result;
	is >> result;
	return result;
}

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
					std::vector<string> name(1, unescape_ident(CCP(GET_TEXT(d))));
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
					string name = unescape_ident(CCP(GET_TEXT(d)));
					return attribute_value(name);
				}
			} break;
			case TOKEN(INT): {
				return attribute_value(static_cast<Dwarf_Unsigned>(node_text_to<unsigned int>(d)));
			} break;
			case TOKEN(STRING_LIT): {
				assert(false);
			} break;
		case TOKEN(KEYWORD_TRUE): {
			return attribute_value(static_cast<Dwarf_Bool>(1));
		} break;
		case TOKEN(KEYWORD_FALSE): {
			return attribute_value(static_cast<Dwarf_Bool>(0));
		} break;
		case TOKEN(OPCODE_LIST): {
			loc_expr *expr = new loc_expr;
			FOR_ALL_CHILDREN(d) {
				assert(GET_TYPE(n) == TOKEN(OPCODE));
				INIT;
				auto child_count = GET_CHILD_COUNT(n);
				Dwarf_Loc op;
				assert (child_count >= 1 && child_count <= 3);
				if (child_count >= 1) {
					BIND2(n, opcode);
					auto opcode_str = string("DW_OP_") + string(CCP(GET_TEXT(opcode)));
					op.lr_atom = DEFAULT_DWARF_SPEC.op_for_name(opcode_str.c_str());
					if (child_count >= 2) {
						BIND2(n, arg1);
						op.lr_number = node_text_to<unsigned int>(arg1);
						if (child_count >= 3) {
							BIND2(n, arg2);
							op.lr_number2 = node_text_to<unsigned int>(arg2);
						}
					}
				}
				expr->push_back(op);
			}
			return attribute_value(expr);
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
		
		if (getenv("DEBUG_CC")) { cerr << "Created DIE: ";
			created.print_with_attrs(cerr);
			cerr << endl; }
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
					
						vector<string> name(1, string(unescape_ident(CCP(GET_TEXT(value)))));
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

	iterator_base create_dies(antlr::tree::Tree *ast) {
		root_die *root = new root_die;
		auto iter = root->begin();
		create_dies(iter, ast);
		return iter;
	}
	
	iterator_base create_dies(const iterator_base& parent, antlr::tree::Tree *ast)
	{
		/* Walk the tree. Create any DIE we see. We also have to
		 * scan attrs and create any that are inlined and do not
		 * already exist. */
		cerr << "Got AST: " << CCP(TO_STRING_TREE(ast)) << endl;
		iterator_base first_created;

		auto dummy_cu = parent.get_root().make_new(parent, DW_TAG_compile_unit);
		//		set<antlr::tree::Tree*> non_compile_unit_dies;
		FOR_ALL_CHILDREN(ast)
		{
			cerr << "Got a node: " << CCP(TO_STRING_TREE(n)) << endl;
			SELECT_ONLY(DIE);
			antlr::tree::Tree *die = n;
			INIT;
			BIND2(n, tag_keyword);
			if (string(CCP(GET_TEXT(tag_keyword))).compare("compile_unit") == 0) {
				// We are a compilation unit, continue
				auto created = create_one_die_with_children(parent, n);
				if (!first_created) first_created = created;
			} else {
				// Not a compilation unit, need to add to the dummy one because
				// having one is expected by lots of libdwarfpp
				//non_compile_unit_dies.insert(n);
				auto created = create_one_die_with_children(dummy_cu, n);
				if (!first_created) first_created = created;
			}
			if (getenv("DEBUG_CC")) cerr << "Created one DIE and its children; we now have: " << endl << parent.root();
		}

		
		
		// non_compile_unit_dies is now populated
		// for (auto iter = non_compile_unit_dies.begin(); iter != non_compile_unit_dies.end(); iter++) {
		// 	auto created = create_one_die_with_children(dummy_cu, *iter);
		// 	if (!first_created) first_created = created;
		// }

		
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

		// if (getenv("DEBUG_CC")) cerr << "Created some more stuff; whole tree is now: " << endl << parent.get_root();
		
		return first_created;
	}
}
