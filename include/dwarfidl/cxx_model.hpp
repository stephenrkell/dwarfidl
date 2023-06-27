/* dwarfpp: C++ binding for a useful subset of libdwarf, plus extra goodies.
 * 
 * cxx_model.hpp: tools for constructing binary-compatible C++ models 
 * 			of DWARF elements.
 *
 * Copyright (c) 2009--2012, Stephen Kell.
 */

#ifndef DWARFPP_CXX_MODEL_HPP_
#define DWARFPP_CXX_MODEL_HPP_

#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <sstream>
#include <regex>
#include <type_traits>
#include <srk31/algorithm.hpp>
#include <srk31/indenting_ostream.hpp>

#include <cxxgen/cxx_compiler.hpp>
#include <dwarfpp/spec.hpp>
#include <dwarfpp/lib.hpp>

namespace dwarf {
namespace tool {

using namespace dwarf;
using dwarf::lib::Dwarf_Half;
using dwarf::lib::Dwarf_Off;
using dwarf::core::root_die;
using dwarf::core::iterator_base;
using dwarf::core::iterator_df;
using dwarf::core::basic_die;
using dwarf::core::type_die;
using dwarf::core::base_type_die;
using dwarf::core::subprogram_die;
using dwarf::core::subroutine_type_die;
using dwarf::core::type_describing_subprogram_die;
using dwarf::core::formal_parameter_die;
using dwarf::spec::opt;

using std::cerr;
using std::vector;
using std::pair;
using std::map;
using std::string;
using boost::optional;
using srk31::indenting_ostream;

/** This class contains generally useful stuff for generating C++ code.
 *  It should be free from DWARF details. FIXME: move it to libcxxgen. */
class cxx_generator
{
public:
	static const vector<string> cxx_reserved_words;
	static bool is_reserved(const string& word);
	static bool is_valid_cxx_ident(const string& word);
	virtual string make_valid_cxx_ident(const string& word);
	virtual string cxx_name_from_string(const string& s, const char *prefix);
	virtual string name_from_name_parts(const vector<string>& parts);
};

/** This class implements a mapping from DWARF constructs to C++ constructs,
 *  and utility functions for understanding the C++ constructs corresponding
 *  to various DWARF elements. */
class cxx_generator_from_dwarf : public cxx_generator
{
protected:
	virtual string get_anonymous_prefix()
	{ return "_dwarfhpp_anon_"; } // HACK: remove hard-coding of this in libdwarfpp, dwarfhpp and cake
	virtual string get_untyped_argument_typename() = 0;

public:
	const spec::abstract_def *const p_spec;

	cxx_generator_from_dwarf() : p_spec(&spec::DEFAULT_DWARF_SPEC) {}
	cxx_generator_from_dwarf(const spec::abstract_def& s) : p_spec(&s) {}

	bool is_builtin(iterator_df<> p_d); // FIXME: belongs with compiler? no because DWARFy
	
	virtual 
	optional<string>
	name_for_base_type(iterator_df<base_type_die>) = 0;

	bool 
	type_infixes_name(iterator_df<basic_die> p_d);

	vector<string> 
	fq_name_parts_for(iterator_df<basic_die> p_d);

	bool 
	cxx_type_can_be_qualified(iterator_df<type_die> p_d) const;

	bool 
	cxx_type_can_have_name(iterator_df<type_die> p_d) const;

	pair<string, bool>
	cxx_decl_from_type_die(
		iterator_df<type_die> p_d, 
		spec::opt<string> infix_typedef_name = spec::opt<string>(),
		bool use_friendly_names = true,
		spec::opt<string> extra_prefix = spec::opt<string>(),
		bool use_struct_and_union_prefixes = true
	);

	bool 
	cxx_assignable_from(
		iterator_df<type_die> dest,
		iterator_df<type_die> source
	);

	bool 
	cxx_is_complete_type(iterator_df<type_die> t);
	
	pair<string, bool>
	name_for_type(
		iterator_df<type_die> p_d, 
		optional<string> infix_typedef_name = optional<string>(),
		bool use_friendly_names = true,
		optional<string> extra_prefix = optional<string>(),
		bool use_struct_and_union_prefixes = true
		);

	string 
	name_for_argument(
		iterator_df<formal_parameter_die> p_d, 
		int argnum);

	string
	make_typedef(
		iterator_df<type_die> p_d,
		const string& name 
	);
	
	string
	make_function_declaration_of_type(
		iterator_df<type_describing_subprogram_die> p_d,
		const string& name,
		bool write_semicolon = true,
		bool wrap_with_extern_lang = true
	);

	struct referencing_ostream
	{
	private:
		std::ostringstream super;
	public:
		cxx_generator_from_dwarf &gen;
		bool use_friendly_base_type_names;
		opt<string> extra_prefix;
		bool use_struct_and_union_prefixes;

		referencing_ostream(cxx_generator_from_dwarf &gen,
			bool use_friendly_base_type_names,
			opt<string> extra_prefix,
			bool use_struct_and_union_prefixes
		) : gen(gen), use_friendly_base_type_names(use_friendly_base_type_names),
		    extra_prefix(extra_prefix),
			use_struct_and_union_prefixes(use_struct_and_union_prefixes)
		{}
		
		string str() const { return this->super.str(); }
		/* Can't do this because we can't */
		referencing_ostream fresh_context() const
		{ return std::move(referencing_ostream(this->gen,
			this->use_friendly_base_type_names,
			this->extra_prefix, this->use_struct_and_union_prefixes)); }

		opt<string> can_refer_to(iterator_base i);

		// any string we receive should be a C++ token
		referencing_ostream& operator<<(const string& s)
		{ this->super << s; return *this; }
		referencing_ostream& operator<<(const char *s)
		{ this->super << s; return *this; }
		/* PROBLEM: we want this operator<< to override any
		 * operator<< that is defined on iterator_base and its subclasses.
		 * Since only we know that we are an ostream, that seems simple,
		 * yet I am seeing the wrong kind of printout emerging.  */
		referencing_ostream& operator<<(iterator_base i)
		{ this->super << *can_refer_to(i); return *this; }
		/* generic but for 'any scalar type' only, to avoid overlap
		 * with our iterator_base case. */
		template <typename Any, typename dummy = typename std::enable_if< std::is_scalar<Any>::value >::type >
		referencing_ostream& operator<<(Any s)
		{ this->super << s; return *this; }
	};
	
	string
	make_declaration_having_type(
		iterator_df<type_die> p_d,
		const std::string& name_in,
		referencing_ostream& namer,
		bool emit_fp_names = true
	);

	srk31::indenting_ostream&
	emit_definition(iterator_base i_d,
		srk31::indenting_ostream& out,
		referencing_ostream& namer
	);

	string 
	create_ident_for_anonymous_die(
		iterator_df<basic_die> p_d
	);

	string 
	protect_ident(const string& ident);

protected:
	virtual 
	iterator_df<type_die>
	transform_type(
		iterator_df<type_die> t,
		const iterator_base& context
	)
	{
		return t;
	}

};

/** This class supports generation of C++ code targetting a particular C++ compiler
 *  (including any command-line options that are relevant to codegen).
 *  It uses DWARF only to understand the base types of the compiler. */
class cxx_target : public cxx_generator_from_dwarf, public cxx_compiler
{
public:
	// forward base constructors
	cxx_target(const vector<string>& argv) : cxx_compiler(argv) {}

	cxx_target(const spec::abstract_def& s, const vector<string>& argv)
	 : cxx_generator_from_dwarf(s), cxx_compiler(argv) {}

	cxx_target(const spec::abstract_def& s)
	 : cxx_generator_from_dwarf(s) {}

	cxx_target() {}
	
	// implementation of pure virtual function in cxx_generator_from_dwarf
	optional<string> name_for_base_type(iterator_df<base_type_die> p_d);
};

} // end namespace tool
} // end namespace dwarf

#endif
