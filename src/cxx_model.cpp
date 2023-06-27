/* dwarfpp: C++ binding for a useful subset of libdwarf, plus extra goodies.
 * 
 * cxx_model.cpp: tools for constructing binary-compatible C++ models 
 * 			of DWARF elements.
 *
 * Copyright (c) 2009--2012, Stephen Kell.
 */

#include "dwarfidl/cxx_model.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

using std::vector;
using std::map;
using std::string;
using std::cerr;
using std::endl;
using std::hex;
using std::dec;
using std::ostringstream;
using std::deque;
using boost::optional;
using boost::regex;
using namespace dwarf;
using dwarf::lib::Dwarf_Half;
using dwarf::lib::Dwarf_Off;
using dwarf::lib::Dwarf_Unsigned;
using namespace dwarf::core;

namespace dwarf {
namespace tool {

	string 
	cxx_generator::cxx_name_from_string(const string& s, const char *prefix)
	{
		if (is_reserved(s)) 
		{
			cerr << "Warning: generated C++ name `" << (prefix + s) 
				<< " from reserved word " << s << endl;
			return prefix + s; // being reserved implies s is lexically okay
		}
		else if (is_valid_cxx_ident(s)) return s;
		else // something is lexically illegal about s
		{
			return make_valid_cxx_ident(s);
		}
	}
	
	// static function
	bool 
	cxx_generator::is_reserved(const string& word)
	{
		return std::find(cxx_reserved_words.begin(),
			cxx_reserved_words.end(), 
			word) != cxx_reserved_words.end();
	}
	
	// static function
	bool 
	cxx_generator::is_valid_cxx_ident(const string& word)
	{
		static const regex e("[a-zA-Z_][a-zA-Z0-9_]*");
		return !is_reserved(word) &&
			std::move(regex_match(word, e));	
	}
	
	string // FIXME: I think liballocstool duplicates this?!
	cxx_generator::make_valid_cxx_ident(const string& word)
	{
		// FIXME: make this robust to illegal characters other than spaces
		string working = word;
		return is_valid_cxx_ident(word) ? word
			: (std::replace(working.begin(), working.end()-1, ' ', '_'), working);
	}
	
	string 
	cxx_generator::name_from_name_parts(const vector<string>& parts) 
	{
		ostringstream s;
		for (auto i_part = parts.begin(); i_part != parts.end(); i_part++)
		{
			if (i_part != parts.begin()) s << "::";
			s << *i_part;
		}
		return s.str();
	}
	
	bool 
	cxx_generator_from_dwarf::type_infixes_name(iterator_df<basic_die> p_d)
	{
		auto t = p_d.as_a<type_die>();
		//cerr << "Does this type infix a name? " << *t << endl;
		assert(t);
		auto unq_t = t->get_unqualified_type();
		return 
			unq_t && (
			unq_t.tag_here() == DW_TAG_subroutine_type
			||  unq_t.tag_here() == DW_TAG_array_type
			|| 
			(unq_t.tag_here() == DW_TAG_pointer_type &&
				unq_t.as_a<pointer_type_die>()->get_type()
				&& 
				unq_t.as_a<pointer_type_die>()->get_type().tag_here() == DW_TAG_subroutine_type)
				);
	}
	
	bool cxx_generator_from_dwarf::cxx_type_can_be_qualified(iterator_df<type_die> p_d) const
	{
		if (p_d.tag_here() == DW_TAG_array_type) return false;
		if (p_d.tag_here() == DW_TAG_subroutine_type) return false;
		if (p_d.tag_here() == DW_TAG_subprogram) return false;
		// FIXME: any more?
		// now we know this bit of the type is fine for qualifying 
		// what about chained bits?
		if (!p_d.is_a<type_chain_die>()) return true;
		// if we're chaining void, also true
		if (!p_d.as_a<type_chain_die>()->get_type()) return true;
		return cxx_type_can_be_qualified(
			p_d.as_a<type_chain_die>()->get_type()
		);
	}

	bool cxx_generator_from_dwarf::cxx_type_can_have_name(iterator_df<type_die> p_d) const
	{
		/* In C++, introducing a name to a modified type requires typedef. */
		if (p_d.tag_here() == DW_TAG_typedef) return true;
		else if (p_d.is_a<type_chain_die>()) return false;
		else return true;
	}

	opt<string> cxx_generator_from_dwarf::referencing_ostream::can_refer_to(iterator_base i)
	{
		string name_prefix;
		// the null DIE reference does represent something: type void
		if (!i) return string("void");
		/* How do qualified names fit in to this logic? We used to have
		 * fq_name_parts_for for DIEs that were not immediate children of a CU. */
		switch (i.tag_here())
		{
			// return the friendly compiler-determined name or not, depending on argument
			case DW_TAG_base_type:
				if (use_friendly_base_type_names) return
						*gen.name_for_base_type(i.as_a<base_type_die>());
				goto handle_named_def;
			case DW_TAG_typedef:
				return (!!extra_prefix ? *extra_prefix : "") + *i.name_here();
			case DW_TAG_structure_type:
				if (i.name_here())
				{ name_prefix = use_struct_and_union_prefixes ? "struct " : ""; goto handle_named_def; }
				else goto handle_anonymous;
			case DW_TAG_union_type:
				if (i.name_here())
				{ name_prefix = use_struct_and_union_prefixes ? "union " : ""; goto handle_named_def; }
				else goto handle_anonymous;
			case DW_TAG_class_type:
				if (i.name_here())
				{ name_prefix = use_struct_and_union_prefixes ? "class " : ""; goto handle_named_def; }
				else goto handle_anonymous;
			case DW_TAG_enumeration_type:
				if (i.name_here())
				{ name_prefix = use_struct_and_union_prefixes ? "enum " : ""; goto handle_named_def; }
				else goto handle_anonymous;
			default:
				if (i.name_here()) goto handle_named_def;
				goto handle_anonymous;
			handle_named_def: {
				assert(i.name_here());
				/* In C code, we can get a problem with tagged namespaces (struct, union, enum) 
				 * overlapping with member names (and with each other). This causes an error like
				 * error: declaration of `cake::link_p2k_::kfs::uio_rw cake::link_p2k_::kfs::uio::uio_rw'
				 * librump.o.hpp:1341:6: error: changes meaning of `uio_rw' from `enum cake::link_p2k_::kfs::uio_rw'
				 *
				 * We work around it by using the anonymous DIE name if there is a CU-toplevel 
				 * decl of this name (HACK: should really be visible file-toplevel).
				 */

				// to make sure we don't get ourselves as "conflicting",
				// we should check that we're a member_die or other non-CU-level thing
				auto conflicting_toplevel_die = 
					(i.parent().tag_here() != DW_TAG_compile_unit) 
						? i.root().find_visible_grandchild_named(*i.name_here())
						: iterator_base::END;
				// if we get a conflict, we shouldn't be conflicting with ourselves
				assert(!conflicting_toplevel_die || conflicting_toplevel_die != i);
				if (!conflicting_toplevel_die)
				{
					return (!!extra_prefix ? *extra_prefix : "") + name_prefix
					+ gen.cxx_name_from_string(*i.name_here(), "_dwarfhpp_");
				}
				assert(conflicting_toplevel_die);
				// this is the conflicting case
				cerr << "Warning: renaming element " << *i.name_here()
					<< " child of " << i.parent().summary()
					<< " because it conflicts with toplevel " << conflicting_toplevel_die.summary()
					<< endl;
				goto handle_anonymous;
			}
			/* This block means our behaviour is "we can name anything".
			 * This means conversely that when we are asked to emit things,
			 * everything comes out with a name, generated if necessary.
			 * This is why the namer is used in both places: it ensures
			 * that e.g. if we have 'typedef struct { } blah'
			 * the inner struct is given a name and enumerated as a dependency
			 * of the typedef, so the named struct wil lbe emitted first.
			 * If we returned no name for the struct, it will not be a
			 * dependency but also will not be treated as a nameable
			 * reference when emitting the typedef, instead being inlined. */
			handle_anonymous: {
				// anonymous or conflicting -- both the same
				ostringstream s;
				s << "_dwarfhpp_anon_" << hex << i.offset_here();
				return (!!extra_prefix ? *extra_prefix : "") + name_prefix
					+ s.str();
				// we never return this
				// return opt<string>();
			}
		}
	}


	pair<string, bool>
	cxx_generator_from_dwarf::cxx_decl_from_type_die(
		iterator_df<type_die> p_d, 
		opt<string> maybe_name/*= opt<string>()*/,
		bool use_friendly_base_type_names /*= true*/,  
		opt<string> extra_prefix /* = opt<string>() */,
		bool use_struct_and_union_prefixes /* = true */ )
	{
		referencing_ostream namer(*this,
			use_friendly_base_type_names,
			extra_prefix,
			use_struct_and_union_prefixes);
		ostringstream name_out; name_out << (maybe_name ? *maybe_name : "");
		ostringstream out;
		out << make_declaration_having_type(p_d, name_out.str(), namer);
		return make_pair(out.str(), false);
	}
	
	bool 
	cxx_generator_from_dwarf::is_builtin(iterator_df<basic_die> p_d)
	{ 
		bool retval = p_d.name_here() && p_d.name_here()->find("__builtin_") == 0;
		if (retval) cerr << 
			"Warning: DIE at 0x" 
			<< hex << p_d.offset_here() << dec 
			<< " appears builtin and will be ignored." << endl;
		return retval; 
	}

#if 0
	bool 
	cxx_generator_from_dwarf::cxx_assignable_from(
		iterator_df<type_die> dest,
		iterator_df<type_die> source
	)
	{
		// FIXME: better approximation of C++ assignability rules goes here
		/* We say assignable if
		 * - base types (any), or
		 * - pointers to fq-nominally equal types, or
		 * - fq-nominally equal structured types _within the same dieset_ */

		if (dest.tag_here() == DW_TAG_base_type && source.tag_here() == DW_TAG_base_type)
		{ return true; }

		if (dest.tag_here() == DW_TAG_structure_type 
		&& source.tag_here() == DW_TAG_structure_type)
		{
			return fq_name_for(source) == fq_name_for(dest)
				&& &source.root() == &dest.root();
		}

		if (dest.tag_here() == DW_TAG_pointer_type
		&& source.tag_here() == DW_TAG_pointer_type)
		{
			if (!dest.as_a<pointer_type_die>()->get_type())
			{
				return true; // can always assign to void
			}
			else return 
				source.as_a<pointer_type_die>()->get_type()
				&& fq_name_for(source.as_a<pointer_type_die>()->get_type()) 
					== 
					fq_name_for(dest.as_a<pointer_type_die>()->get_type());
		}

		return false;
	}
#endif

	bool 
	cxx_generator_from_dwarf::cxx_is_complete_type(iterator_df<type_die> t)
	{
		t = t->get_concrete_type();
		if (!t) return false;
		
// 		// if we can't find a definition, we're not complete
// 		if (t->get_declaration() && *t->get_declaration()) 
// 		{
// 			auto with_data_members = t.as_a<with_data_members_die>();
// 			if (with_data_members)
// 			{
// 				auto defn = with_data_members->find_my_own_definition();
// 				if (!defn) return false;
// 			}
// 			else assert(false); // FIXME: do we get decls for other kinds of DIE?
// 		}
		// HACK: For now, if we're *not* a definition, we're not complete.
		// It's the caller's problem to find a definition if one exists.
		if (t->get_declaration() && *t->get_declaration()) 
		{
			return false;
			// FIXME: use basic_die::find_definition
		}
		
		if (t.tag_here() == DW_TAG_array_type)
		{
			if (!t.as_a<array_type_die>()->element_count()
			|| *t.as_a<array_type_die>()->element_count() == 0)
			{
				return false;
			}
			else return true;
		}
		
		// if we're structured, we're complete iff 
		// we don't have the "declaration" flag
		// and all members are complete
		if (t.is_a<with_named_children_die>())
		{
			auto nc = t.as_a<with_named_children_die>();
			//cerr << "DEBUG: testing completeness of cxx type for " << *t << endl;
			auto ms = t.children().subseq_of<member_die>();
			for (auto i_member = ms.first; i_member != ms.second; ++i_member)
			{
				assert(i_member->get_tag() == DW_TAG_member);
				auto memb_opt_type = i_member->get_type();
				if (!memb_opt_type)
				{
					return false;
				}
				if (!cxx_is_complete_type(memb_opt_type))
				{
					return false;
				}
			}
		}

		return true;
	}
	
	pair<string, bool>
	cxx_generator_from_dwarf::name_for_type(
		iterator_df<type_die> p_d, 
		boost::optional<string> infix_typedef_name /*= none*/,
		bool use_friendly_base_type_names/*= true*/,
		optional<string> extra_prefix /* = optional<string>() */,
		bool use_struct_and_union_prefixes /* = true */
	)
	{
// 		try
// 		{
			return cxx_decl_from_type_die(p_d, 
				infix_typedef_name ? *infix_typedef_name : opt<string>(),
				use_friendly_base_type_names,
				extra_prefix ? *extra_prefix : opt<string>(),
				use_struct_and_union_prefixes);
// 		} 
// 		catch (dwarf::expr::Not_supported)
// 		{
// 			// HACK around this strange (const (array)) case
// 			if (p_d.tag_here() == DW_TAG_const_type
// 			 && dynamic_pointer_cast<const_type_die>(p_d)->get_type()->get_concrete_type().tag_here()
// 			 	== DW_TAG_array_type)
// 			{
// 				// assume that we will be emitting a typedef for the const type itself,
// 				// so just output its anonymous name.
// 				// BUT how did we emit that typedef? We would run down this same path.
// 				return make_pair(create_ident_for_anonymous_die(p_d), false);
// 			}
// 			else throw;
// 		}
	}	

	string 
	cxx_generator_from_dwarf::name_for_argument(
		iterator_df<formal_parameter_die> p_d, 
		int argnum
	)
	{
		std::ostringstream s;
		//std::cerr << "called name_for_argument on argument position " << argnum << ", ";
		if (p_d.name_here()) { /*std::cerr << "named ";*/ s << "_dwarfhpp_arg_" << *p_d.name_here(); /*std::cerr << *d.get_name();*/ }
		else { /*std::cerr << "anonymous";*/ s << "_dwarfhpp_anon_arg_" << argnum; }
		//std::cerr << std::endl;
		return s.str();
	} 

	string 
	cxx_generator_from_dwarf::protect_ident(const string& ident)
	{
		/* In at least one reported case, the DWARF name of a declaration
		 * appearing in a standard header (pthread.h) conflicts with a macro (__WAIT_STATUS). 
		 * We could protect every ident with an #if defined(...)-type block, but
		 * that would make the header unreadable. Instead, we make a crude HACK
		 * of a guess: protect the ident if it uses a reserved identifier
		 * (for now: beginning '__')
		 * and is all caps (because most macros are all caps). */

		std::ostringstream s;
		if (ident.find("__") == 0 && ident == boost::to_upper_copy(ident))
		{
			s << std::endl << "#if defined(" << ident << ")" << std::endl
				<< "_dwarfhpp_protect_" << ident << std::endl
				<< "#else" << std::endl
				<< ident << std::endl
				<< "#define _dwarfhpp_protect_" << ident << " " << ident << std::endl
				<< "#endif" << std::endl;
		}
		else s << ident;
		return s.str();
	}
	
	string 
	cxx_generator_from_dwarf::make_typedef(
		iterator_df<type_die> p_target,
		const string& typedef_name
	)
	{
		std::ostringstream out;
		string name_to_use = cxx_name_from_string(typedef_name, "_dwarfhpp_");
		// typedefs are named by definition, so only reserved words can cause problems
		assert(name_to_use == typedef_name || std::find(cxx_reserved_words.begin(),
			cxx_reserved_words.end(), typedef_name) != cxx_reserved_words.end());

		// Make sure that if we're going to throw an exception, we do it before
		// we write any output.
		// What if the thing we're typedefing has no name?
		// We're supposed to invent names (cxx_name_from_die) in that case.
		// name_for_type just calls cxx_decl_from_type_die,
		// which seems fishy, especially as we passed it 'name_to_use' as the name
		auto decl = cxx_decl_from_type_die(p_target,
				opt<string>(), // <-- this said 'name_to_use' but that seems wrong. We're proposing the typedef name as the name to use
				/* use_friendly_base_type_names */ true, // FIXME: get this from caller
				/* extra_prefix */ opt<string>(),
				/* use_struct_and_union_prefixes */ true); // FIXME: get this from caller
		
		//name_for_type(p_target, name_to_use);
		out << "typedef " << protect_ident(decl.first);
		// HACK: we use the infix for subroutine types
		if (!decl.second)
		{
			out << " " << protect_ident(name_to_use);
		}
		out << ";" << endl;
		return out.str();
	}
	
	string
	cxx_generator_from_dwarf::make_function_declaration_of_type(
		iterator_df<type_describing_subprogram_die> p_d,
		const string& name,
		bool write_semicolon /* = true */,
		bool wrap_with_extern_lang /* = true */
	)
	{
		std::ostringstream out;
		string name_to_use = cxx_name_from_string(name, "_dwarfhpp_");
		
		string lang_to_use;
		switch (p_d.enclosing_cu()->get_language())
		{
			case DW_LANG_C:
			case DW_LANG_C89:
			case DW_LANG_C99:
				lang_to_use = "C";
				break;
			default: 
				assert(false);
				wrap_with_extern_lang = false;
				break;
		}
		
		if (wrap_with_extern_lang)
		{
			out << "extern \"" << lang_to_use << "\" {\n";
		}
		out << cxx_decl_from_type_die(p_d).first;
		if (write_semicolon) out << ";";
		out << endl;
		if (write_semicolon && wrap_with_extern_lang) 
		{
			out << "} // end extern " << lang_to_use << endl;
		}
		
		return out.str();
	}

	/* How do we make a generic version of this that can optionally collect
	 * emit-{decl,def}-before dependencies? While still looking like a printer? */
	string
	cxx_generator_from_dwarf::make_declaration_having_type(
		iterator_df<type_die> t,
		const string& name_in, /* <-- this may have to become name_handler to collect deps */
			/* and sub-ostringstreams may have to become sub-name-handlers... HMM */
		cxx_generator_from_dwarf::referencing_ostream& namer,
		bool emit_fp_names /* = true */
	)
	{
	/*
    (* recurse down if we can *)
    match d with
        Pointer(pointeeT) ->
            let starAndMaybeParenify x t = begin
                let starred = "*" ^ x in
                match t with
                    Subprogram _ | Array _ -> "(" ^ starred ^ ")"
                  | _ -> starred
                end
            in
            printDecl pointeeT (starAndMaybeParenify ident pointeeT)
      | Int -> "int " ^ ident
      | Array(n, t) -> (printDecl t ident) ^ "[" ^ (string_of_int n) ^ "]"
      | Const(t) -> printDecl t ("const " ^ ident)
      | Subprogram(rett, argts) ->
            (printDecl rett (ident ^ "(" ^ (
               let concat_with_comma s1 s2 = s2 ^ ", " ^ s2 in
               List.fold_left concat_with_comma ""
                   (List.map (fun argt -> (printDecl argt "")) argts)
            ) ^ ")" ))

	 */
		if (namer.can_refer_to(t)) return (namer.fresh_context() << t << " " << name_in).str();
		assert(!!t); // namer has handled void
		assert(!t.is_a<base_type_die>()); // ... and base types (must have name)
		assert(!t.is_a<typedef_die>()); // and typedefs (ditto)
		if (t.is_a<with_data_members_die>()
			|| t.is_a<enumeration_type_die>())
		{	/* unnamed; caller may want us to emit an "inline
			 * definition" or to use a generated name */
			assert(!t.name_here());
			abort();
		}
		else if (t.is_a<subrange_type_die>())
		{
			return (namer.fresh_context() << t.as_a<subrange_type_die>()->get_type()
				<< " " << name_in).str();
		}
		else if (t.is_a<address_holding_type_die>())
		{
			auto pointeeT = t.as_a<address_holding_type_die>()->find_type();
			referencing_ostream o = namer.fresh_context();
			bool do_paren = (pointeeT.is_a<type_describing_subprogram_die>()
			 || pointeeT.is_a<array_type_die>());
			string op = (t.is_a<pointer_type_die>() ? "*" :
				         t.is_a<reference_type_die>() ? "&" :
				         t.is_a<rvalue_reference_type_die>() ? "&&" :
				         (string("/* really ") +
				             t.spec_here().tag_lookup(t.tag_here())
				             +  "*/ *")
			);
			if (do_paren) o << "(";
			o << op << name_in;
			if (do_paren) o << ")";
			string starred_name = o.str();
			return make_declaration_having_type(pointeeT, starred_name, namer,
				/* emit_fp_names */ false);
		}
		else if (t.is_a<array_type_die>())
		{
			// we only understand C arrays, for now
			int language = t.enclosing_cu()->get_language();
			assert(language == DW_LANG_C89 
				|| language == DW_LANG_C 
				|| language == DW_LANG_C99);
			auto elT = t.as_a<array_type_die>()->find_type();
			auto ctxt = namer.fresh_context();
			ctxt << make_declaration_having_type(elT, name_in, namer);
			vector<opt<Dwarf_Unsigned> > elCounts
			 = t.as_a<array_type_die>()->dimension_element_counts();
			for (auto maybe_count : elCounts)
			{
				ctxt << "[";
				if (maybe_count) namer << *maybe_count;
				ctxt << "]";
			}
			return ctxt.str();
		}
		else if (t.is_a<qualified_type_die>())
		{
			string qual = (t.is_a<const_type_die>() ? "const" :
				 t.is_a<volatile_type_die>() ? "volatile" :
				 t.is_a<restrict_type_die>() ? "restrict" :
				 std::regex_replace( /* best guess! */
				 	t.spec_here().tag_lookup(t.tag_here()),
				 	std::regex("DW_TAG_(.*)_type", std::regex_constants::extended),
					"$&") // FIXME: test this by commenting out the ?: above
				);
			auto innerT = t.as_a<qualified_type_die>()->find_type();
			referencing_ostream s = namer.fresh_context(); s << qual << " " << name_in;
			return make_declaration_having_type(innerT, s.str(), namer);
			/* NOTE: there were some quirks in the original cxx_decl_from_type_die
			 * implementation, splitting on cxx_type_can_be_qualified(chained_type)),
			 * but I'm not sure what case that hit (qualified-type DIE wrapped around
			 * a cxx type that can't be qualified? sounds like fishy DWARF) */
		}
		else if (t.is_a<type_describing_subprogram_die>())
		{
			auto rett = t.as_a<type_describing_subprogram_die>()->find_type();
			referencing_ostream s = namer.fresh_context();
			s << name_in << "(";
			auto fp_children = t->children().subseq_of<formal_parameter_die>();
			for (auto i_arg = fp_children.first; i_arg != fp_children.second; ++i_arg)
			{
				if (emit_fp_names && i_arg.name_here())
				{
					// FIXME: need to get arg name from our helper, to do mangling etc
					s << *i_arg.name_here() << " ";
				}
				if (i_arg != fp_children.first) s << ", ";
				s << make_declaration_having_type(i_arg->find_type(), "", namer);
			}
			s << ")";
			return make_declaration_having_type(
				rett,
				s.str(),
				namer
			);
		}
		else /*including if (t.is_a<string_type_die>()) */
		{
			assert(false); abort();
		}
		assert(false); abort();
	}

	srk31::indenting_ostream&
	cxx_generator_from_dwarf::emit_definition(iterator_base i_d,
		srk31::indenting_ostream& out,
		referencing_ostream& namer)
	{
		if (i_d.is_a<enumeration_type_die>())
		{
			out << "enum " 
				<< protect_ident(
					*namer.can_refer_to(i_d.as_a<enumeration_type_die>())
				)
				<< " { " << std::endl;

			auto children_seq = i_d.children().subseq_of<enumerator_die>();
			for (auto i_child = children_seq.first; i_child != children_seq.second; ++i_child)
			{
				out.inc_level();
				emit_definition(*i_child, out, namer);
				out.dec_level();
			}

			out << endl << "};" << endl;
		}
		else if (i_d.is_a<enumerator_die>())
		{
			auto p_d = i_d.as_a<enumerator_die>();
			auto parent = p_d.parent().as_a<enumeration_type_die>();
			if (parent.children().subseq_of<enumerator_die>().first != p_d)
			{ out << ", " << endl; }
			out << protect_ident(*p_d.name_here());
		}
		else if (i_d.is_a<with_data_members_die>())
		{
			auto p_d = i_d.as_a<with_data_members_die>();
			out << cxx_decl_from_type_die(p_d,
				opt<string>(),
				namer.use_friendly_base_type_names,
				namer.extra_prefix,
				/* use_struct_and_union_prefixes */ true).first;
			if (!(p_d->get_declaration() && *p_d->get_declaration()))
			{
				out << " { " << std::endl;
				out.inc_level();
				auto children_seq = p_d.children().subseq_of<member_die>();
				for (auto i_child = children_seq.first; i_child != children_seq.second; ++i_child)
				{
					out.inc_level();
					emit_definition(*i_child, out, namer);
					out.dec_level();
				}
				out.dec_level();
				out << "}"; // we used to do __attribute__((packed)) here....
			}
			else
			{
				/* declarations shouldn't have any member children */
				auto ms = p_d.children().subseq_of<member_die>();
				assert(srk31::count(ms.first, ms.second) == 0);
			}
			out << ";" << std::endl;
		}
		else if (i_d.is_a<member_die>())
		{
			auto p_d = i_d.as_a<member_die>();

			/* To reproduce the member's alignment, we always issue an align attribute. 
			 * We choose our alignment so as to ensure that the emitted field is located
			 * at the offset specified in DWARF. */
			auto member_type = transform_type(p_d->get_type(), i_d);

			// recover the previous formal parameter's offset and size
			iterator_df<with_data_members_die> p_type = p_d.parent().as_a<with_data_members_die>();
			assert(p_type);
			auto ms = p_type.children().subseq_of<member_die>();
			auto i = ms.first;
			auto prev_i = ms.second; // initially
			while (i != ms.second
				&& i.offset_here() != p_d.offset_here())
			{ prev_i = i; ++i; }

			// now i points to us, and prev_i to our predecessor
			assert(i != ms.second); // would mean we failed to find ourselves

			Dwarf_Unsigned cur_offset;
			if (prev_i == ms.second) cur_offset = 0;
			else 
			{
				auto prev_member = prev_i.as_a<member_die>();
				assert(prev_member->get_type());

				if (prev_member->get_data_member_location())
				{
					auto prev_member_calculated_byte_size 
					 = transform_type(prev_member->get_type(), i_d)->calculate_byte_size();
					if (!prev_member_calculated_byte_size)
					{
						cerr << "couldn't calculate size of data member " << prev_member
							<< " so giving up control of layout in struct " << *p_type
							<< endl;
						cur_offset = std::numeric_limits<Dwarf_Unsigned>::max(); // sentinel value
					}
					else
					{
						cur_offset = dwarf::expr::evaluator(
							prev_member->get_data_member_location()->at(0), 
							p_d.spec_here(),
							stack<Dwarf_Unsigned>(
								// push zero as the initial stack value
								deque<Dwarf_Unsigned>(1, 0UL)
								)
							).tos()
							+ *prev_member_calculated_byte_size; 
					}
				}
				else
				{
					cerr << "no data member location: context die is " << p_d.parent() << endl;
					cur_offset = (p_d.parent().tag_here() == DW_TAG_union_type) 
						? 0 : std::numeric_limits<Dwarf_Unsigned>::max();
				}
			}

			/* This handles the difficulty where in C code, we can get a problem
			 * with tagged namespaces (struct, union, enum) 
			 * overlapping with member names (and with each other).
			 */
			string name_to_use = *namer.can_refer_to(p_d);

			auto decl = name_for_type(member_type, name_to_use);
			// (p_d.name_here())
			//	? name_for_type(member_type, *p_d.name_here())
			//	: name_for_type(member_type, optional<string>());

			out << protect_ident(decl.first);
			out	<< " ";
			if (!decl.second)
			{
				out << protect_ident(name_to_use);
			}

			if (p_d->get_data_member_location() && p_d->get_data_member_location()->size() == 1)
			{
				Dwarf_Unsigned target_offset = dwarf::expr::evaluator(
					p_d->get_data_member_location()->at(0), 
					p_d.spec_here(),
					stack<Dwarf_Unsigned>(
						// push zero as the initial stack value
						deque<Dwarf_Unsigned>(1, 0UL)
						)
					).tos();

				if (!(target_offset > 0 && cur_offset != std::numeric_limits<Dwarf_Unsigned>::max()))
				{
					out << ";";
				}
				else
				{
					/* Calculate a sensible align value for this. We could just use the offset,
					 * but that might upset the compiler if it's larger than what it considers
					 * the reasonable biggest alignment for the architecture. So pick a factor
					 * of the alignment s.t. no other multiples exist between cur_off and offset. */
					//std::cerr << "Aligning member to offset " << offset << std::endl;

					// just search upwards through powers of two...
					// until we find one s.t.
					// searching upwards from cur_offset to factors of this power of two,
					// our target offset is the first one we find
					unsigned power = 1;
					bool is_good = false;
					const unsigned MAXIMUM_SANE_OFFSET = 1<<30; // 1GB
					unsigned test_offset = cur_offset;
					do
					{
						// if it doesn't divide by our power of two,
						// set it to the next-highest multiple of our power
						if (test_offset % power != 0)
						{
							test_offset = ((test_offset / power) + 1) * power;
						}
						assert(test_offset % power == 0);
						// now we have the offset we'd get if we chose aligned(power)

						// HMM: what if test_offset
						// also divides by the *next* power of two?
						// In that case, we will go straight through.
						// Eventually we will get a power that is bigger than it.
						// Then it won't be divisible.

						// now test_offset is a multiple of power -- check it's our desired offset
						is_good = (test_offset == target_offset);
					} while (!is_good && (power <<= 1, power != 0));

					if (power == 0 || test_offset < cur_offset)
					{
						// This happens for weird object layouts, i.e. with gaps in.
						// Test case: start 48, target 160; the relevant power 
						// cannot be higher than 32, because that's the highest that
						// divides 160. But if we choose align(32), we will get offset 64.
						out << ";" << endl;
						out << "// WARNING: could not infer alignment for offset "
							<< target_offset << " starting at " << cur_offset << endl;
						if (target_offset - cur_offset < MAXIMUM_SANE_OFFSET)
						{
							out << "char " << "_dwarfhpp_anon_" << hex << p_d.offset_here()
								<< "_padding[" << target_offset - cur_offset << "];" << endl;
						}
						else
						{
							out << "// FIXME: this struct is WRONG (probably until we support bitfields)" << endl;
						}
						power = 1;
					}

					out << " __attribute__((aligned(" << power << ")));";
				}

				/*<< " static_assert(offsetof(" 
					<< name_for_type(compiler, p_type, boost::optional<const std::string&>())
					<< ", "
					<< protect_ident(*d.get_name())
					<< ") == " << offset << ");" << */

				out << " // offset: " << target_offset << endl;
			}
			else 
			{
				// guess at natural alignment and hope for the best
				out << "; // no DW_AT_data_member_location, so hope the compiler gets it right" << std::endl;
			}
		}
		return out;
	}
	

/* from dwarf::tool::cxx_target */
	optional<string>
	cxx_target::name_for_base_type(iterator_df<base_type_die> p_d)
	{
		map<base_type, string>::iterator found = base_types.find(
			base_type(p_d));
		if (found == base_types.end()) return optional<string>();
		else return found->second;
	}

	const vector<string> cxx_generator::cxx_reserved_words = {
		"auto",
		"const",
		"double",
		"float",
		"int",
		"short",
		"struct",
		"unsigned",
		"break",
		"continue",
		"else",
		"for",
		"long",
		"signed",
		"switch",
		"void",
		"case",
		"default",
		"enum",
		"goto",
		"register",
		"sizeof",
		"typedef",
		"volatile",
		"char",
		"do",
		"extern",
		"if",
		"return",
		"static",
		"union",
		"while",
		"asm",
		"dynamic_cast",
		"namespace",
		"reinterpret_cast",
		"try",
		"bool",
		"explicit",
		"new",
		"static_cast",
		"typeid",
		"catch",
		"false",
		"operator",
		"template",
		"typename",
		"class",
		"friend",
		"private",
		"this",
		"using",
		"const_cast",
		"inline",
		"public",
		"throw",
		"virtual",
		"delete",
		"mutable",
		"protected",
		"true",
		"wchar_t",
		"and",
		"bitand",
		"compl",
		"not_eq",
		"or_eq",
		"xor_eq",
		"and_eq",
		"bitor",
		"not",
		"or",
		"xor"
	};

} // end namespace tool
} // end namespace dwarf
