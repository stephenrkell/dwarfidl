#include <cstdio>
#include <cassert>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <sstream>
#include <cmath>
#include <cstdlib>
#include <boost/algorithm/string.hpp>
#include <srk31/indenting_ostream.hpp>
#include "dwarfidl/cxx_model.hpp"
#include "dwarfidl/dwarfidl_cxx_target.hpp"
#include "dwarfidl/dwarf_interface_walk.hpp"
#include <srk31/algorithm.hpp>

using namespace srk31;
using namespace dwarf;
using namespace dwarf::lib;
using std::vector;
using std::pair;
using std::set;
using std::map;
using std::multimap;
using std::string;
using std::cerr;
using std::endl;
using std::hex;
using std::dec;
using std::ostringstream;
using std::istringstream;
using std::stack;
using std::deque;
using boost::optional;
using dwarf::core::root_die;
using dwarf::core::compile_unit_die;
using dwarf::core::with_data_members_die;
using dwarf::core::type_chain_die;
using dwarf::core::variable_die;
using dwarf::core::member_die;
using dwarf::core::program_element_die;
using dwarf::spec::opt;

static std::ostream& debug(int msg_level = 1)
{
	static std::ofstream dummy;
	char *level_str = getenv("DWARFIDL_DEBUG_LEVEL");
	int level = (level_str ? atoi(level_str) : 0);
	if (level >= msg_level) return cerr;
	return dummy;
}

namespace dwarf { namespace tool {

void dwarfidl_cxx_target::transitively_close(
	set<std::pair<dwarfidl_cxx_target::emit_kind, iterator_base>, dwarfidl_cxx_target::compare_with_type_equality > const& to_output,
	cxx_generator_from_dwarf::referencer_fn_t maybe_get_name,
	/* TODO: accept 'definer', e.g. for noopgen? */
	map<std::pair<dwarfidl_cxx_target::emit_kind, iterator_base>, string, dwarfidl_cxx_target::compare_with_type_equality >& output_fragments,
	multimap< std::pair<dwarfidl_cxx_target::emit_kind, iterator_base>, std::pair<dwarfidl_cxx_target::emit_kind, iterator_base>, dwarfidl_cxx_target::compare_with_type_equality >& order_constraints
)
{
	// 1. pre-populate -- this is something the client tool does
	// by giving us a set<pair<emit_kind, iterator_base> >,
	// but we turn it into a vector so that we can use it as a worklist (add to the back)
	// ... and we also keep a set representation so we can test for already-presentness
	set<std::pair<emit_kind, iterator_base>, compare_with_type_equality > output_expanded;
	list <std::pair<emit_kind, iterator_base> > worklist;
	opt<std::pair<emit_kind, iterator_base>> seen_size_t; // HACK for debugging
	auto maybe_add_to_worklist = [&]( std::pair<emit_kind, iterator_base> const& el) {
		// only add something if it's new
		debug() << "Maybe adding to worklist: " << el.first << " of " << el.second.summary()
			<< std::endl;
		bool must_not_insert = false;
		auto found = output_expanded.find(el);
		if (el.second.name_here() && *el.second.name_here() == "size_t") // HACK for debugging
		{
			if (seen_size_t) must_not_insert = true; // second time and later must not insert
		}
		auto inserted = output_expanded.insert(el);
		if (el.second.name_here() && *el.second.name_here() == "size_t") // HACK for debugging
		{
			if (!seen_size_t)
			{
				debug() << "First size_t being inserted: " << inserted.first->second << endl;
				seen_size_t = *inserted.first;
			}
		}
		if (!inserted.second)
		{
			debug() << "Actually didn't insert because it's already in output_expanded ("
				<< inserted.first->second.summary() << ")" << endl;
			return; // nothing was inserted, i.e. it was already present
		}
		if (must_not_insert)
		{
			// if we got here, it means we really did insert
			debug() << "Did not expect to insert but did: " << el.first << " of " << el.second << endl;
			debug() << "Already-to-output size_t: "
				<< seen_size_t->first << " of " << seen_size_t->second << endl
				<< "Does it equal the one to insert? it is less than el? "
						<< std::boolalpha
						<< dwarfidl_cxx_target::compare_with_type_equality()(*seen_size_t, el)
						<< "; el is less than it? "
						<< std::boolalpha
						<< dwarfidl_cxx_target::compare_with_type_equality()(el, *seen_size_t)
						<< endl;
			assert(!inserted.second); // always fails
		}
		worklist.push_back(el);
	};
	for (auto el : to_output) maybe_add_to_worklist(el);
	auto add_order_constraint = [&order_constraints](
		const std::pair<dwarfidl_cxx_target::emit_kind, iterator_base>& from,
		const std::pair<dwarfidl_cxx_target::emit_kind, iterator_base>& to
	) {
		auto new_ent = make_pair(from, to);
		/* Here we insert while suppressing duplicates.
		 * Care: the range over which the key is equal is wider than the range over
		 * which the value is equal. */
		auto key_equal_range = order_constraints.equal_range(from);
		for (auto i_r = key_equal_range.first; i_r != key_equal_range.second; ++i_r)
		{
			if (i_r->second == to) return;
		}
		order_constraints.insert(new_ent);
	};

	// 2. transitively close -- this is something we do. How? By calling
	// the printer and adding to to_emit the named dependencies it tells us about.
	// Once we've printed everything in to_emit, we have full ordering info.
	while (!worklist.empty())
	{
		/* Traverse the whole worklist... we may append to the list during the loop. */
		auto i_pair = worklist.begin();
		while (i_pair != worklist.end())
		{
			auto the_pair = *i_pair;
			auto maybe_get_name_tracking_deps
			= [the_pair, maybe_add_to_worklist, add_order_constraint, maybe_get_name]
			  (iterator_base i, enum ref_kind k) -> opt<string> {
				opt<string> ret = maybe_get_name(i, k);
				if (!ret || k == cxx_generator::DEFINING) return ret;
				// we shouldn't have got a generated name for 'void'
				assert(!!i);
				/* If we generate a name, record it as a dependency
				 * against the current DIE. We rely on the caller
				 * to tell us if they require a complete type. Otherwise
				 * just a declaration will suffice. HMM. "Declared before"
				 * vs "declared anywhere" is a distinction we don't capture. */
				if (k == cxx_generator::TYPE_MUST_BE_COMPLETE)
				{
					// if the referred-to type is concrete i.e. not a qualified
					// or typedef, just add the DEF dependency
					if (i && i.is_a<type_die>()
						&& i.as_a<type_die>()->get_concrete_type() == i)
					{
						auto new_pair = make_pair(EMIT_DEF, i);
						add_order_constraint(the_pair, new_pair);
						maybe_add_to_worklist(new_pair);
					}
					else if (i && i.is_a<type_die>())
					{
						// the typedef must come first, but also...
						// FIXME: should this add everything on the chain, not just
						// the concrete?
						auto immediate_pair = make_pair(EMIT_DECL, i);
						add_order_constraint(the_pair, immediate_pair);
						maybe_add_to_worklist(immediate_pair);
						auto concrete_pair = make_pair(EMIT_DEF, 
							i.as_a<type_die>()->get_concrete_type());
						add_order_constraint(the_pair, concrete_pair);
					}
					else
					{
						// anything here?
						abort();
					}
				}
				else
				{
					auto default_pair = make_pair(EMIT_DECL, i);
					add_order_constraint(the_pair, default_pair);
					maybe_add_to_worklist(default_pair);
				}
				return ret;
			}; // end our namer
			string to_add;
			switch (the_pair.first)
			{
				case EMIT_DECL: {
					ostringstream s;
					s << decl_of_die(
						the_pair.second,
						maybe_get_name_tracking_deps,
						/* emit_fp_names = */ true,
						/* emit_semicolon = */ true
					);
					// s << " /* " << the_pair.second.summary() << " */" << endl;
					output_fragments[the_pair] = s.str();
				} break;
				case EMIT_DEF: {
					ostringstream s;
					indenting_ostream out(s);
					out << defn_of_die(
						the_pair.second,
						maybe_get_name_tracking_deps,
						/* override_name = */ opt<string>(),
						/* emit_fp_names = */ true,
						/* semicolon */ false,
						/* body = */ string()
					);
					// s << " /* " << the_pair.second.summary() << " */" << endl;
					output_fragments[the_pair] = s.str();
				} break;
				default:
					assert(false); abort();
			}
			// we've just processed one worklist item
			i_pair = worklist.erase(i_pair);
		} // end while worklist
		// if we're about to terminate, we should not have seen any missing fragment
	}
	
	// check invariant: anything that's depended on must also be emissible
	for (auto i_pair = order_constraints.begin(); i_pair != order_constraints.end(); ++i_pair)
	{
		assert(output_fragments.find(i_pair->second) != output_fragments.end());
	}
	// now the caller has the output fragments and order constraints
}

std::ostream& operator<<(std::ostream& s, const dwarfidl_cxx_target::emit_kind& k)
{
	switch (k)
	{
		case dwarfidl_cxx_target::EMIT_DECL: s << "decl"; break;
		case dwarfidl_cxx_target::EMIT_DEF:  s << "def"; break;
		default: abort();
	}
	return s;
}
void dwarfidl_cxx_target::write_ordered_output(
		map<pair<emit_kind, iterator_base>, string, compare_with_type_equality > output_fragments /* by value: copy */,
		multimap< pair<emit_kind, iterator_base>, pair<emit_kind, iterator_base>, compare_with_type_equality >& order_constraints
	)
{
	std::ostream& out = std::cout; /* FIXME: take as arg */
	set <pair<emit_kind, iterator_base>, compare_with_type_equality > emitted;

	multimap< pair<emit_kind, iterator_base>, pair<emit_kind, iterator_base>, compare_with_type_equality >
	 order_constraints_reversed;
	for (auto p : order_constraints) order_constraints_reversed.insert(make_pair(p.second, p.first));

	/* We output what we can, until there's nothing left. */
	while (!output_fragments.empty())
	{
		unsigned i = 0;
		bool removed_one = false;
		auto i_pair = output_fragments.begin();
		while (i_pair != output_fragments.end())
		{
			auto& el = i_pair->first;
			auto& frag = i_pair->second;
			/* Can we emit this? */
			auto deps_seq = order_constraints.equal_range(el);
			bool all_sat = true;
			for (auto i_dep = deps_seq.first; i_dep != deps_seq.second; ++i_dep)
			{
				auto& dep = i_dep->second;
				bool already_emitted = (emitted.find(dep) != emitted.end());
				all_sat &= already_emitted;
				if (!already_emitted)
				{
					debug() << "Can't emit " << el.first << " of " << el.second << " yet"
						<< " because not yet emitted (maybe among others) "
						<< dep.first << " of " << dep.second
						<< "; continuing (total " << output_fragments.size()
						<< " fragments remaining of which we are number " << i << ")" << endl;
					assert(!all_sat);
					break;
				}
			}
			if (all_sat)
			{
				// emit this fragment
				out << frag;
				// add to the emitted set
				auto retpair = emitted.insert(el);
				assert(retpair.second); // really inserted
				// first remove from order constraints any constraint depending on what we just emitted
				auto depending_on_this = order_constraints_reversed.equal_range(i_pair->first);
				for (auto i_rev = depending_on_this.first;
					i_rev != depending_on_this.second;
					++i_rev)
				{
					auto& depending = i_rev->second;
					auto& depended_on = i_rev->first;
					/* Careful: we only want to erase those constraints that
					 * both have key 'depending' and have value 'depended on' */
					auto pos = order_constraints.lower_bound(depending);
					while (pos->first == depending)
					{
						if (pos->second == depended_on) pos = order_constraints.erase(pos);
						else ++pos;
					}
				}
				// now remove from output fragments
				i_pair = output_fragments.erase(i_pair);
				removed_one = true;
			}
			else /* continue to next */ { ++i_pair; ++i; }
		}
		if (!removed_one)
		{
			debug(0) << "digraph stuck_with_order_constraints {" << endl;
			for (auto pair : order_constraints)
			{
				debug() << pair.first.first << "_of_" << std::hex;
				if (pair.first.second) debug(0) << pair.first.second.offset_here(); else debug(0) << "void";
				debug(0) << " -> "
				     << pair.second.first << "_of_" << std::hex;
				if (pair.second.second) debug(0) << pair.second.second.offset_here(); else debug(0) << "void";
				debug(0) << "; //" << pair.first.second << " -> " << pair.second.second << endl;
			}
			debug(0) << "}" << endl;
			assert(false); abort();
		} /* lack of progress */
	}
}

} } // end namespace dwarf::tool
