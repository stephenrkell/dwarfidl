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

namespace dwarf { namespace tool {

void dwarfidl_cxx_target::transitively_close(
	set<std::pair<dwarfidl_cxx_target::emit_kind, iterator_base> > const& to_output,
	cxx_generator_from_dwarf::referencer_fn_t maybe_get_name,
	/* TODO: accept 'definer', e.g. for noopgen? */
	map<std::pair<dwarfidl_cxx_target::emit_kind, iterator_base>, string >& output_fragments,
	multimap< std::pair<dwarfidl_cxx_target::emit_kind, iterator_base>, std::pair<dwarfidl_cxx_target::emit_kind, iterator_base> >& order_constraints
)
{
	// 1. pre-populate -- this is something the client tool does
	// by giving us a set<pair<emit_kind, iterator_base> >,
	// but we turn it into a vector so that we can use it as a workload (add to the back)
	// ... and we also keep a set representation so we can test for already-presentness
	set<std::pair<emit_kind, iterator_base> > output_expanded;
	vector <std::pair<emit_kind, iterator_base> > worklist;
	auto maybe_add_to_worklist = [&]( std::pair<emit_kind, iterator_base> const& el) {
		// only add something if it's new
		auto inserted = output_expanded.insert(el);
		if (!inserted.second) return; // nothing was inserted, i.e. it was already present
		worklist.push_back(el);
	};
	for (auto el : to_output) maybe_add_to_worklist(el);

	// 2. transitively close -- this is something we do. How? By calling
	// the printer and adding to to_emit the named dependencies it tells us about.
	// Once we've printed everything in to_emit, we have full ordering info.
	bool changed = true;
	while (!worklist.empty())
	{
		changed = false; // we haven't yet changed anything this cycle
		bool saw_missing_fragment = false;
		/* Traverse the whole worklist... we may append to the list during the loop. */
		auto i_pair = worklist.begin();
		while (i_pair != worklist.end())
		{
			auto& the_pair = *i_pair;
			// FIXME: if 'referencer' was just a function, much boilerplate would go...
			// this is basically a lambda capturing locals.  Is a refactoring feasible?
			auto maybe_get_name_tracking_deps
			= [&](iterator_base i, enum ref_kind k) -> opt<string> {
				opt<string> ret = maybe_get_name(i, k);
				if (!ret || k == cxx_generator::DEFINING) return ret;
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
						order_constraints.insert(make_pair(the_pair, new_pair));
						maybe_add_to_worklist(new_pair);
					}
					else if (i && i.is_a<type_die>())
					{
						// the typedef must come first, but also...
						// FIXME: should this add everything on the chain, not just
						// the concrete?
						auto immediate_pair = make_pair(EMIT_DECL, i);
						order_constraints.insert(make_pair(the_pair, immediate_pair));
						maybe_add_to_worklist(immediate_pair);
						auto concrete_pair = make_pair(EMIT_DEF, 
							i.as_a<type_die>()->get_concrete_type());
						order_constraints.insert(make_pair(the_pair, concrete_pair));
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
					order_constraints.insert(make_pair(the_pair, default_pair));
					maybe_add_to_worklist(default_pair);
				}
				return ret;

			};
#if 0
			struct dep_tracking_referencer : referencer
			{
				multimap< std::pair<emit_kind, iterator_base>, std::pair<emit_kind, iterator_base> >& order_constraints;
				std::pair<emit_kind, iterator_base> referring;
				set<std::pair<emit_kind, iterator_base> >& output_expanded;
				vector <std::pair<emit_kind, iterator_base> >& worklist;
				bool& changed;
				std::function<void(std::pair<emit_kind, iterator_base> const&)> maybe_add_to_worklist;
				/* In the process of generating the code fragment for
				 * the_pair, anything that is emitted as a name
				 * will be recorded as a dependency. */
				dep_tracking_referencer(
					referencer& wrapped,
					multimap< std::pair<emit_kind, iterator_base>, std::pair<emit_kind, iterator_base> >& order_constraints,
					std::pair<emit_kind, iterator_base> referring,
					set<std::pair<emit_kind, iterator_base> >& output_expanded,
					vector <std::pair<emit_kind, iterator_base> >& worklist,
					bool& changed,
					std::function<void(std::pair<emit_kind, iterator_base> const&)> maybe_add_to_worklist
				) : referencer(wrapped), order_constraints(order_constraints),
				referring(referring), output_expanded(output_expanded), worklist(worklist),
				changed(changed), maybe_add_to_worklist(maybe_add_to_worklist) {}
				opt<string> can_name(iterator_base i, bool type_must_be_complete) const
				{
				}
			} our_r(r, order_constraints, the_pair, output_expanded, worklist, changed, maybe_add_to_worklist);
			// try to generate a string.
			// Some DIEs will just emit nothing because e.g. for "const int",
			// no definition is needed, only a dependency in "int" (which itself
			// will emit nothing when 'declared' or 'defined').
#endif
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
					output_fragments[the_pair] = s.str(); changed = true;
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
					output_fragments[the_pair] = s.str(); changed = true;
				} break;
				default:
					assert(false); abort();
			}
			// we've just processed one worklist item
			i_pair = worklist.erase(i_pair);
		} // end while worklist
		// if we're about to terminate, we should not have seen any missing fragment
	}
	
	// now the caller has the output fragments and order constraints
}

void dwarfidl_cxx_target::write_ordered_output(
		map<pair<emit_kind, iterator_base>, string > output_fragments /* by value: copy */,
		multimap< pair<emit_kind, iterator_base>, pair<emit_kind, iterator_base> >& order_constraints
	)
{
	std::ostream& out = std::cout; /* FIXME: take as arg */
	set <pair<emit_kind, iterator_base> > emitted;
	/* We output what we can, until there's nothing left. */
	while (!output_fragments.empty())
	{
		bool removed_one = false;
		auto i_pair = output_fragments.begin();
		while (i_pair != output_fragments.end())
		{
			auto& el = i_pair->first;
			auto& frag = i_pair->second;
			/* Can we emit this? */
			auto deps_seq = order_constraints.equal_range(i_pair->first);
			bool all_sat = true;
			for (auto i_dep = deps_seq.first; i_dep != deps_seq.second; ++i_dep)
			{
				auto& dep = i_dep->second;
				bool already_emitted = (emitted.find(dep) != emitted.end());
				all_sat &= already_emitted;
			}
			if (all_sat)
			{
				// emit this fragment
				out << frag;
				// add to the emitted set
				auto retpair = emitted.insert(el);
				assert(retpair.second); // really inserted
				// remove from output fragmnets
				i_pair = output_fragments.erase(i_pair);
				removed_one = true;
			}
			else /* continue to next */ ++i_pair;
		}
		if (!removed_one) { assert(false); abort(); } /* lack of progress */
	}
}

#if 0
void dwarfidl_cxx_target::emit_forward_decls(const set<iterator_base>& fds)
{
	if (fds.size() > 0)
	{
		out << "// begin a group of forward decls" << endl;
		for (auto i = fds.begin(); i != fds.end(); i++)
		{
			bool is_struct = (*i).tag_here() == DW_TAG_structure_type;
			bool is_union = (*i).tag_here() == DW_TAG_union_type;

			assert((is_struct || is_union) && (*i).name_here());

			out << (is_struct ? "struct " : "union ") << protect_ident(*(*i).name_here()) 
				<< "; // forward decl" << std::endl;
		}
		out << "// end a group of forward decls" << std::endl;
	}
}

void dwarfidl_cxx_target::emit_decls(set<iterator_base>& dies)
{
	/* We now don't bother doing the topological sort, so we just
	 * forward-declare everything that we can.
	 * PROBLEM: anything can depend on a typedef, and
	 * typedefs can depend on typedefs.
	 * Instead of an up-front topsort we can probably just do a fixed-point
	 * iteration: emit anything whose dependencies have been emitted,
	 * and keep doing that until nothing is left.
	 */
	set<iterator_base> to_fd;
	for (auto i_i_d = dies.begin(); i_i_d != dies.end(); ++i_i_d)
	{
		auto i_d = *i_i_d;
		if (i_d.is_a<with_data_members_die>() && i_d.name_here())
		{
			to_fd.insert(i_d);
		}
	}
	emit_forward_decls(to_fd);
	
	// FIXMEs for noopgen:
	// - not emitting named function typedefs (make_precise_fn_t)
	// - function arguments going astray entirely: __lookup_bigalloc_top_level, mmap, ...
	// - typedefs of arrays are coming out wrong
	// - extra parens around non-pointed functions?
	// - bitfields not coming out
	// - spurious warning message for unions
	// - enumeration coming out with leading comma (addr_discipl_t)
	// - redundant "aligned(1)" attribute for fields that are on the ABI alignment
	// - C vs C++ issues: struct tags, "_Bool", etc.
	// - nameless function typedefs after big_allocation (3805..31e6)
	



	/* Just what does "dependency" mean here?
	 * It means another thing that must be emitted before we can emit the first thing.
	 * Whether that is true depends on
	 * (1) how we are doing naming ("OK to inline anonymouses"? 
	 *        "see through typedefs"? if yes then no dependency on the typedef?
	 *        + what we have forward-decl'd <-- NO, this just affects emit algo/strategy
	 *        + should base types be explicitly named/declared as if a typedef
	 *        + should struct/union/class be tagged
	 *        
	 * (2) how we are using the thing (pointer-to-struct? forward-decl is enough;
	 *        instantiate struct? forward-decl is not enough)
	 */
	bool use_friendly_base_type_names = true;
	referencer namer(*this, use_friendly_base_type_names,
		spec::opt<string>(), /* use_struct_and_union_prefixes */ true);
}
#endif
} } // end namespace dwarf::tool
