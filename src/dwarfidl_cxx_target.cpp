#include <cstdio>
#include <cassert>
#include <string>
#include <vector>
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
using std::set;
using std::map;
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

// this is the base case
void dwarfidl_cxx_target::emit_all_decls(root_die& r)
{
	/* What does "emit all decls" mean? 
	 * In general we want to recursively walk the DIEs
	 * and emit anything which we can emit
	 * and which we haven't already emitted.
	 * For each DIE that is something we can put into a header,
	 * we emit a declaration.
	 * 
	 * For types, we emit them if we haven't already emitted them.
	 * 
	 * For objects (functions, variables), we emit them if they are
	 * non-declarations.
	 * 
	 * For DIEs that contain other DIEs, we emit the parent as
	 * an enclosing block, and recursively emit the children.
	 * This holds for namespaces, struct/class/union types,
	 * functions (where the "enclosed block" is the bracketed
	 * part of the signature) and probably more.
	 *
	 * For each declaration we emit, we should check that the
	 * ABI a C++ compiler will generate from our declaration is
	 * compatible with the ABI that our debugging info encodes.
	 * This means that for functions, we check the symbol name.
	 * We should also check base types (encoding) and structured
	 * types (layout). For now we just use alignment annotations.
	 *
	 * Complication! For dependency reasons, we forward-declare
	 * everything we reasonably can first.
	 */
	
	map< vector<string>, iterator_base > toplevel_decls_emitted;
	
	set<iterator_base> dies;
	dwarf::core::type_set types;
	gather_interface_dies(r, dies, types, [/*subprogram_names*/](const iterator_base& i){
		/* This is the basic test for whether an interface element is of 
		 * interest. For us, it's just whether it's a visible subprogram in our list. */
		return true; //i_subp && i_subp.name_here() 
			//&& (!i_subp->get_visibility() || *i_subp->get_visibility() == DW_VIS_exported)
			//&& (subprogram_names.find(*i_subp.name_here()) != subprogram_names.end());
	});
	
	emit_decls(dies);
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
	
	// FIXMEs:
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
	
	set<iterator_base> emitted;

	/* I started writing a bunch of things like this... */
#if 0
	enum anonymous_emit_method {
		EMIT_INLINE,
		EMIT_WITH_GENERATED_NAME
	};
	enum base_type_reference_method {
		EMIT_USING_COMPILER_INFO,
		EMIT_AS_IF_TYPEDEF
	};
	enum typedef_emit_method {
		NO_EMIT_TYPEDEF,
		EMIT_TYPEDEF
	};
#endif
	/* ... but these seem more like roughly per-method overrides.
	 * So instead let's use "static polymorphism" here. */
	enum dep_kind {
		DEP_DECL,
		DEP_DEF
	};
	multimap< iterator_base, pair<dep_kind, iterator_base> > dependencies;

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
#if 0
	struct name_emitter
	{

		opt<string> name_referring_to_anonymous(iterator_df<basic_die> i)
		{ return opt<string>(); /* forces inline treatment of anonymouses */ }
		opt<string> name_referring_to(iterator_df<basic_die> i)
		{ return i.name_here() ?: name_referring_to_anonymous(i); }
		/* Now if I supply overrides like... */
		opt<string> name_referring_to(iterator_df<base_type_die> i)
		{  }
		/* ... how do I ensure they get dispatched to? Or even
		 * using template <Dwarf_Half tag> ... ? */

		set<pair<dep_kind, iterator_base> > get_dependencies(iterator_base i)
		{
			/* Enumerate what any given DIE depends on. Whenever we get
			 * a DIE that we depend on, use name_referring_to and see what
			 * it says. If it comes back with a name, we depend on that
			 * named thing. If it comes back with no name, we don't depend
			 * on that named thing but we will have to inline that definition
			 * e.g. of an anonymous struct.
			 *
			 * So really we are doing the emission, just in a no-op mode
			 * where we don't actually emit anything, just record dependencies.
			 * So this should really be implemented as a parameterisation of
			 * our .
			 *
			 * "Can we refer to it by name [in this context]?"
			 * If we can, get the name. Emit: use it.
			 *                          DepRecord: depend on a decl/def of it.
			 * If we can't, decompose.  Emit: use the parts to accumulate the name.
			 *                          DepRecord: depend on a decl/def of each part.
			 *
			 * /*namer << t << "name" ;*/
			 * What about the very top level?
			 */
		
		}
	};

	std::function<void(iterator_base)> collect_named_dependencies
	 = [&dependencies, collect_named_dependencies](iterator_base i) {
		auto add_dependency = [&dependencies, &i](iterator_base j) {
			dependencies.insert(make_pair(i, j));
		};
		// if it's forward-declared, we don't bother counting it as a dependency
		if (i.is_a<with_data_members_die>() && i.name_here()) return;
		if (i.is_a<base_type_die>()) return; // base types are always available
		if (i.is_a<type_chain_die>())
		{ collect_named_dependencies(i.as_a<type_chain_die>()->find_type()); }
		else if (i.is_a<variable_die>())
		{ collect_named_dependencies(i.as_a<variable_die>()->find_type()); }
		else if (i.is_a<with_data_members_die>()) // anonymous case
		{
			// what about the with-data-members itself? no, it's not named
			// (In practice, we may give it a name. And if we do, anything
			// depending on it 
			auto children = i.children().subseq_of<member_die>();
			for (auto i_child = children.first; i_child != children.second; ++i_child)
			{
				add_dependency(i_child->find_type());
			}
		}
		else if (i.is_a<type_describing_subprogram_die>())
		{
			if (i.as_a<type_describing_subprogram_die>()->get_type())
			{
				collect_named_dependencies(i.as_a<type_describing_subprogram_die>()->find_type());
			}
			auto children = i.children().subseq_of<member_die>()
			for (auto i_child = children.first; i_child != children.second; ++i_child)
			{
				collect_named_dependencies(i_child->find_type());
			}
		}
		else { /* what goes here? */ }
	};
#endif
	auto can_emit_now = [dependencies](iterator_base i) -> bool {
		// gather dependencies
		/* Now test whether all dependencies are emitted. */
#if 0
		for (auto i_dep = dependencies.begin(); i_dep != dependencies.end(); ++i_dep)
		{
			if (emitted.find(i_dep) == emitted.end()) return false;
		}
#endif
		return true;
	};
	multimap< vector<opt<string > >, iterator_base > emitted_by_path;
	auto pred = [&emitted_by_path, &emitted, this](iterator_base i) -> bool {
		/* We check whether exactly we have been declared already */
		if (emitted.find(i) != emitted.end()) return false;
		/* Also check whether something of this name has been declared already. */
		vector< opt<string> > ident_path;
		bool path_is_complete = true;
		for (iterator_df<> p = i; p.is_a<compile_unit_die>(); p = p.parent())
		{
			auto maybe_name = i.name_here();
			path_is_complete &= !!maybe_name;
			ident_path.push_back(maybe_name);
		}
		if (path_is_complete && ident_path.size() == 1)
		{
			auto found = emitted_by_path.equal_range(ident_path);
			if (found.first != found.second)
			{
				/* This means we would be redecling if we emitted here. */
				auto &previous = found.first->second;
				auto &current = i;
				auto print_name_parts = [](const vector<opt<string> >& ident_path)
				{
					for (auto i_name_part = ident_path.begin();
						i_name_part != ident_path.end(); ++i_name_part)
					{
						if (i_name_part != ident_path.begin()) cerr << " :: ";
						cerr << (*i_name_part ? **i_name_part : "(anonymous)");
					}
				};
				/* In the case of types, we output a warning. */
				if (current.is_a<type_die>() && previous.is_a<type_die>())
				{
					if (current.as_a<type_die>()->summary_code() != 
					previous.as_a<type_die>()->summary_code())
					{
						cerr << "Warning: saw rep-incompatible types with "
								"identical toplevel names: ";
						print_name_parts(ident_path);
						cerr << endl;
					}
				}
				// we should skip this
				cerr << "Skipping redeclaration of DIE named ";
				print_name_parts(ident_path);
				cerr << i.summary();
				cerr << " already emitted as "
					<< previous.summary()
					<< endl;
				return false;
			}

			/* At this point, we are going to give the all clear to emit.
			 * But we want to remember this, so we can skip future redeclarations
			 * that might conflict. */

			/* Some declarations are harmless to emit, because they never 
			 * conflict (forward decls). We won't bother remembering these. */
			auto as_program_element
			 = i.as_a<program_element_die>();
			bool is_harmless_fwddecl = as_program_element
				&& as_program_element->get_declaration()
				&& *as_program_element->get_declaration();

			// if we got here, we will go ahead with emitting;
			// if it generates a name, remember this!
			// NOTE that dwarf info has been observed to contain things like
			// DW_TAG_const_type, type structure (see evcnt in librump.o)
			// where the const type and the structure have the same name.
			// We won't use the name on the const type, so we use the
			// cxx_type_can_have_name helper to rule those cases out.
			auto is_type = i.as_a<type_die>();
			if (
				(!is_type || (is_type && this->cxx_type_can_have_name(is_type)))
			&&  !is_harmless_fwddecl
			)
			{
				// toplevel_decls_emitted.insert(make_pair(*opt_ident_path, i));
			}
		} // end if path is complete and size 1
		// we remember what we've okayed and never okay the same thing twice
		emitted.insert(i);
		emitted_by_path.insert(make_pair(ident_path, i));
		return true;
	};
	auto i_i_d = dies.begin();
	while (!dies.empty())
	{
		auto i_i_d = dies.begin();
		bool removed_one = false;
		while (i_i_d != dies.end())
		{
			auto i_d = *i_i_d;
			// only emit if dependencies allow
			if (can_emit_now(i_d))
			{
				cerr << "Now dispatching " << i_d.summary() << endl;
				// o = (*i)->get_offset();
				//if (!spec::file_toplevel_die::is_visible()(p_d)) continue; 
				dispatch_to_model_emitter(
					out,
					i_d,
					// this is our predicate
					pred
					);
				// remove what we emit
				i_i_d = dies.erase(i_i_d);
				removed_one = true;
			}
			else ++i_i_d;
		}
		assert(removed_one);
		// we go around again
	}
}

} } // end namespace dwarf::tool
