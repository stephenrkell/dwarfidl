#include <cstdio>
#include <cassert>
#include <string>
#include <vector>
#include <set>
#include <sstream>
#include <cmath>

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
using std::cout;
using std::cerr;
using std::endl;
using std::hex;
using std::dec;
using std::ostringstream;
using std::istringstream;
using std::stack;
using std::deque;
using boost::optional;
using namespace dwarf::lib;
using dwarf::core::iterator_base;
using dwarf::core::root_die;
using dwarf::core::abstract_die;
using dwarf::core::type_set;
using dwarf::core::subprogram_die;
using dwarf::core::with_data_members_die;
using dwarf::tool::gather_interface_dies;

int main(int argc, char **argv)
{
	using dwarf::tool::dwarfidl_cxx_target;

	// open the file passed in on the command-line
	assert(argc > 1);
	FILE* f = fopen(argv[1], "r");
	
	// construct a dwarf::file
	struct root_die_with_sticky_types : public root_die
	{
		using root_die::root_die;
		
		bool is_sticky(const abstract_die& d)
		{
			return /* d.get_spec(*this).tag_is_type(d.get_tag())
				|| */this->root_die::is_sticky(d);
		}
	} r(fileno(f));
	
	indenting_ostream& s = srk31::indenting_cout;
	
	vector<string> compiler_argv = dwarf::tool::cxx_compiler::default_compiler_argv(true);
	compiler_argv.push_back("-fno-eliminate-unused-debug-types");
	compiler_argv.push_back("-fno-eliminate-unused-debug-symbols");
	
	dwarfidl_cxx_target target(" ::cake::unspecified_wordsize_type", // HACK: Cake-specific
		s, compiler_argv);
	
	/* If the user gives us a list of function names on stdin, we use that. */
	using std::cin;
	std::istream& in = /* p_in ? *p_in :*/ cin;
	/* populate the subprogram and types lists. */
	char buf[4096];
	set<string> subprogram_names;
	while (in.getline(buf, sizeof buf - 1))
	{
		/* Find a toplevel grandchild that is a subprogram of this name. */
		subprogram_names.insert(buf);
	}

	set<iterator_base> dies;
	type_set types;
	gather_interface_dies(r, dies, types, [subprogram_names](const iterator_base& i){
		/* This is the basic test for whether an interface element is of 
		 * interest. For us, it's just whether it's a visible subprogram or variable
		 * in our list. Or, if our list is empty, it's any subprogram. 
		 * FIXME: variables too!*/
		auto i_subp = i.as_a<subprogram_die>();
		return i_subp && i_subp.name_here() 
			&& (!i_subp->get_visibility() || *i_subp->get_visibility() == DW_VIS_exported)
			&& (
				subprogram_names.size() > 0 ? 
					subprogram_names.find(*i_subp.name_here()) != subprogram_names.end()
				:   true
				);
	});

	//target.emit_decls(dies);
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
	if (to_fd.size() > 0)
	{
		s << "// begin a group of forward decls" << endl;
		for (auto i = to_fd.begin(); i != to_fd.end(); i++)
		{
			bool is_struct = (*i).tag_here() == DW_TAG_structure_type;
			bool is_union = (*i).tag_here() == DW_TAG_union_type;

			assert((is_struct || is_union) && (*i).name_here());

			s << (is_struct ? "struct " : "union ") << target.protect_ident(*(*i).name_here()) 
				<< "; // forward decl" << std::endl;
		}
		s << "// end a group of forward decls" << std::endl;
	}
	set<iterator_base> emitted;
	auto i_i_d = dies.begin();
	while (i_i_d != dies.end())
	{
		if (emitted.find(*i_i_d) != emitted.end()) goto next;
		/* Everything in the slice is either an object (incl. function) or a type. */
		// FIXME: call the target to emit a decl...
		// ... the only exception being structs which need a def
	next:
		i_i_d = dies.erase(i_i_d);
	}


	return 0;
}
