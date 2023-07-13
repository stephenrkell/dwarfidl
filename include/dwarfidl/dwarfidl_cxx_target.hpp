#ifndef DWARFIDL_CXX_DEPENDENCY_ORDER_HPP_
#define DWARFIDL_CXX_DEPENDENCY_ORDER_HPP_

#include <set>
#include <utility>
#include <map>

namespace dwarf { namespace tool { 

using std::set;
using std::pair;
using std::multimap;
using std::string;
using std::vector;

using namespace dwarf;
using namespace dwarf::lib;

/* This class needs to become roughly a reusable base for
 * dwarfhpp, noopgen, and any similar tools.
 * What does it do, according to that?
 * It provides a way to emit a C++ representation of a set of DIEs (a slice),
 * without fixing too much what that representation is.
 * E.g. in liballocstool we could even use it to emit uniqtypes, perhaps,
 * and keep track of ordering constraints between those (HMM).
 * 
 * Currently it also wraps cxx_target supplying some unimportant stuff
 * (untyped_argument_typename). This seems tool-specific (goes in dwarfhpp?).
 */

class dwarfidl_cxx_target : public cxx_target
{
	const string m_untyped_argument_typename;
	srk31::indenting_ostream& out;
public:	
	//static const vector<string> default_compiler_argv;
	dwarfidl_cxx_target(
		const string& untyped_argument_typename,
		srk31::indenting_ostream& out,
		const vector<string>& compiler_argv = default_compiler_argv(true)
	)
	 : cxx_target(compiler_argv), 
	m_untyped_argument_typename(untyped_argument_typename), 
	out(out)
	{}

	string get_untyped_argument_typename() const { return m_untyped_argument_typename; }

	enum emit_kind {
		EMIT_DECL,
		EMIT_DEF
	};

	//typedef std::less< pair<emit_kind, iterator_base> > compare_with_type_equality;
	struct compare_with_type_equality
	{
		bool operator()(const pair<emit_kind, iterator_base>& p1,
		                const pair<emit_kind, iterator_base>& p2) const
		{
			if (p1.first < p2.first) return true;
			if (p1.first > p2.first) return false;
			// from here, we know the 'first's are equal
			return iterator_base::less_by_type_equality()(p1.second, p2.second);
		}
	};

	void transitively_close(
		set<pair<emit_kind, iterator_base>, compare_with_type_equality > const& to_output,
		cxx_generator_from_dwarf::referencer_fn_t r,
		map<pair<emit_kind, iterator_base>, string, compare_with_type_equality >& output_fragments,
		multimap< pair<emit_kind, iterator_base>, pair<emit_kind, iterator_base>, compare_with_type_equality >& order_constraints
	);

	void write_ordered_output(
		map<pair<emit_kind, iterator_base>, string, compare_with_type_equality > output_fragments,
		multimap< pair<emit_kind, iterator_base>, pair<emit_kind, iterator_base>, compare_with_type_equality >& order_constraints
	);
};

/* close namespaces */
} } 
#endif
