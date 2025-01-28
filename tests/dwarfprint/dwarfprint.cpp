#include <fstream>
#include <string>
#include <boost/iostreams/filtering_streambuf.hpp>

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

/* This struct will get copied, so should not keep state itself. */
struct counting_filter : boost::iostreams::output_filter
{
	unsigned *chars;
	unsigned *newlines;
	counting_filter(unsigned *chars, unsigned *newlines) : chars(chars), newlines(newlines) {}
	template<typename Sink>
	bool put_char(Sink& dest, int c)
	{
		if (c == '\n') ++(*newlines);
		++(*chars);
		return boost::iostreams::put(dest, c);
	}
	template<typename Sink>
	bool put(Sink& dest, int c) 
	{
		return put_char(dest, c);
	}
};
class counting_ostream
: private boost::iostreams::filtering_ostreambuf,
  private counting_filter,
  public std::ostream
{
	unsigned chars;
	unsigned newlines;
public:
	counting_ostream(std::ostream& s = std::cout)
	: boost::iostreams::filtering_ostreambuf(),
	  counting_filter(&chars, &newlines),
	  std::ostream(static_cast<boost::iostreams::filtering_ostreambuf *>(this)),
	  chars(0), newlines(0)
	{
		push(static_cast<counting_filter&>(*this));
		push(s);
	}
	unsigned chars_written() const { return chars; }
	unsigned newlines_written() const { return newlines; }
};

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
	// HACK to ensure that bad parse really does fail the test
	//FILE *the_f = fopen(tmpname, "r");
	//assert(the_f);
	//int fd = fileno(the_f);
	if (fd == -1) exit(42);
	std::ofstream outf(tmpname);
	assert(outf);
	close(fd);
	// print to a special stream that counts lines and characters.
	counting_ostream counting_outf(outf);
	print_dies(counting_outf, dies, types);

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
	// how many lines did we parse? it should match the number output
	unsigned nlines = ret.stop->getLine(ret.stop);
	std::clog << "DEBUG: parsed " << nlines << " versus written: "
		<< (counting_outf.newlines_written() - 1) << std::endl;
	std::clog << "DEBUG: temporary output file is at " << tmpname << std::endl;
	assert(nlines == counting_outf.newlines_written() - 1);
	unlink(tmpname);

	return 0;
}
