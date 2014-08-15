#ifndef DWARFIDL_DWARF_INTERFACE_WALK_HPP_
#define DWARFIDL_DWARF_INTERFACE_WALK_HPP_

#include <set>
#include <functional>
#include <dwarfpp/lib.hpp>

namespace dwarf { namespace tool {

using std::set;
using dwarf::core::root_die;
using dwarf::core::iterator_base;
using dwarf::core::type_set;

void 
gather_interface_dies(root_die& root, 
	set<iterator_base>& out, type_set& dedup_types_out, 
	std::function<bool(const iterator_base&)> pred);

} }

#endif
