#include <cstdio>
#include <cassert>
#include <string>
#include <vector>
#include <set>
#include <sstream>
#include <cmath>
#include <fstream>
#include <map>
#include <cctype>
#include <cstdlib>
#include <cstddef>
#include <memory>

#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/icl/interval_map.hpp>
#include <srk31/algorithm.hpp>
#include <srk31/ordinal.hpp>
#include <cxxgen/tokens.hpp>
#include <cxxgen/cxx_compiler.hpp>
#include <dwarfpp/attr.hpp>
#include <dwarfpp/encap.hpp>
#include <dwarfpp/lib.hpp>
#include <fileno.hpp>

#include <srk31/concatenating_iterator.hpp>
#include <srk31/indenting_ostream.hpp>
#include <dwarfidl/dwarf_interface_walk.hpp>

std::string dies_to_idl(std::set<dwarf::core::iterator_base> dies);
void print_type_die(std::ostream &_s, dwarf::core::iterator_df<dwarf::core::basic_die> die_iter, boost::optional<dwarf::core::type_set&> types = boost::optional<dwarf::core::type_set&>());
void print_dies(std::ostream &s, std::set<dwarf::core::iterator_base> dies, boost::optional<dwarf::core::type_set&> types = boost::optional<dwarf::core::type_set&>());

