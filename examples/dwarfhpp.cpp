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

// 	if (subprogram_names.size() > 0)
// 	{
// 	
// 	}
// 	else
// 	{
// 		target.emit_all_decls(r);
// 	}
	target.emit_decls(dies);

	return 0;
}
