#ifndef DWARFIDL_CXX_DEPENDENCY_ORDER_HPP_
#define DWARFIDL_CXX_DEPENDENCY_ORDER_HPP_

#include <set>

namespace dwarf { namespace tool { 

using std::set;
using std::string;
using std::vector;

using namespace dwarf;
using namespace dwarf::lib;

/* Think of this class as the reusable base for
 * dwarfhpp, noopgen, and any similar tools.
 * But what does it do? It wraps cxx_target, supplying
 * some unimportant stuff (untyped_argument_typename)
 * and handles the topsort-alike handling of forward declarations
 * and other inter-declaration dependencies. */

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
	
	string get_untyped_argument_typename() { return m_untyped_argument_typename; }
	
// 	void 
// 	emit_typedef(
// 		shared_ptr<spec::type_die> p_d,
// 		const string& name
// 	)
// 	{ out << make_typedef(p_d, name); }

	void emit_all_decls(root_die& r);

	void emit_decls(const set<iterator_base>& dies);
	
	void emit_forward_decls(const set<iterator_base>& fds);
};

/* close namespaces */
} } 
#endif
