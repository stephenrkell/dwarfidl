/* dwarfidl: C++ binding for a useful subset of libdwarf, plus extra goodies.
 * 
 * print.hpp: 
 *
 * Copyright (c) 2009--2014, Stephen Kell.
 */

#ifndef DWARFIDL_PRINT_HPP_
#define DWARFIDL_PRINT_HPP_

#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <sstream>
#include <regex>
#include <srk31/algorithm.hpp>
#include <srk31/indenting_ostream.hpp>
#include <dwarfpp/lib.hpp>

namespace dwarf {
namespace tool {

using namespace dwarf;
using namespace dwarf::core;
using dwarf::lib::Dwarf_Half;
using dwarf::lib::Dwarf_Off;
using std::vector;
using std::pair;
using std::map;
using std::string;
using boost::optional;
using srk31::indenting_ostream;

extern vector<string> reserved_words; // empty for now

string 
ident_name_from_string(const string& s, const char *prefix);

// static function
bool 
is_reserved(const string& word);

// static function
bool 
is_valid_ident(const string& word);

string 
make_valid_ident(const string& word);

string 
name_from_name_parts(const vector<string>& parts);

bool 
type_infixes_name(iterator_df<type_die> t);

opt<string>
dwarfidl_name_from_die(const iterator_base& i);


vector<string> 
local_name_parts_for(
	iterator_df<> p_d,
	bool use_friendly_names = true);

vector<string> 
fq_name_parts_for(iterator_df<> p_d);


template<typename Pred = srk31::True< iterator_base > > 
void 
dispatch_to_printer(
	indenting_ostream& out, 
	const iterator_base& i,
	const Pred& pred = Pred()
);

template <typename Pred = srk31::True< iterator_base > >
void 
recursively_print_children(
	indenting_ostream& out,
	const iterator_base& i,
	const Pred& pred = Pred()
);

void
default_print(
	srk31::indenting_ostream& out,
	const iterator_base& i_d
);

template <Dwarf_Half Tag> 
void
print(
	indenting_ostream& out,
	const iterator_base& r
);

/* specializations of the above */
template<> void print<0>                            (indenting_ostream& out, const iterator_base& i);
template<> void print<DW_TAG_base_type>             (indenting_ostream& out, const iterator_base& i);
template<> void print<DW_TAG_compile_unit>          (indenting_ostream& out, const iterator_base& i);
template<> void print<DW_TAG_subprogram>            (indenting_ostream& out, const iterator_base& i);
template<> void print<DW_TAG_formal_parameter>      (indenting_ostream& out, const iterator_base& i);
template<> void print<DW_TAG_unspecified_parameters>(indenting_ostream& out, const iterator_base& i);
template<> void print<DW_TAG_array_type>            (indenting_ostream& out, const iterator_base& i);
template<> void print<DW_TAG_enumeration_type>      (indenting_ostream& out, const iterator_base& i);
template<> void print<DW_TAG_member>                (indenting_ostream& out, const iterator_base& i);
template<> void print<DW_TAG_pointer_type>          (indenting_ostream& out, const iterator_base& i);
template<> void print<DW_TAG_reference_type>        (indenting_ostream& out, const iterator_base& i);
template<> void print<DW_TAG_structure_type>        (indenting_ostream& out, const iterator_base& i);
template<> void print<DW_TAG_subroutine_type>       (indenting_ostream& out, const iterator_base& i);
template<> void print<DW_TAG_typedef>               (indenting_ostream& out, const iterator_base& i);
template<> void print<DW_TAG_union_type>            (indenting_ostream& out, const iterator_base& i);
template<> void print<DW_TAG_const_type>            (indenting_ostream& out, const iterator_base& i);
template<> void print<DW_TAG_constant>              (indenting_ostream& out, const iterator_base& i);
template<> void print<DW_TAG_enumerator>            (indenting_ostream& out, const iterator_base& i);
template<> void print<DW_TAG_variable>              (indenting_ostream& out, const iterator_base& i);
template<> void print<DW_TAG_volatile_type>         (indenting_ostream& out, const iterator_base& i);
template<> void print<DW_TAG_restrict_type>         (indenting_ostream& out, const iterator_base& i);
template<> void print<DW_TAG_subrange_type>         (indenting_ostream& out, const iterator_base& i);

inline void 
print(
	indenting_ostream& out,
	root_die& r)
{
	print<0>(out, r.begin());
}

/* The dispatch function (template) defined. */
template <typename Pred /* = srk31::True< ... > */ > 
void dispatch_to_printer(
	indenting_ostream& out,
	const iterator_base& i_d,
	const Pred& pred /* = Pred() */)
{
	if (!pred(i_d)) return;

	// otherwise dispatch
	switch(i_d.tag_here())
	{
		case 0:
			assert(false);
	#define CASE(fragment) case DW_TAG_ ## fragment:	\
		print<DW_TAG_ ## fragment>(out, i_d); break; 
		CASE(compile_unit)
		CASE(subprogram)
		CASE(base_type)
		CASE(typedef)
		CASE(structure_type)
		CASE(pointer_type)
		CASE(volatile_type)
		CASE(formal_parameter)
		CASE(array_type)
		CASE(enumeration_type)
		CASE(member)
		CASE(subroutine_type)
		CASE(union_type)
		CASE(const_type)
		CASE(constant)
		CASE(enumerator)
		CASE(variable)
		CASE(restrict_type)
		CASE(subrange_type)   
		CASE(unspecified_parameters)
	#undef CASE
		// The following tags we silently pass over without warning
		//case DW_TAG_condition:
		//case DW_TAG_lexical_block:
		//case DW_TAG_label:
		//	break;
		default:
			default_print(out, i_d);
			break;
	}
}

} // end namespace tool
} // end namespace dwarf

#endif
