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
#include "dwarfidl/dependency_ordering_cxx_target.hpp"
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

/* FIXME: this mixes together the task of generating snippets of code
 * with the core task of elaborating a dependency graph. These are really coroutines in
 * a sense: generating a snippet will generate outgoing references, and
 * these in turn need to be generated, and so on.
 *
 * It also bothers me that we are passing the namer around rather than just
 * using an overridden virtual function. Can we make the worklist and expanded output list
 * object-level state, of a subclass transitively_closing_dwarfidl_cxx_target or similar?
 * Does this help with unbundling the two coroutines? Rather than the caller passing in
 * the maps of output fragments and ordering constraints, they would pull these out
 * at the end.
 *
 * This is a good idea because e.g. noopgen can override defn_of_die to output the body
 * that it wants to generate. The current practice of passing a 'body' string does not
 * work. OK, fixed by adding a virtual member function that generates the body.
 *
 * In what way is this target a 'dwarfidl' target? It's not, except that it resides
 * in libdwarfidl. Could it even go in libcxxgen, maybe with the fragment generator pure-virtual?
 * Yes, and the dependency tracking could be lifted up to be DWARF-agnostic level,
 * just like the mooted cxx_generator_from_model<DwarfModel>. However, this is too much for now.
 */
void dependency_ordering_cxx_target::maybe_add_to_worklist(std::pair<emit_kind, iterator_base> const& el)
{
	// only add something if it's new
	debug() << "Maybe adding to worklist: " << el.first << " of " << el.second.summary()
		<< std::endl;
	bool must_not_insert = false;
	auto found = output_expanded.find(el);
	auto inserted = output_expanded.insert(el);
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
		assert(!inserted.second); // always fails
	}
	worklist.push_back(el);
}
void dependency_ordering_cxx_target::add_order_constraint(
		const std::pair<dependency_ordering_cxx_target::emit_kind, iterator_base>& from,
		const std::pair<dependency_ordering_cxx_target::emit_kind, iterator_base>& to
	) 
{
	auto new_ent = make_pair(from, to);
	/* Here we insert while suppressing duplicates.
	 * Care: the range over which the key is equal is wider than the range over
	 * which the value is equal... can't assume values are all the same, so
	 * to suppress duplicate values we may have to look at the whole key-equal range. */
	auto key_equal_range = m_order_constraints.equal_range(from);
	for (auto i_r = key_equal_range.first; i_r != key_equal_range.second; ++i_r)
	{
		if (i_r->second == to) return;
	}
	m_order_constraints.insert(new_ent);
}	

opt<string>
dependency_ordering_cxx_target::maybe_get_name(iterator_base i, enum ref_kind k)
{
	opt<string> ret = this->cxx_target::maybe_get_name(i, k);
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
			add_order_constraint(this->current_pair_transitively_closing_from, new_pair);
			maybe_add_to_worklist(new_pair);
		}
		else if (i && i.is_a<type_die>())
		{
			// the typedef must come first, but also...
			// FIXME: should this add everything on the chain, not just
			// the concrete?
			auto immediate_pair = make_pair(EMIT_DECL, i);
			add_order_constraint(this->current_pair_transitively_closing_from,
				immediate_pair);
			maybe_add_to_worklist(immediate_pair);
			auto concrete_pair = make_pair(EMIT_DEF, 
				i.as_a<type_die>()->get_concrete_type());
			add_order_constraint(this->current_pair_transitively_closing_from,
				concrete_pair);
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
		add_order_constraint(this->current_pair_transitively_closing_from, default_pair);
		maybe_add_to_worklist(default_pair);
	}
	return ret;
} // end our namer

void dependency_ordering_cxx_target::transitively_close()
{
	// 1. pre-populate -- this is something the client tool does
	// by giving us a set<pair<emit_kind, iterator_base> >,
	// but we turn it into a vector so that we can use it as a worklist (add to the back)
	// ... and we also keep a set representation so we can test for already-presentness
	opt<std::pair<emit_kind, iterator_base>> seen_size_t; // HACK for debugging
	for (auto el : m_to_output) maybe_add_to_worklist(el);

	// 2. transitively close -- this is something we do. How? By calling
	// the printer and adding to to_emit the named dependencies it tells us about.
	// Once we've printed everything in to_emit, we have full ordering info.
	while (!worklist.empty())
	{
		/* Traverse the whole worklist... we may append to the list during the loop. */
		auto i_pair = worklist.begin();
		map<string, iterator_df<type_die> > fragments_by_cxx_name;
		while (i_pair != worklist.end())
		{
			/* We need to set per-object state because decl_of_die and defn_of_die
			 * will call maybe_get_name, and it may want to know (for dependency-
			 * tracking purposes) what we're currently working on. */
			this->current_pair_transitively_closing_from = *i_pair;
			ostringstream s;
			switch (this->current_pair_transitively_closing_from.first)
			{
				case EMIT_DECL: {
					s << decl_of_die(
						this->current_pair_transitively_closing_from.second,
						/* emit_fp_names = */ true,
						/* emit_semicolon = */ true
					);
				}
				if (this->current_pair_transitively_closing_from.second.is_a<type_die>())
				{ goto check_for_duplicates; } else break;
				case EMIT_DEF: {
					indenting_ostream out(s);
					/* To track down "same type, different summary code and therefore unequal"
					 * cases, where we emit multiple definitions that conflict owing to
					 * having the same name, let's remember the "C++ def name" of this
					 * DIE and report if we see a duplicate. Getting the "C++ def name"
					 * must account for how, say, 'struct foo' and 'foo' don't conflict
					 * because the struct namespace can still disambiguate. */
					out << defn_of_die(
						this->current_pair_transitively_closing_from.second,
						/* override_name = */ opt<string>(),
						/* emit_fp_names = */ true,
						/* semicolon */ false
					);
				} goto check_for_duplicates;
				check_for_duplicates: {
					opt<string> cxx_name = maybe_get_name(
						this->current_pair_transitively_closing_from.second,
						DEFINING
					);
					if (cxx_name)
					{
						auto retpair = fragments_by_cxx_name.insert(make_pair(*cxx_name,
							this->current_pair_transitively_closing_from.second
						));
						/* Did we really insert it? if we didn't,
						 * let's emit a warning. */
						if (!retpair.second)
						{
							cerr << "Warning: duplicate C++ name for distinct DIEs: current "
								<< this->current_pair_transitively_closing_from.second.summary()
								<< ", previous " << retpair.first->second.summary()
								<< endl;
						}
						else
						{
							// cerr << "Blah: something of C++ name " << *cxx_name << endl;
						}
					}
				}
				break;
				default:
					assert(false); abort();
			}
			// s << " /* " << this->current_pair_transitively_closing_from.second.summary() << " */" << endl;
			string frag = s.str();
			if (frag.length() == 0) std::cerr << "WARNING: generated empty fragment for " <<
				this->current_pair_transitively_closing_from.second.summary()
				<< std::endl;
			m_output_fragments[this->current_pair_transitively_closing_from] = frag;
			// we've just processed one worklist item
			i_pair = worklist.erase(i_pair);
		} // end while worklist
	}
	// check invariant: anything that's depended on must also be emissible
	for (auto i_pair = m_order_constraints.begin(); i_pair != m_order_constraints.end(); ++i_pair)
	{
		assert(m_output_fragments.find(i_pair->second) != m_output_fragments.end());
	}
}

std::ostream& operator<<(std::ostream& s, const dependency_ordering_cxx_target::emit_kind& k)
{
	switch (k)
	{
		case dependency_ordering_cxx_target::EMIT_DECL: s << "decl"; break;
		case dependency_ordering_cxx_target::EMIT_DEF:  s << "def"; break;
		default: abort();
	}
	return s;
}
void dependency_ordering_cxx_target::write_ordered_output()
{
	std::ostream& out = std::cout; /* FIXME: take as arg */
	set <pair<emit_kind, iterator_base>, compare_with_type_equality > emitted;

	multimap< pair<emit_kind, iterator_base>, pair<emit_kind, iterator_base>, compare_with_type_equality >
	 order_constraints_reversed;
	for (auto p : m_order_constraints) order_constraints_reversed.insert(make_pair(p.second, p.first));

	/* Remember the definition and summary code of all type definitions we
	 * emit. This is because compare_by_type_equality still makes a lot
	 * of fine distinctions between types, e.g. it will distinguish
	 * typedef->typedef->base_type from typedef->base_type even though
	 * they are type-compatible in C or C++ terms. We suppress repeated
	 * definitions if they are compatible; for now just use the summary
	 * code as the test of compatibility. If they are not compatible then
	 * it's an error because we can't emit both in the same file (unless
	 * we change the name of one of them... we could do this rewriting
	 * here, but not clear it would benefit any application). */
	map<string, pair<uint32_t, string> > type_definitions_generated_by_name;

	/* We output what we can, until there's nothing left. */
	while (!m_output_fragments.empty())
	{
		unsigned i = 0;
		bool removed_one = false;
		auto i_pair = m_output_fragments.begin();
		while (i_pair != m_output_fragments.end())
		{
			auto& el = i_pair->first;
			auto&d = el.second;
			auto& frag = i_pair->second;
			/* Can we emit this? */
			auto deps_seq = m_order_constraints.equal_range(el);
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
						<< "; continuing (total " << m_output_fragments.size()
						<< " fragments remaining of which we are number " << i << ")" << endl;
					assert(!all_sat);
					break;
				}
			}
			if (all_sat)
			{
				// emit this fragment. if we should... check for type compatibility
				map<string, pair<uint32_t, string> >::iterator found = 
					type_definitions_generated_by_name.end();
				bool is_a_named_type_die = d.is_a<type_die>() && d.name_here();
				bool already_emitted_compatible_type =
					is_a_named_type_die &&
					(found = type_definitions_generated_by_name.find(*d.name_here()))
					!= type_definitions_generated_by_name.end();
				if (!already_emitted_compatible_type)
				{
					out << frag;
					if (is_a_named_type_die)
					{
						opt<uint32_t> maybe_our_summary_code = d.as_a<type_die>()->summary_code();
						// incompletes can be ignored -- duplicate declarations are OK
						if (maybe_our_summary_code)
						{
							type_definitions_generated_by_name[*d.name_here()] = make_pair(
								*maybe_our_summary_code, frag);
						}
					}
				}
				else if (is_a_named_type_die)
				{
					/* Check the thing we found is compatible with us. */
					assert(found != type_definitions_generated_by_name.end());
					auto found_summary_code = found->second.first;
					opt<uint32_t> maybe_our_summary_code = d.as_a<type_die>()->summary_code();
					assert(maybe_our_summary_code);
					if (found->second.first != *maybe_our_summary_code)
					{
						debug(0) << "Trying to emit incompatible type definition "
							<< "for a name already used: "
							<< *d.name_here()
							<< ", " << d.summary()
							<< ", emitted frag was " << found->second.second << endl;
					}
				}
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
					auto pos = m_order_constraints.lower_bound(depending);
					while (pos->first == depending)
					{
						if (pos->second == depended_on) pos = m_order_constraints.erase(pos);
						else ++pos;
					}
				}
				// now remove from output fragments
				i_pair = m_output_fragments.erase(i_pair);
				out << "\n// fragments remaining: " << m_output_fragments.size() << endl;
				removed_one = true;
			}
			else /* continue to next */ { ++i_pair; ++i; }
		}
		if (!removed_one)
		{
			debug(0) << "digraph stuck_with_order_constraints {" << endl;
			for (auto pair : m_order_constraints)
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
