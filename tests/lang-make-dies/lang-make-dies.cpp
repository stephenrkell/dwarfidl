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
	core::root_die r(fileno(in));
	
	/* Also open a dwarfidl file, read some DIE definitions from it. */
	auto str = antlr3FileStreamNew(
		reinterpret_cast<uint8_t*>(const_cast<char*>("dies.dwarfidl")), 
		ANTLR3_ENC_UTF8
	);
	assert(str);
	auto lexer = dwarfidlSimpleCLexerNew(str);
	auto tokenStream = antlr3CommonTokenStreamSourceNew(ANTLR3_SIZE_HINT, TOKENSOURCE(lexer));
	auto parser = dwarfidlSimpleCParserNew(tokenStream);
	dwarfidlSimpleCParser_toplevel_return ret = parser->toplevel(parser);
	antlr::tree::Tree *tree = ret.tree;
	
	auto created_cu = r.make_new(r.begin(), DW_TAG_compile_unit);
	assert(created_cu);
	dwarfidl::create_dies(r.find(created_cu->get_offset()), tree);
	
	cout << r;
	
	return 0;
}

