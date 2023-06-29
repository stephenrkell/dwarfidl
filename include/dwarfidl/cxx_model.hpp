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
using std::ostream;
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
using std::ostringstream;
using boost::optional;
using srk31::indenting_ostream;

/** This class contains generally useful stuff for generating C++ code.
 *  It should be free from DWARF details. FIXME: move it to libcxxgen. */
class cxx_generator
{
public:
	enum ref_kind { NORMAL, TYPE_MUST_BE_COMPLETE, DEFINING };
	static const vector<string> cxx_reserved_words; // FIXME: move to toplevel in libcxxgen
	static bool is_reserved(const string& word);
	static bool is_valid_cxx_ident(const string& word);
	virtual string language_linkage_name() const { return "C++"; }
	virtual string make_valid_cxx_ident(const string& word) const;
	// FIXME: overlaps with namer/referencer; eliminate?
	virtual string cxx_name_from_string(const string& s) const;
	virtual string name_from_name_parts(const vector<string>& parts) const;
};

/** This class implements a mapping from DWARF constructs to C++ constructs,
 *  and utility functions for understanding the C++ constructs corresponding
 *  to various DWARF elements. */
class cxx_generator_from_dwarf : public cxx_generator
{
protected:
	virtual string get_anonymous_prefix() const
	{ return "_dwarfhpp_anon_"; } // HACK: remove hard-coding of this in libdwarfpp, dwarfhpp and cake
	virtual string get_untyped_argument_typename() const = 0;

public:
	const spec::abstract_def *const p_spec;
	typedef std::function< opt<string>(iterator_base, ref_kind) > referencer_fn_t;
	referencer_fn_t get_default_referencer() const;

	cxx_generator_from_dwarf() : p_spec(&spec::DEFAULT_DWARF_SPEC) {}
	cxx_generator_from_dwarf(const spec::abstract_def& s) : p_spec(&s) {}

	bool is_builtin(iterator_df<> p_d); // FIXME: belongs with compiler? no because DWARFy
	
	virtual 
	optional<string>
	name_for_base_type(iterator_df<base_type_die>) const = 0;
	virtual bool use_friendly_base_type_names() const { return true; }

	virtual bool use_struct_and_union_prefixes() const { return true; }

	virtual 
	optional<string>
	prefix_for_all_idents() const { return optional<string>(); };

	bool 
	cxx_type_can_be_qualified(iterator_df<type_die> p_d) const;

	bool 
	cxx_type_can_have_name(iterator_df<type_die> p_d) const;

	bool 
	cxx_assignable_from(
		iterator_df<type_die> dest,
		iterator_df<type_die> source
	);

	bool 
	cxx_is_complete_type(iterator_df<type_die> t);

	string 
	name_for_argument(
		iterator_df<formal_parameter_die> p_d, 
		int argnum);

	string
	make_function_declaration_of_type(
		iterator_df<type_describing_subprogram_die> p_d,
		const string& name,
		bool write_semicolon = true,
		bool wrap_with_extern_lang = true
	);

	/* We package some of our functionality as stream manipulators. */
	typedef std::function< indenting_ostream&( indenting_ostream& ) > strmanip_t;

	/* Referencer objects encapsulate how to emit named references to C++
	 * declarations, and indeed whether this can be done. This affects
	 * which DIEs are emitted anonymously versus with names, what names
	 * are used, any prefixing or other rewriting of names, etc.*/
#if 0
	struct referencer
	{
		cxx_generator_from_dwarf &gen;
		bool use_friendly_base_type_names;
		opt<string> extra_prefix;
		bool use_struct_and_union_prefixes;

		/* HACK: require callers to tell us if they need the name
		 * to be the name of a complete type. It's a hack because
		 * we have no need of this data; it's purely to benefit the
		 * subclass dep_tracking_referencer in dwarfidl_cxx_target. */
		virtual opt<string> can_name(iterator_base i, bool must_be_complete_type = false) const;
		//string name(iterator_base i, bool must_be_complete_type = false) const
		//{ return *can_name(i, must_be_complete_type); }
		struct stream : ostringstream
		{
			const referencer& ref;
			stream(const referencer& ref) : ref(ref) {}
			stream(stream&& s) : ref(s.ref) {}

			ostringstream&       wrapped()       { return static_cast<ostringstream&>(*this); }
			ostringstream const& wrapped() const { return static_cast<const ostringstream&>(*this); }
			
			// any string we receive should be a C++ token [sequence] but this isn't checked
			stream& operator<<(const string& s)
			{ wrapped() << s; return *this; }

			stream& operator<<(const char *s)
			{ wrapped() << s; return *this; }
			stream& operator<<(iterator_base i)
			{ wrapped() << *ref.can_name(i); return *this; }
			/* generic but for 'any scalar type' only, to avoid overlap
			 * with our iterator_base case. */
			template
			<typename Any,
			 typename dummy = typename std::enable_if< std::is_scalar<Any>::value >::type >
			stream& operator<<(Any s)
			{ wrapped() << s; return *this; }

			// support our own kind of I/O manipulators
			stream& operator<<(std::function<stream&(stream&)> m)
			{ m(*this); return *this; }
			/* HACK: support standard I/O manipulators */
			stream& operator<<(std::ios_base&(*m)(std::ios_base&))
			{ m(*this); return *this; }
			stream& operator<<(std::ostream&(*m)(std::ostream&))
			{ m(*this); return *this; }
		}; // end stream
	};
#endif
	
	string
	decl_having_type(
		iterator_df<type_die> t,
		const string& name,
		referencer_fn_t r,
		bool emit_fp_names = true
	);
	string
	decl_of_die(
		iterator_df<core::program_element_die> d,
		referencer_fn_t r,
		bool emit_fp_names = false,
		bool write_semicolon = true
	);
	srk31::indenting_ostream&
	emit_definition(iterator_base i_d,
		srk31::indenting_ostream& out,
		referencer_fn_t namer
	);
	// same but packaged as a stream manipulator -- FIXME?
	strmanip_t defn_of_die(
		iterator_df<core::program_element_die> i_d,
		referencer_fn_t r,
		opt<string> override_name = opt<string>(),
		bool emit_fp_names = true,
		string body = string()
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
	optional<string> name_for_base_type(iterator_df<base_type_die> p_d) const;
};

} // end namespace tool
} // end namespace dwarf

#endif
