#include "dwarfidl/create.hpp"
#include "dwarfprint.hpp"
#include <boost/algorithm/string/case_conv.hpp>
#include <string>
#include <sstream>
#include <vector>
#include <exception>

using namespace dwarf;
using namespace dwarf::core;
using encap::attribute_value;
using encap::loc_expr;
using dwarf::spec::DEFAULT_DWARF_SPEC;

using std::cerr;
using std::string;
using std::ostringstream;
using std::vector;
using std::pair;
using std::exception;
using std::make_pair;
using boost::to_lower_copy;
using antlr::tree::Tree;


class ident_not_found : public exception 
{
public:
	 string ident;
	 
	 ident_not_found(string ident) : ident(ident) {}

	 virtual const char *what() const throw() {
		  return ident.c_str();
	 }
};
	 

template <typename T> inline T node_text_to(Tree *node) {
	 // FIXME, brittle hex detection etc?
	 
	 string text = CCP(GET_TEXT(node));
	 bool is_hex = (text.substr(0, 2).compare(string("0x")) == 0);
	 if (is_hex) {
		  text = text.substr(2, string::npos);
	 }
	 std::istringstream is(text);
	 if (is_hex) {
		  is >> std::hex;
	 } else {
		  is >> std::dec;
	 }
	 T result;
	 is >> result;
	 return result;
}

namespace dwarfidl
{
	attribute_value make_attribute_value(Tree *d, 
		const iterator_base& context, 
		Dwarf_Half attr,
		const std::map<Tree *, iterator_base>& nested)
	{
		switch (GET_TYPE(d)) {
		case TOKEN(DIE): {
			/* nested DIE; we should already have created it */
			auto found = nested.find(d);
			assert(found != nested.end());
			
			return attribute_value(attribute_value::weak_ref(found->second.get_root(), 
					found->second.offset_here(), false, 
					context.offset_here(), attr));
		} break;
		case TOKEN(IDENTS): {
			ostringstream os;
			bool first = true;
			FOR_ALL_CHILDREN(d) {
				if (first) first = false;
				else os << " ";
				os << CCP(GET_TEXT(n));
			}
			string identifier = os.str();
			
			if (attr != DW_AT_name) {
				/* unless we're naming something, resolve this ident */
				std::vector<string> name(1, identifier);//unescape_ident(CCP(GET_TEXT(d))));
				auto found = context.root().scoped_resolve(context,
					name.begin(), name.end());
				if (!found || found.tag_here() == 0 || found.offset_here() == 0) {
					 throw ident_not_found(identifier);
				}				
				auto val = attribute_value(attribute_value::weak_ref(found.get_root(), 
						found.offset_here(), true, 
						context.offset_here(), attr));
				return val;
			}
			else {
				//string name = unescape_ident(CCP(GET_TEXT(d)));
				return attribute_value(identifier);
			}
		} break;
		case TOKEN(INT): {
			return attribute_value(static_cast<Dwarf_Unsigned>(node_text_to<unsigned int>(d)));
		} break;
		case TOKEN(ABSOLUTE_OFFSET): {
			INIT;
			BIND2(d, addr_node);
			unsigned int off = node_text_to<unsigned int>(addr_node);
			assert(off != 0);
			return attribute_value(attribute_value::weak_ref(context.get_root(), off, true, context.offset_here(), attr));
		} break;
		case TOKEN(STRING_LIT): {
			string text = string(CCP(GET_TEXT(d)));
			return attribute_value(text);
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
	 
	 iterator_base create_one_die(iterator_base& parent, Tree *d,
								  vector<pair<iterator_base&, Tree*> > &postpone,
								  const std::map<Tree *, iterator_base>& nested /* = ... */)
	 {
		INIT;
		BIND2(d, tag_keyword);
		BIND3(d, attrs, ATTRS);
		BIND3(d, children, CHILDREN);
		
		const char *tagstr = CCP(GET_TEXT(tag_keyword));
		
		Dwarf_Half tag = DEFAULT_DWARF_SPEC.tag_for_name((string("DW_TAG_") + tagstr).c_str());
		
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
			try {
				 encap::attribute_value v = make_attribute_value(value, created, attrnum, nested);
				 // FIXME HACK
				 if (attrnum == DW_AT_type) {
					  if (v.is_address()) {
						   cerr << "checking addr != 0..." << endl;
						   assert(v.get_address().addr != 0);
					  }
					  if (v.is_ref()) {
						   cerr << "checking ref != 0..." << endl;
						   assert(v.get_ref().off != 0);
					  }
				 }
				 
				 
				 dynamic_cast<core::in_memory_abstract_die&>(created.dereference())
					  .attrs(opt<root_die&>())
					  .insert(make_pair(attrnum, v));
			
			} catch (ident_not_found const &e) {
				 cerr << "Ident not found: '" << e.what() << "', postponing to next pass" << endl;
				 postpone.push_back(pair<iterator_base&, Tree*>(parent, d));
				 return parent.root().end();
			}
		}
		
			

		
		if (getenv("DEBUG_CC")) {
			 cerr << "Created DIE: ";
			 created.print_with_attrs(cerr);
			 //print_type_die(cerr, created);
			 cerr << endl;
		}
		return created;
	}
	
	 iterator_base create_one_die_with_children(iterator_base& parent,
												Tree *d,
												vector<pair<iterator_base&, Tree*> > &postpone)
	{
		cerr << "Creating a DIE from " << CCP(TO_STRING_TREE(d)) << endl;

		INIT;
		BIND2(d, tag_keyword);
		BIND3(d, attrs, ATTRS);
		BIND3(d, children, CHILDREN);
		map<Tree *, iterator_base> nested;
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
						 iterator_base sub_die = find_or_create_die(parent, value, postpone);
						nested[value] = sub_die;
					} break;
					case TOKEN(IDENT): {
						/* If we're an ident, then either we're giving a new name to a thing, 
						 * or we're referencing an existing thing by name . We only care about 
						 * the latter case. */
						if (to_lower_copy(string(CCP(GET_TEXT(attr)))) == "name") break;
					
						vector<string> name(1, string(unescape_ident(CCP(GET_TEXT(value)))));
						iterator_base found = parent.root().scoped_resolve(parent, 
							name.begin(), name.end());
						if (!found) {
							 cerr << "Could not resolve name " << CCP(TO_STRING_TREE(value)) << ", postponing to next pass" << endl;
							 postpone.push_back(pair<iterator_base&, Tree*>(parent, d));
							 return parent.root().end();
						}
						nested[value] = found;
						break;
					}
					default:
						cerr << "Subtree " << CCP(TO_STRING_TREE(value)) 
							<< " is not a nested DIE" << endl;
						break;
				}
			}
		}

		auto created = create_one_die(parent, d, postpone, nested);
		
		FOR_ALL_CHILDREN(children)
		{
			 create_one_die_with_children(created, n, postpone);
		}
		
		return created;
	}

	 iterator_base find_or_create_die(iterator_base& parent, Tree *ast, std::vector<std::pair<iterator_base&, antlr::tree::Tree*> > &postpone)
	{
		/* Sometimes we can search for an existing DIE. But currently we blindly
		 * re-create DIEs that might already be existing. */
		// auto found = parent.root().scoped_resolve(
		 return create_one_die_with_children(parent, ast, postpone);
	}

	iterator_base create_dies(Tree *ast) {
		in_memory_root_die *root = new in_memory_root_die;
		auto iter = root->begin();
		create_dies(iter, ast);
		return iter;
	}


	iterator_base create_dies(iterator_base& parent, Tree *ast)
	{
		/* Walk the tree. Create any DIE we see. We also have to
		 * scan attrs and create any that are inlined and do not
		 * already exist. */
		cerr << "Got AST: " << CCP(TO_STRING_TREE(ast)) << endl;
		iterator_base first_created;

		auto dummy_cu = parent.get_root().make_new(parent, DW_TAG_compile_unit);
		vector<pair<iterator_base&, Tree*> > postpone;
		
		// Initial pass: go straight from AST
		FOR_ALL_CHILDREN(ast)
		{
			cerr << "Got a node: " << CCP(TO_STRING_TREE(n)) << endl;
			SELECT_ONLY(DIE);
			INIT;
			BIND2(n, tag_keyword);
			if (string(CCP(GET_TEXT(tag_keyword))).compare("compile_unit") == 0) {
				// We are a compilation unit, continue
				 auto created = create_one_die_with_children(parent, n, postpone);
				if (!first_created) first_created = created;
			} else {
				// Not a compilation unit, need to add to the dummy one because
				// having one is expected by lots of libdwarfpp
				 auto created = create_one_die_with_children(dummy_cu, n, postpone);
				if (!first_created) first_created = created;
			}
			if (getenv("DEBUG_CC")) cerr << "Created one DIE and its children; we now have: " << endl << parent.root();
		}

		// postpone now contains pairs of (parent, parse tree) for
		// DIEs which referenced an unresolved ident.
		//
		// Loop through this until we have resolved everything
		// or failed to do so
		int postponed_pass_n = 0;
		while (postpone.size() > 0) {
			 cerr << "=== POSTPONED PASS " << ++postponed_pass_n << " ===" << endl;
			 
			 auto old_size = postpone.size();
			 auto old_postpone = postpone;
			 postpone.clear();
			 for (auto iter = old_postpone.begin(); iter != old_postpone.end(); iter++) {
				  auto postponed_parent = iter->first;
				  auto postponed_tree = iter->second;
				  create_one_die_with_children(postponed_parent, postponed_tree, postpone);
			 }
			 auto new_size = postpone.size();
			 assert(old_size > new_size);
		}
		
		return first_created;
	}
	
	iterator_base create_dies(iterator_base& parent, const string& some_dwarfidl)
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
		Tree *tree = ret.tree;

		iterator_base first_created = create_dies(parent, tree);

		// if (getenv("DEBUG_CC")) cerr << "Created some more stuff; whole tree is now: " << endl << parent.get_root();
		
		return first_created;
	}
}
