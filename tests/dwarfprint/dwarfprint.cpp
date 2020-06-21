#include <fstream>
#include <string>

#include "dwarfidl/lang.hpp"
#include "dwarfidl/dwarfprint.hpp"

using namespace std;
using namespace dwarf;
using namespace dwarf::core;

typedef ANTLR3_TOKEN_SOURCE TokenSource;
typedef ANTLR3_COMMON_TOKEN CommonToken;
typedef ANTLR3_INPUT_STREAM ANTLRInputStream;
typedef ANTLR3_COMMON_TOKEN_STREAM CommonTokenStream;
typedef ANTLR3_BASE_TREE Tree; 
typedef ANTLR3_COMMON_TREE CommonTree;

using dwarf::tool::gather_interface_dies;

int main(int argc, char **argv)
{
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
	
	/* If the user gives us a list of function names on stdin, we use that. */
//	using std::cin;
//	std::istream& in = /* p_in ? *p_in :*/ cin;
	/* populate the subprogram and types lists. */
	char buf[4096];
	set<string> element_names = { "main", "fopen" };
	map<string, iterator_base> named_element_dies;
// 	while (in.getline(buf, sizeof buf - 1))
// 	{
// 		/* Find a toplevel grandchild that is a subprogram of this name. */
// 		element_names.insert(buf);
// 	}

	set<iterator_base> dies;
	type_set types;
	gather_interface_dies(r, dies, types,
		[element_names, &named_element_dies](const iterator_base& i) {
		/* This is the basic test for whether an interface element is of 
		 * interest. For us, it's just whether it's a visible subprogram or variable
		 * in our list. Or, if our list is empty, it's any subprogram. Variables too!*/

		bool result;
		auto i_pe = i.as_a<program_element_die>();
		if (i_pe && i_pe.name_here()
			&& ((i_pe.is_a<variable_die>() && i_pe.as_a<variable_die>()->has_static_storage())
				|| i_pe.is_a<subprogram_die>())
			&& (!i_pe->get_visibility() || *i_pe->get_visibility() == DW_VIS_exported)
			&& (
				element_names.size() == 0 
				|| element_names.find(*i_pe.name_here()) != element_names.end()))
		{
			if (element_names.size() == 0)
			{
				result = true;
			}
			else if (element_names.find(*i_pe.name_here()) != element_names.end())
			{
				named_element_dies[*i_pe.name_here()] = i;
				result = true;
			} else result = false;
		} else result = false;;

		// if (i_pe && i_pe.name_here()) {
		// 	cerr << "Considering " << *(i_pe.name_here()) << " => " << result << endl;
		// }
		return result;
	});
	char tmpname[] = "/tmp/tmp.XXXXXX";
	int fd = mkstemp(tmpname);
	if (fd == -1) exit(42);
	std::ofstream outf(tmpname);
	assert(outf);
	close(fd);
	print_dies(outf, dies, types);

	/* also test that we can parse this output, using the dwarfidlNewC parser. */
	pANTLR3_INPUT_STREAM in_fileobj = antlr3FileStreamNew((uint8_t *) tmpname,
		ANTLR3_ENC_UTF8);
	dwarfidlNewCLexer *lexer = dwarfidlNewCLexerNew(in_fileobj);
	CommonTokenStream *tokenStream = antlr3CommonTokenStreamSourceNew(
		ANTLR3_SIZE_HINT, TOKENSOURCE(lexer));
	dwarfidlNewCParser *parser = dwarfidlNewCParserNew(tokenStream);
	dwarfidlNewCParser_toplevel_return ret = parser->toplevel(parser);
	Tree *tree = ret.tree;
	assert(tree);
	assert(ret.start != ret.stop);
	//unlink(tmpname);
	std::cerr << "DEBUG: temporary output file is at " << tmpname << std::endl;

	return 0;
}
