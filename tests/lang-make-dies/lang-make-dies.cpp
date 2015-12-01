#include <fstream>
#include <fileno.hpp>
#include <dwarfpp/lib.hpp>
#include <dwarfpp/attr.hpp>
#include <dwarfidl/create.hpp>

using std::cout; 
using std::endl;
using namespace dwarf;

int main(int argc, char **argv)
{
	using namespace dwarf::core; 
	
	cout << "Opening " << argv[0] << "..." << endl;
	std::ifstream in(argv[0]);
	core::in_memory_root_die r(fileno(in));
	
	/* Also open a dwarfidl file, read some DIE definitions from it. */
	auto str = antlr3FileStreamNew(
		reinterpret_cast<uint8_t*>(const_cast<char*>("dies.dwarfidl")), 
		ANTLR3_ENC_UTF8
	);
	assert(str);
	auto lexer = dwarfidlNewCLexerNew(str);
	auto tokenStream = antlr3CommonTokenStreamSourceNew(ANTLR3_SIZE_HINT, TOKENSOURCE(lexer));
	auto parser = dwarfidlNewCParserNew(tokenStream);
	dwarfidlNewCParser_toplevel_return ret = parser->toplevel(parser);
	antlr::tree::Tree *tree = ret.tree;
	
	auto created_cu = r.make_new(r.begin(), DW_TAG_compile_unit);
	assert(created_cu);
	cout << "Created CU: " << created_cu << endl;
	
	dwarfidl::create_dies(created_cu, tree);
	
	cout << "Created some more stuff; whole tree is now: " << endl << r;
	
	return 0;
}

