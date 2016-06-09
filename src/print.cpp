/* dwarfidl: 
 * 
 * print.cpp: 
 *
 * Copyright (c) 2009--2014, Stephen Kell.
 */

#include "print.hpp"
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
using namespace dwarf::spec;
using dwarf::lib::Dwarf_Half;
using dwarf::lib::Dwarf_Off;
using dwarf::lib::Dwarf_Unsigned;

/* TODO: things I want the printer to do:
 *
 * - factor out isomorphic subgraphs, e.g. types, s.t. 
       the printout is prefixed by a "shared" CU, and each 
       repeated definition is given all of its offsets in the file
       as a set of labels, e.g. 0xa: 0x110a: 0x235a: base_type int;
       
       NOTE: this is potentially ambiguous in that we no longer see 
       which elements referred to which other isomorphic elements.
       But this could be recovered using offset ranges to infer CU
       membership. 
   
   - expand out location lists etc., but use {arch, ABI, src-language}-dep "defaults"
     s.t. unsurprising location exprs can be omitted
     (e.g. for member layouts that follow the ABI on the given triple; 
           for argument locations that follow the calling ABI (BUT only for 0-vaddr case,
                also the decl case);
				
				CAN we also print out a dwarfidl description of the "ABI-specified"
				definitions here? Not directly, but we could write a function from 
				a C function signature to its "standard ABI" DWARF, and likewise
				for a C struct or union.
				
           others?)
		   
		   NOTE that this implies that some details of the enclosing ELF (or Mach-O) file
		   are visible -- specifically the architecture and OS ABI information.
	
	- have friendly source-code coordinates in the info section, subject to the usual 
	  unambiguity constraint, e.g. <"foo.c", 10, _>
	
	- full syntax for a named subprogram might look like the following.
	
	@0xc00: myfunc: (arg1 : int, arg2 : float) => char [ /* attributes go here * /];
	
	which is sugar for 
		@0xc00: subprogram myfunc [ type = @0x2c ] /* attributes * / {
			@0xc04: formal_parameter [type = @0x27];
			@0xc08: formal_parameter [type = @0x2a];
		};
	
	- we are printing info { } for now, but we might also want to print 
	                  line { }
	                  and 
	                  frame { }
	                  and even
	                  macro { }  (or fold that into "info" somehow? hmm, seems unlikely; perhaps fold into "line"?)
	
	- def/use abstraction for "boring" definitions: allow "use" of a literal expression
	  that can be used in place of the definition, e.g. "int ref" instead of 
	  referring to @beef 
	
	- def/use including attrs, e.g. "base_type [byte_size = 4] ref"
	
	- ++++ solve the ambiguity here: "base_type" is a tag, but "int" is a DIE name; 
	  perhaps tags should be prefixed by something, like %, 
	  and attrs with something like ~?
	  ideally not something that's a binary operator; 
	  ` or # or ~ all seem reasonable
	
	- how do DWARF specs fit in?

 */

namespace dwarf {
namespace tool {

	using namespace dwarf::core;

	vector<string> reserved_words; // empty for now
	
	string 
	ident_name_from_string(const string& s, const char *prefix)
	{
		if (is_reserved(s)) 
		{
			cerr << "Warning: generated dwarfidl name `" << (prefix + s) 
				<< " from reserved word " << s << endl;
			return prefix + s; // being reserved implies s is lexically okay
		}
		else if (is_valid_ident(s)) return s;
		else // something is lexically illegal about s
		{
			return make_valid_ident(s);
		}
	}
	
	// static function
	bool 
	is_reserved(const string& word)
	{
		return std::find(reserved_words.begin(),
			reserved_words.end(), 
			word) != reserved_words.end();
	}
	
	// static function
	bool 
	is_valid_ident(const string& word)
	{
		static const regex e("[a-zA-Z_][a-zA-Z0-9_]*");
		return !is_reserved(word) &&
			std::move(regex_match(word, e));	
	}
	
	string 
	make_valid_ident(const string& word)
	{
		// FIXME: make this robust to illegal characters other than spaces
		string working = word;
		return is_valid_ident(word) ? word
			: (std::replace(working.begin(), working.end()-1, ' ', '_'), working);
	}
	
	string 
	name_from_name_parts(const vector<string>& parts) 
	{
		ostringstream s;
		for (auto i_part = parts.begin(); i_part != parts.end(); i_part++)
		{
			if (i_part != parts.begin()) s << ".";
			s << *i_part;
		}
		return s.str();
	}
	
	bool 
	type_infixes_name(iterator_df<type_die> t)
	{
		assert(t);
		auto unq_t = t->get_unqualified_type();
		return 
			unq_t && (
			unq_t->get_tag() == DW_TAG_subroutine_type
			||  unq_t->get_tag() == DW_TAG_array_type
			|| 
			(unq_t->get_tag() == DW_TAG_pointer_type &&
				unq_t.as_a<pointer_type_die>()->get_type()
				&& 
				unq_t.as_a<pointer_type_die>()->get_type()
					->get_tag()	== DW_TAG_subroutine_type)
				);
	}

	opt<string>
	dwarfidl_name_from_die(const iterator_base& i)
	{
		/* FIXME: some escaping is necessary here, but foremost a question
		 * needs answering: what are "names" for in dwarfidl? 
		 *
		 * It seems best to suppose that 
		 * -- any element may be given a name;
		 * -- for non-synthetic program elements, this should be the source-level name
		 * -- in other cases names are arbitrary details of the DWARF generator.
		 * Most importantly,
		 * -- names are not necessarily unique 
		 * -- (though a given source language might have rules about this, with DWARF knock-on)
		 * -- offsets (labels in dwarfidl) are the only reliably unique identifiers
		 * -- nevertheless, using names instead of labels is permitted...
		 * -- ... *iff* they are unambiguous
		 * -- ... according to what resolution algorithm?
		 * Go with the iterated parent-child sequencing, falling back to outermore scopes.
		 * Note that hiding does not imply ambiguity!
		 *
		 * So what should this function return?
		 * Nothing if there's no unambiguous name, something if there is?
		 * What about accepting an argument to denote the context?
		 * No argument means top-level context?
		 */
		return i.name_here();
	}
	

	vector<string> 
	local_name_parts_for(
		iterator_df<> i_d,
		bool use_friendly_names /* = true */)
	{ 
		//if (use_friendly_names && i_d.tag_here() == DW_TAG_base_type)
		//{
		//	auto opt_name = name_for_base_type(i_d.as_a<base_type_die>());
		//	if (opt_name) return vector<string>(1, *opt_name);
		//	else assert(false);
		//}
		//else
		//{
			return vector<string>(1, *dwarfidl_name_from_die(i_d));
		//}
	}

	vector<string> 
	fq_name_parts_for(iterator_df<> p_d)
	{
		if (p_d.offset_here() != 0UL && p_d.depth() != 2)
		{
			auto parts = fq_name_parts_for(p_d.parent());
			parts.push_back(*dwarfidl_name_from_die(p_d));
			return parts;
		}
		else return /*dwarfidl_name_from_die(p_d);*/ local_name_parts_for(p_d);
		/* For simplicity, we want the fq names for base types to be
		 * their C++ keywords, not an alias. So if we're outputting
		 * a CU-toplevel DIE, defer to local_name_for -- we don't do
		 * this in the recursive case above because we might end up
		 * prefixing a C++ keyword with a namespace qualifier, which
		 * wouldn't compile (although presently would only happen in
		 * the strange circumstance of a non-CU-toplevel base type). */
	}
	
	void
	default_print(
		srk31::indenting_ostream& out,
		const iterator_base& i_d
	)
	{
		/* By default, a DIE looks like
		
		   @<label> tag name [attrs] {
		        children...
		   }
		
		 */
		
		Dwarf_Off off = i_d.offset_here();
		Dwarf_Half tag = i_d.tag_here();
		
		out << "@" << std::hex << off << ": "
			<< string(i_d.spec_here().tag_lookup(tag)).substr(sizeof "DW_TAG_" - 1)
			<< (i_d.name_here() ? " " + *i_d.name_here() : "")
			<< " [";

		encap::attribute_map attrs = i_d.dereference().copy_attrs();
		for (auto i_a = attrs.begin(); i_a != attrs.end(); ++i_a)
		{
			if (i_a != attrs.begin()) out << ", ";
			Dwarf_Half attr = i_a->first;
			out << string(i_d.spec_here().attr_lookup(attr)).substr(sizeof "DW_AT_" - 1);
			out << " = " << i_a->second;
		}
		out << "] {";
		
		auto children = i_d.children_here();
		if (srk31::count(children.first, children.second) > 0)
		{
			// out << endl;
			out.inc_level();
			recursively_print_children(out, i_d);
			out.dec_level();
		}
		
		out << "}" << endl;
	}
	
	template <Dwarf_Half Tag>
	void 
	print(
		srk31::indenting_ostream& out,
		const iterator_base& i_d
	)
	{
		assert(i_d.tag_here() == Tag);
		default_print(out, i_d);
	}
	
	template <typename Pred>
	void 
	recursively_print_children(
		indenting_ostream& out,
		const iterator_base& i_d,
		const Pred& pred /* = Pred() */
	)
	{ 
		//out.inc_level(); 
		bool not_yet_inced = true; // we delay inc'ing because it emits a newline...
		auto children = i_d.children_here();
		for (auto i_child = children.first; i_child != children.second; ++i_child)
		{
			// if the caller asked only for a subset of children, we oblige
			if (!pred(i_child)) continue;
			
			// ... and we don't want the newline uness we actually have children
			if (not_yet_inced) { out.inc_level(); not_yet_inced = false; }
			
			// do a dynamic dispatch on the tag
			// (could really be a separate function, rather than the <0> specialization)
			dispatch_to_printer(out, i_child);
		}
		if (!not_yet_inced) out.dec_level(); 
	}

	// explicitly instantiate 0
	template<> void print<0>             (indenting_ostream& out, const iterator_base& i)
	{
		// default_print only works for real DIEs
		if (i != iterator_base::END && i.offset_here() == 0)
		{
			recursively_print_children(out, i);
		}
		else abort();
	}
	
	// define specializations here
	template<> void print<DW_TAG_base_type>             (indenting_ostream& out, const iterator_base& i)
	{


	}
	
	template<> void print<DW_TAG_compile_unit>          (indenting_ostream& out, const iterator_base& i)
	{
		
		recursively_print_children(out, i);

	}

	template<> void print<DW_TAG_subprogram>            (indenting_ostream& out, const iterator_base& i) 
	{
		iterator_df<> i_d = i;
		if (i_d.name_here()) out << *i_d->get_name();

		/* recurse on children -- we do children in a particular order!
		 * formal parameters,
		 * then unspecified parameters,
		 * then others. */
		recursively_print_children(out, i_d, 
			[](const iterator_base& p){ return p.tag_here() == DW_TAG_formal_parameter; }
		);
		recursively_print_children(out, i_d, 
			[](const iterator_base& p){ return p.tag_here() == DW_TAG_unspecified_parameters; }
		);
		recursively_print_children(out, i_d, 
			[](const iterator_base &p){ 
				return p.tag_here() != DW_TAG_unspecified_parameters
				    && p.tag_here() != DW_TAG_formal_parameter; }
		);
		
	}

	template<> void print<DW_TAG_formal_parameter>      (indenting_ostream& out, const iterator_base& i)
	{
	}

	template<> void print<DW_TAG_unspecified_parameters>(indenting_ostream& out, const iterator_base& i)
	{
		
		out << "...";
	}

	template<> void print<DW_TAG_array_type>            (indenting_ostream& out, const iterator_base& i)
	{
	}

	template<> void print<DW_TAG_enumeration_type>      (indenting_ostream& out, const iterator_base& i)
	{
		out << "enum ";
		
		recursively_print_children(out, i);
	}

	template<> void print<DW_TAG_member>                (indenting_ostream& out, const iterator_base& i)
	{
	}

	template<> void print<DW_TAG_pointer_type>          (indenting_ostream& out, const iterator_base& i)
	{

	}
	
	template<> void print<DW_TAG_reference_type>        (indenting_ostream& out, const iterator_base& i)
	{
	}

	template<> void print<DW_TAG_structure_type>        (indenting_ostream& out, const iterator_base& i)
	{

			recursively_print_children(out, i);

	}
	
	template<> void print<DW_TAG_subroutine_type>       (indenting_ostream& out, const iterator_base& i)
	{

	}

	template<> void print<DW_TAG_typedef>               (indenting_ostream& out, const iterator_base& i)
	{

	}

	template<> void print<DW_TAG_union_type>            (indenting_ostream& out, const iterator_base& i)
	{

		recursively_print_children(out, i);
	}

	template<> void print<DW_TAG_const_type>            (indenting_ostream& out, const iterator_base& i)
	{

	}
	
	template<> void print<DW_TAG_constant>              (indenting_ostream& out, const iterator_base& i)
	{}

	template<> void print<DW_TAG_enumerator>            (indenting_ostream& out, const iterator_base& i)
	{

	}
	
	template<> void print<DW_TAG_variable>              (indenting_ostream& out, const iterator_base& i)
	{}

	template<> void print<DW_TAG_volatile_type>         (indenting_ostream& out, const iterator_base& i)
	{

	}
	
	template<> void print<DW_TAG_restrict_type>         (indenting_ostream& out, const iterator_base& i)
	{}
	
	template<> void print<DW_TAG_subrange_type>         (indenting_ostream& out, const iterator_base& i)
	{

	}

} // end namespace tool
} // end namespace dwarf
