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

class dependency_ordering_cxx_target : public cxx_target
{
	const string m_untyped_argument_typename;
	srk31::indenting_ostream& out;
public:	
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
private:
	set<pair<emit_kind, iterator_base>, compare_with_type_equality > m_to_output;
	map<pair<emit_kind, iterator_base>, string, compare_with_type_equality > m_output_fragments;
	multimap< pair<emit_kind, iterator_base>, pair<emit_kind, iterator_base>, compare_with_type_equality > m_order_constraints;
	
	/* XXX: bit fragile: these are used in one-shot fashion during transitively_close
	 * (used to be locals) */
	set<std::pair<emit_kind, iterator_base>, compare_with_type_equality > output_expanded;
	list <std::pair<emit_kind, iterator_base> > worklist;
	void maybe_add_to_worklist(std::pair<emit_kind, iterator_base> const& el);
	void add_order_constraint(
		const std::pair<emit_kind, iterator_base>& from,
		const std::pair<emit_kind, iterator_base>& to
	);
	// HACK: we used to capture this when our maybe_get_name was a lambda,
	// but since we can't when it's a method, we need to use a local
	pair<emit_kind, iterator_base> current_pair_transitively_closing_from;
public:
	//static const vector<string> default_compiler_argv;
	dependency_ordering_cxx_target(
		const string& untyped_argument_typename,
		srk31::indenting_ostream& out,
		set<pair<emit_kind, iterator_base>, compare_with_type_equality > const& to_output,
		const vector<string>& compiler_argv = default_compiler_argv(true)
	)
	 : cxx_target(compiler_argv),
	m_untyped_argument_typename(untyped_argument_typename), 
	out(out),
	m_to_output(to_output)
	{}
	map<pair<emit_kind, iterator_base>, string, compare_with_type_equality > const&
	output_fragments() const { return m_output_fragments; }

	// override
	opt<string> maybe_get_name(iterator_base i, enum ref_kind k);

	void transitively_close();
	void write_ordered_output();
};

/* close namespaces */
} } 
#endif
