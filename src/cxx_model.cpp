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
using std::ostream;
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
	cxx_generator::cxx_name_from_string(const string& s) const
	{
		const char prefix[] = "_dwarfhpp_"; // FIXME: init from pure virtual function, cache in obj?
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
	cxx_generator::make_valid_cxx_ident(const string& word) const
	{
		// FIXME: make this robust to illegal characters other than spaces
		string working = word;
		return is_valid_cxx_ident(word) ? word
			: (std::replace(working.begin(), working.end()-1, ' ', '_'), working);
	}
	
	string 
	cxx_generator::name_from_name_parts(const vector<string>& parts) const
	{
		ostringstream s;
		for (auto i_part = parts.begin(); i_part != parts.end(); i_part++)
		{
			if (i_part != parts.begin()) s << "::";
			s << *i_part;
		}
		return s.str();
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

	cxx_generator_from_dwarf::referencer_fn_t
	cxx_generator_from_dwarf::get_default_referencer() const
	{
		return [this](iterator_base i, enum ref_kind k) -> opt<string> {
			string name_prefix;
			// the null DIE reference does represent something: type void
			if (!i) return /*string("void")*/ opt<string>();
			/* How do qualified names fit in to this logic? We used to have
			 * fq_name_parts_for for DIEs that were not immediate children of a CU. */
			bool do_prefix = (k == DEFINING) || this->use_struct_and_union_prefixes();
			switch (i.tag_here())
			{
				case DW_TAG_base_type:
					// If our config is to use the friendly compiler-determined builtin name like "int",
					// that does not count as a name we are expected to generate.
					if (this->use_friendly_base_type_names()) return opt<string>();
					goto handle_named_def;
				case DW_TAG_typedef:
					return (!!this->prefix_for_all_idents() ? *this->prefix_for_all_idents() : "") + *i.name_here();
				case DW_TAG_structure_type:
					if (i.name_here())
					{ name_prefix = do_prefix ? "struct " : ""; goto handle_named_def; }
					else goto handle_anonymous;
				case DW_TAG_union_type:
					if (i.name_here())
					{ name_prefix = do_prefix ? "union " : ""; goto handle_named_def; }
					else goto handle_anonymous;
				case DW_TAG_class_type:
					if (i.name_here())
					{ name_prefix = do_prefix ? "class " : ""; goto handle_named_def; }
					else goto handle_anonymous;
				case DW_TAG_enumeration_type:
					if (i.name_here())
					{ name_prefix = do_prefix ? "enum " : ""; goto handle_named_def; }
					else goto handle_anonymous;
				default:
					if (i.name_here()) goto handle_named_def;
					goto handle_anonymous;
				handle_named_def:
					assert(i.name_here());
					return (!!this->prefix_for_all_idents() ? *this->prefix_for_all_idents() : "") + name_prefix
						+ this->cxx_name_from_string(*i.name_here());
				handle_anonymous:
					return opt<string>();
			}
		};
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
	cxx_generator_from_dwarf::decl_having_type(
		iterator_df<type_die> t,
		const string& name,
		cxx_generator_from_dwarf::referencer_fn_t maybe_get_name,
		bool emit_fp_names
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
	 	ostringstream str;
		if (t && t.is_a<subprogram_die>())
		{
			/* The namer will think we're asking if we can name the subprogram.
			 * In this context we're not interested in that; to emit a declaration
			 * of the subprogram's type, we need to force the expansion of that
			 * declaration. */ // FIXME: this seems gash
		}
		else if (!t) { str << "void " << name; goto out; }
		else if (t.is_a<base_type_die>() && this->use_friendly_base_type_names())
		{ str << *this->name_for_base_type(t.as_a<base_type_die>()) << " " << name; goto out; }
		else if (maybe_get_name(t, NORMAL))
		{ str << *maybe_get_name(t, NORMAL) << " " << name; goto out; }
		assert(!t.is_a<base_type_die>()); // either we or namer has handled base types (must have name)
		assert(!t.is_a<typedef_die>()); // and namer has handled typedefs (ditto)
		if (t.is_a<with_data_members_die>()
			|| t.is_a<enumeration_type_die>())
		{	/* unnamed and no way to refer to it; only way is to emit an "inline definition" */
			assert(!t.name_here());
			indenting_ostream indenting_str(str);
			indenting_str << defn_of_die(
				t,
				maybe_get_name,
				opt<string>("") /* override_name -- we override it to be empty! */,
				/* emit_fp_names */ false,
				/* write_semicolon */ false
			) << " " << name;
		}
		else if (t.is_a<subrange_type_die>())
		{
			// to declare a thing as of subrange type, just declare it as of the full type
			str << decl_having_type(t.as_a<subrange_type_die>()->get_type(),
				name,
				maybe_get_name,
				emit_fp_names);
		}
		else if (t.is_a<address_holding_type_die>())
		{
			auto pointeeT = t.as_a<address_holding_type_die>()->find_type();
			ostringstream o;
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
			o << op << name;
			if (do_paren) o << ")";
			string starred_name = o.str();
			str << decl_having_type(pointeeT, starred_name, maybe_get_name, /* emit_fp_names */ false);
		}
		else if (t.is_a<array_type_die>())
		{
			// we only understand C arrays, for now
			int language = t.enclosing_cu()->get_language();
			assert(language == DW_LANG_C89 
				|| language == DW_LANG_C 
				|| language == DW_LANG_C99);
			auto elT = t.as_a<array_type_die>()->find_type();
			str << decl_having_type(elT, name, maybe_get_name, emit_fp_names);
			vector<opt<Dwarf_Unsigned> > elCounts
			 = t.as_a<array_type_die>()->dimension_element_counts();
			for (auto maybe_count : elCounts)
			{
				str << "[";
				if (maybe_count) str << *maybe_count;
				str << "]";
			}
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
			str << decl_having_type(innerT, qual + " " + name, maybe_get_name, emit_fp_names);
			/* NOTE: there were some quirks in the original cxx_decl_from_type_die
			 * implementation, splitting on cxx_type_can_be_qualified(chained_type)),
			 * but I'm not sure what case that hit (qualified-type DIE wrapped around
			 * a cxx type that can't be qualified? sounds like fishy DWARF) */
		}
		else if (t.is_a<type_describing_subprogram_die>())
		{
			auto rett = t.as_a<type_describing_subprogram_die>()->find_type();
			ostringstream ctxt;
			ctxt << name << "(";
			auto fp_children = t->children().subseq_of<formal_parameter_die>();
			for (auto i_arg = fp_children.first; i_arg != fp_children.second; ++i_arg)
			{
				if (i_arg != fp_children.first) ctxt << ", ";
				ctxt << decl_having_type(i_arg->find_type(), emit_fp_names ? *i_arg.name_here() : "", maybe_get_name, false);
			}
			ctxt << ")";
			str << decl_having_type(rett, ctxt.str(), maybe_get_name, false);
			// str << " /* return type is " << rett << " */ ";
		}
		else /*including if (t.is_a<string_type_die>()) */
		{
			cerr << "Confused by " << t << endl;
			assert(false); abort();
		}
	out:
		return str.str();
	}
	
	string
	cxx_generator_from_dwarf::decl_of_die(
		iterator_df<program_element_die> d,
		cxx_generator_from_dwarf::referencer_fn_t maybe_get_name,
		bool emit_fp_names,
		bool write_semicolon /* = false */
	)
	{
		ostringstream out;
		if (!d) goto undeclarable; // void type is undeclarable
		out << "/* decl'ing " << d << " */" << endl;
		if (d.is_a<variable_die>())
		{
			out << "extern " << decl_having_type(
				d.as_a<variable_die>()->find_type(),
				cxx_name_from_string(*d.name_here()), // <-- how do we pick names to use for definitions? namer should do this too?
				maybe_get_name,
				false) << (write_semicolon ? ";" : "") << std::endl;
		}
		else if (d.is_a<subprogram_die>())
		{
			string name_to_use = cxx_name_from_string(*d.name_here());
			string lang_to_use;
			// want to compare against codegen context somehow
			switch (d.enclosing_cu()->get_language())
			{
				case DW_LANG_C:
				case DW_LANG_C89:
				case DW_LANG_C99:
#ifdef DW_LANG_C11
				case DW_LANG_C11:
#endif
					lang_to_use = "C";
					break;
				case DW_LANG_C_plus_plus:
#ifdef DW_LANG_C_plus_plus_03
				case DW_LANG_C_plus_plus_03:
#endif
#ifdef DW_LANG_C_plus_plus_11
				case DW_LANG_C_plus_plus_11:
#endif
#ifdef DW_LANG_C_plus_plus_17
				case DW_LANG_C_plus_plus_17:
#endif
					lang_to_use = "C++";
				default: 
					assert(false); abort();
					break;
			}
			bool wrap_with_extern_lang = (language_linkage_name() != lang_to_use);
			if (wrap_with_extern_lang)
			{
				out << "extern \"" << lang_to_use << "\" {\n";
			}
			out << decl_having_type(
				d.as_a<subprogram_die>(),
				name_to_use, // <-- how do we pick names to use for definitions? namer should do this too?
				maybe_get_name,
				true);
			if (write_semicolon) out << ";";
			out << endl;
			if (write_semicolon && wrap_with_extern_lang)
			{
				out << "} // end extern " << lang_to_use << endl;
			}
		}
		else if (d.is_a<with_data_members_die>() || d.is_a<enumeration_type_die>())
		{
			/* FIXME: to forward-declare an aggregate type we always use the tag.
			 * How can we force maybe_get_name to always include it, or to leave it out?
			 * For now we rely on that the default is to icnlude it. */
			/* We can only forward-declare something we're giving a name to. */
			opt<string> maybe_name = maybe_get_name(d, DEFINING);
			if (maybe_name)
			{
				out << *maybe_name << (write_semicolon ? ";" : "")
					<< std::endl;
			}
			else cerr << "Skipping declaration of unnameable aggregate type " << d  << endl;
		}
		else if (d.is_a<typedef_die>())
		{
			/* What does this mean? typedefs only have decls, not defs.
			 * We decree this in order to avoid duplicates. We never
			 * generate dependencies on the def of a typedef. The decl
			 * is the only thing we output. */
			out << "typedef " << decl_having_type(
				d.as_a<typedef_die>()->find_type(),
				cxx_name_from_string(*d.name_here()), // <-- how do we pick names to use for definitions?
				maybe_get_name,
				false) << (write_semicolon ? ";" : "") << std::endl;
		}
		else if (d.is_a<type_die>())
		{
		undeclarable:
			/* It's not possible to declare other types in C++; they are
			 * just constructed from syntax where needed. */
			cerr << "Skipping declaration of undeclareable " << d << endl;
		}
		else
		{
			abort(); // not sure what hits this case
		}
		return out.str();
	}

	/* Why is this a manipulator, not just a string-returner?
	 * It's so that we can call other stuff on the stream, like inc_level()
	 * and dec_level(). If we output a whole collection of DIEs in this way,
	 * this is fine. But our usage model involves outputting snippets
	 * (as strings?) which we then emit in a topsorted order. That means
	 * the tabbing won't necessarily be right. At a minimum, to start a new
	 * string we have to initialise the indented ostream with the right
	 * initial tabbing level. */
	cxx_generator_from_dwarf::strmanip_t cxx_generator_from_dwarf::defn_of_die(
		iterator_df<program_element_die> i_d,
		cxx_generator_from_dwarf::referencer_fn_t maybe_get_name,
		opt<string> override_name /* = opt<string>() */,
		bool emit_fp_names /* = true */,
		bool write_semicolon /* = true */,
		string body /* = string() */
	)
	{
		cxx_generator_from_dwarf::strmanip_t self;
		self = [=](indenting_ostream& out) -> indenting_ostream& {
			out << "/* def'ing " << i_d << " */" << endl;
			if (i_d.is_a<enumeration_type_die>())
			{
				out << "enum " 
					<< protect_ident(*maybe_get_name(i_d.as_a<enumeration_type_die>(), NORMAL))
					<< " { " << std::endl;

				auto children_seq = i_d.children().subseq_of<enumerator_die>();
				for (auto i_child = children_seq.first; i_child != children_seq.second; ++i_child)
				{
					out.inc_level();
					out << defn_of_die(i_child, maybe_get_name);
					out.dec_level();
				}
				out << std::endl;
				out << "}" << (write_semicolon ? ";" : "") << std::endl;
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
				// instead of printing the header using decl_of_die,
				// which will fail for an anonymous struct, we open-code
				// it here.
				string name_to_use = (override_name ? *override_name : *maybe_get_name(i_d, DEFINING));
				/* If we're not generating the struct/union/... tag prefix in the name,
				 * we need to generate it here. Similarly if we're given a name that is
				 * empty, it means that we're generating an anonymous inline definition
				 * and we need to use the keyword to introduce it, e.g. "struct" etc,
				 * regardless of whether we can refer to nameful structs without 'struct'. */
				if (!use_struct_and_union_prefixes()
					|| (override_name && *override_name == "")) out << *tag_for_die(p_d) << " ";
				out << name_to_use;
				if (!(p_d->get_declaration() && *p_d->get_declaration()))
				{
					out << " { " << std::endl;
					out.inc_level();
					auto children_seq = p_d.children().subseq_of<member_die>();
					for (auto i_child = children_seq.first; i_child != children_seq.second; ++i_child)
					{
						out.inc_level();
						out << defn_of_die(i_child, maybe_get_name);
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
				if (write_semicolon) out << ";";
				out << std::endl;
			}
			/* variable_die */
			else if (i_d.is_a<variable_die>())
			{
				out << "/* FIXME: emitting variable definitions */" << std::endl;
				// NOTE these must also use complete types, like members
			}
			else if (i_d.is_a<subprogram_die>())
			{
				out << decl_having_type(i_d, *i_d.name_here(), maybe_get_name, true) << endl
					<< "{" << endl;
				out.inc_level();
				out << body;
				out.dec_level();
				out << "}" << endl;
			}
			else if (i_d.is_a<member_die>())
			{
				auto p_d = i_d.as_a<member_die>();

				/* To reproduce the member's alignment, we always issue an align attribute. 
				 * We choose our alignment so as to ensure that the emitted field is located
				 * at the offset specified in DWARF. */
				auto member_type = /*transform_type(*/p_d->get_type()/*, i_d)*/;

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
						 = /*transform_type(*/prev_member->get_type()/*, i_d)*/->calculate_byte_size();
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

				/* This needs to handle the difficulty where in C code, we can get a problem
				 * with tagged namespaces (struct, union, enum) 
				 * overlapping with member names (and with each other).
				 */
				out << decl_having_type(p_d->find_type(), *maybe_get_name(p_d, DEFINING),
					maybe_get_name, /* emit fp names */ false
					
					);
				// FIXME: type must be complete! Need to pass this down!
				// FIXME: do protect_ident() ! This is a 'name', not a 'reference'

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
						out <<  (write_semicolon ? ";" : "");
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

						out << " __attribute__((aligned(" << power << ")))"
							<< (write_semicolon ? ";" : "");
					}

					out << " // offset: " << target_offset << endl;
				}
				else 
				{
					// guess at natural alignment and hope for the best
					out <<  (write_semicolon ? ";" : "");
					if (!i.is_a<union_type_die>())
					{
						out << " /* no DW_AT_data_member_location, so hope the compiler gets it right */" << std::endl;
					}
				}
			} // end member_die case
			else
			{
				out << "/* asked to define surprising DIE: " << i_d.summary() << " */" << std::endl;
			}
			return out;
		};
		return self;
	}

/* from dwarf::tool::cxx_target */
	optional<string>
	cxx_target::name_for_base_type(iterator_df<base_type_die> p_d) const
	{
		auto found_seq = base_types.equal_range(base_type(p_d));
		if (found_seq.first == found_seq.second) return optional<string>();
		/* PROBLEM: wchar_t and int are indistinguishable in DWARF,
		 * even though libcxxgen's cxx_compiler puts them in different
		 * equivalence classes. The base_types logic in libcxxgen does not
		 * re-group by equivalence class. We could either
		 * 1. look for the first equivalence class having any member
		 *    whose name is in the equal_range we have found,
		 *    and just use the first member of that; or
		 * 2. if any of the equal_range has a name that matches p_d's,
		 *   just use that? i.e. it is definitely a name built in to the compiler.
		 * We use number 1 for now. FIXME: cache the results somewhere in
		 * libcxxgen, i.e. have it tie its results back to their equivalence classes.
		 */
		set< cxx_compiler::equiv_class_ptr_t > seen_equiv_classes;
		unsigned count = 0;
		for (auto i_found = found_seq.first; i_found != found_seq.second; ++i_found)
		{
			++count;
			// do we have an equivalence class?
			auto& name = i_found->second.first;
			auto& equiv = i_found->second.second;
			if (equiv) seen_equiv_classes.insert(equiv);
			if (p_d.name_here() && *p_d.name_here() == name)
			{
				return name; // OK to use the name of the incoming DIE; compiler also understands this
			}
		}
#if 0
		// else just return whatever the compiler knows it as... try to canonicalise by equiv class
		cerr << "Found " << count << " known base types (across " << seen_equiv_classes.size()
			<< " name-equivalence classes; first is " << (*seen_equiv_classes.begin())[0]
			<< ") equivalent to " << p_d << std::endl;
#endif
		if (seen_equiv_classes.size() == 0)
		{
			cerr << "Fishy: saw no base type equivalence classes for " << p_d << std::endl;
			goto last_gasp;
		}
		else if (seen_equiv_classes.size() > 1)
		{
			cerr << "Slightly fishy: saw multiple base type equivalence classes for " << p_d << ": ";
			for (auto i_equiv = seen_equiv_classes.begin();
				i_equiv != seen_equiv_classes.end(); ++i_equiv)
			{
				if (i_equiv != seen_equiv_classes.begin()) cerr << ", ";
				cerr << (*i_equiv)[0];
			}
			cerr << endl;
		}
		return string((*seen_equiv_classes.begin())[0]);
	last_gasp:
		return found_seq.first->second.first;
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
