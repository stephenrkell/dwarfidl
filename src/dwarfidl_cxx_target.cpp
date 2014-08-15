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
using dwarf::core::program_element_die;

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

	/* We now don't bother doing the topological sort, so we just
	 * forward-declare everything that we can. */
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
		
	for (auto i_i_d = dies.begin(); i_i_d != dies.end(); ++i_i_d)
	{
		auto i_d = *i_i_d;
		// o = (*i)->get_offset();
		//if (!spec::file_toplevel_die::is_visible()(p_d)) continue; 
		dispatch_to_model_emitter( 
			out,
			i_d,
			// this is our predicate
			[&toplevel_decls_emitted, this](iterator_base i)
			{
// 					/* We check whether we've been declared already */
// 					auto opt_ident_path = p_d->ident_path_from_cu();
// 					if (opt_ident_path && opt_ident_path->size() == 1)
// 					{
// 						auto found = toplevel_decls_emitted.find(*opt_ident_path);
// 						if (found != toplevel_decls_emitted.end())
// 						{
// 							/* This means we would be redecling if we emitted here. */
// 							auto current_is_type
// 							 = dynamic_pointer_cast<spec::type_die>(p_d);
// 							auto previous_is_type
// 							 = dynamic_pointer_cast<spec::type_die>(found->second);
// 							auto print_name_parts = [](const vector<string>& ident_path)
// 							{
// 								for (auto i_name_part = ident_path.begin();
// 									i_name_part != ident_path.end(); ++i_name_part)
// 								{
// 									if (i_name_part != ident_path.begin()) cerr << " :: ";
// 									cerr << *i_name_part;
// 								}
// 							};
// 							
// 							/* In the case of types, we output a warning. */
// 							if (current_is_type && previous_is_type)
// 							{
// 								if (!current_is_type->is_rep_compatible(previous_is_type)
// 								||  !previous_is_type->is_rep_compatible(current_is_type))
// 								{
// 									cerr << "Warning: saw rep-incompatible types with "
// 											"identical toplevel names: ";
// 									print_name_parts(*opt_ident_path);
// 									cerr << endl;
// 								}
// 							}
// 							// we should skip this
// 							cerr << "Skipping redeclaration of DIE ";
// 							//print_name_parts(*opt_ident_path);
// 							cerr << p_d->summary();
// 							cerr << " already emitted as " 
// 								<< *toplevel_decls_emitted[*opt_ident_path]
// 								<< endl;
// 							return false;
// 						}
// 						
// 						/* At this point, we are going to give the all clear to emit.
// 						 * But we want to remember this, so we can skip future redeclarations
// 						 * that might conflict. */
// 						
// 						/* Some declarations are harmless to emit, because they never 
// 						 * conflict (forward decls). We won't bother remembering these. */
// 						auto as_program_element
// 						 = i.as_a<program_element_die>();
// 						bool is_harmless_fwddecl = as_program_element
// 							&& as_program_element->get_declaration()
// 							&& *as_program_element->get_declaration();
// 						
// 						// if we got here, we will go ahead with emitting; 
// 						// if it generates a name, remember this!
// 						// NOTE that dwarf info has been observed to contain things like
// 						// DW_TAG_const_type, type structure (see evcnt in librump.o)
// 						// where the const type and the structure have the same name.
// 						// We won't use the name on the const type, so we use the
// 						// cxx_type_can_have_name helper to rule those cases out.
// 						auto is_type = i.as_a<type_die>();
// 						if (
// 							(!is_type || (is_type && this->cxx_type_can_have_name(is_type)))
// 						&&  !is_harmless_fwddecl
// 						)
// 						{
// 							// toplevel_decls_emitted.insert(make_pair(*opt_ident_path, i));
// 						}
// 					} // end if already declared with this 

				// not a conflict-creating redeclaration, so go ahead
				return true;
			}
		); 
	} 
}

} } // end namespace dwarf::tool
