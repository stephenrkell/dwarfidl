#include <cstdio>
#include <cassert>
#include <string>
#include <vector>
#include <set>
#include <sstream>
#include <cmath>

#include <boost/algorithm/string.hpp>

#include <srk31/indenting_ostream.hpp>
#include <dwarfpp/encap.hpp>
#include "dwarfidl/cxx_model.hpp"
#include "dwarfidl/cxx_dependency_order.hpp"
#include "print.hpp"
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
using namespace dwarf::core;

int main(int argc, char **argv)
{
	// open the file passed in on the command-line
	assert(argc > 1);
	FILE* f = fopen(argv[1], "r");
	assert(f);
	dwarf::core::root_die r(fileno(f));
	srk31::indenting_ostream out(cout);
	dwarf::tool::print(out, r);

	return 0;
}
