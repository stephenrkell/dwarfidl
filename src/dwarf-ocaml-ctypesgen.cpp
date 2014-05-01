/* This is a simple dwarfpp program which generates a C file
 * recording data on a uniqued set of data types  allocated in a given executable.
 */
 
#include <fstream>
#include <sstream>
#include <map>
#include <set>
#include <unordered_set>
#include <string>
#include <cctype>
#include <cstdlib>
#include <memory>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/icl/interval_map.hpp>
#include <srk31/algorithm.hpp>
#include <srk31/ordinal.hpp>
#include <cxxgen/tokens.hpp>
#include <dwarfpp/lib.hpp>
#include <fileno.hpp>

using std::cin;
using std::cout;
using std::cerr;
using std::map;
using std::make_shared;
using std::ios;
using std::ifstream;
using std::dynamic_pointer_cast;
using boost::optional;
using std::ostringstream;
using std::set;
using std::make_pair;
using std::unordered_set;
using namespace dwarf;
//using boost::filesystem::path;
using dwarf::core::iterator_base;
using dwarf::core::iterator_df;
using dwarf::core::iterator_sibs;
using dwarf::core::type_die;
using dwarf::core::subprogram_die;
using dwarf::core::formal_parameter_die;
using dwarf::core::compile_unit_die;
using dwarf::core::typedef_die;
using dwarf::core::member_die;
using dwarf::core::with_data_members_die;
using dwarf::core::variable_die;
using dwarf::core::program_element_die;
using dwarf::core::with_dynamic_location_die;
using dwarf::core::address_holding_type_die;
using dwarf::core::pointer_type_die;
using dwarf::core::union_type_die;
using dwarf::core::array_type_die;
using dwarf::core::type_chain_die;
using dwarf::core::base_type_die;
using dwarf::core::qualified_type_die;
using dwarf::spec::opt;

using dwarf::lib::Dwarf_Off;

// regex usings
using boost::regex;
using boost::regex_match;
using boost::smatch;
using boost::regex_constants::egrep;
using boost::match_default;
using boost::format_all;

static int debug_out = 1;

static string ocaml_type(const string& in)
{
	return boost::to_lower_copy(in);
}

static string ocaml_ident(const string& in)
{
	if (in == "string") return "_string";
	return boost::to_lower_copy(in);
}

static pair<string, string> base_type_equiv_and_reified_name(iterator_df<base_type_die> t)
{
	/* ctypes already has our names */
	auto encoding = t->get_encoding();
	switch (encoding)
	{
		case DW_ATE_boolean: // HACK: Ctypes doesn't have separate bool, but should
		case DW_ATE_signed:
			switch(*t.as_a<base_type_die>()->get_byte_size())
			{
				case 1:
					return make_pair("int", "int8_t");
				case 2:
					return make_pair("int", "int16_t");
				case 4:
					return make_pair("int", "int32_t"); // HACK: 31 bits is good enough for 32
				case 8:
					return make_pair("int64", "int64_t");
				default: assert(false);
			}
		case DW_ATE_unsigned:
			switch(*t.as_a<base_type_die>()->get_byte_size())
			{
				case 1:
					return make_pair("int", "uint8_t"); // HACK-ish
				case 2:
					return make_pair("int", "uint16_t"); // HACK-ish
				case 4:
					return make_pair("uint32", "uint32_t");
				case 8:
					return make_pair("uint64", "uint64_t");
				default: assert(false);
			}

		case DW_ATE_float:
			switch(*t.as_a<base_type_die>()->get_byte_size())
			{
				case 4:
					return make_pair("float", "float");
				case 8:
					return make_pair("float", "double");
				default: assert(false);
			}

		case DW_ATE_signed_char:
			switch(*t.as_a<base_type_die>()->get_byte_size())
			{
				case 1:
					return make_pair("char", "char");
				case 4:
					return make_pair("int32", "int32_t");
				default: assert(false);
			}

		case DW_ATE_unsigned_char:
			switch(*t.as_a<base_type_die>()->get_byte_size())
			{
				case 1:
					return make_pair("uchar", "uchar");
				case 4:
					return make_pair("uint32", "uint32_t");
				default: assert(false);
			}
		default:
			assert(false);
	}
}

int main(int argc, char **argv)
{
	/* We open the file named by argv[1] and dump its DWARF types. */ 
	
	if (argc <= 1) 
	{
		cerr << "Please name an input file." << endl;
		exit(1);
	}
	std::ifstream infstream(argv[1]);
	if (!infstream) 
	{
		cerr << "Could not open file " << argv[1] << endl;
		exit(1);
	}
	
	if (getenv("DUMPTYPES_DEBUG"))
	{
		debug_out = atoi(getenv("DUMPTYPES_DEBUG"));
	}
	
	using core::root_die;
	struct sticky_root_die : public root_die
	{
		using root_die::root_die;
		
		// virtual bool is_sticky(const core::abstract_die& d) { return true; }
		
	} root(fileno(infstream));
	assert(&root.get_frame_section());
	opt<core::root_die&> opt_r = root; // for debugging

	map<string, iterator_df<subprogram_die> > subprograms;
	
	dwarf::core::type_set types;
	
	std::istream& in = /* p_in ? *p_in :*/ cin;
	
	/* populate the subprogram and types lists. */
	char buf[4096];
	set<string> subprogram_names;
	while (in.getline(buf, sizeof buf - 1))
	{
		/* Find a toplevel grandchild that is a subprogram of this name. */
		subprogram_names.insert(buf);
	}
	auto toplevel_seq = root.grandchildren();
	for (auto i_d = std::move(toplevel_seq.first); i_d != toplevel_seq.second; ++i_d)
	{
		auto i_subp = i_d.base().base().as_a<subprogram_die>();
		if (!i_subp) continue; // HACK until we can do subseq_of on grandchildren
		if (i_subp.name_here() 
			&& (!i_subp->get_visibility() || *i_subp->get_visibility() == DW_VIS_exported)
			&& subprogram_names.find(*i_subp.name_here()) != subprogram_names.end())
		{
			// looks like a goer
			subprograms.insert(
				make_pair(
					*i_subp.name_here(), 
					i_subp
				)
			);
			
			auto add_all = [&types, &root](iterator_df<type_die> outer_t) {
				walk_type(outer_t, iterator_base::END, 
					[&types, &root](iterator_df<type_die> t, iterator_df<program_element_die> reason) -> bool {
						if (!t) return false; // void case
						auto memb = reason.as_a<member_die>();
						if (memb && memb->get_declaration() && *memb->get_declaration()
							&& memb->get_external() && *memb->get_external())
						{
							// static member vars don't get added nor recursed on
							return false;
						}
						// apart from that, insert all nonvoids....
						auto inserted = types.insert(t);
						if (!inserted.second)
						{
							// cerr << "Type was already present: " << *t 
							// 	<< " (or something equal to it: " << *inserted.first
							// 	<< ")" << endl;
							// cerr << "Attributes: " << t->copy_attrs(root) << endl;
							return false; // was already present
						}
						else
						{
							cerr << "Inserted new type: " << *t << endl;
							// cerr << "Attributes: " << t->copy_attrs(root) << endl;
							return true;
						}
					}
				);
			};
			
			// make sure we have all the relevant types in our set
			if (i_subp->find_type()) add_all(i_subp->find_type());
			auto fps = i_subp->children().subseq_of<formal_parameter_die>();
			for (auto i_fp = std::move(fps.first); i_fp != fps.second; ++i_fp)
			{
				add_all(i_fp->find_type());
			}
		}
	}

	cerr << "Found " << subprograms.size() << " subprograms." << endl;
	for (auto i_pair = subprograms.begin(); i_pair != subprograms.end(); ++i_pair)
	{
		iterator_base i_subp = i_pair->second;
		// root.print_tree(std::move(i_subp), cerr);
	}
	cerr << "Types list contains " << types.size() << " data types." << endl;
	for (auto i_i_t = types.begin(); i_i_t != types.end(); ++i_i_t)
	{
		auto t = *i_i_t;
		// root.print_tree(std::move(t), cerr);
	}
	
	std::function<string(iterator_df<type_die>)> equiv_for_type
	 = [&equiv_for_type](iterator_df<type_die> t) -> string {
		if (!t)
		{
			return "unit";
		}
		else if (t.is_a<with_data_members_die>())
		{
			// we forward-declared a type with the right name
			if (t.name_here()) return *t.name_here();
			else 
			{
				// it's an anonymous nested struct
				assert(false);
			}
		}
		else if (t.is_a<typedef_die>())
		{
			return equiv_for_type(t.as_a<typedef_die>()->find_type());
		}
		else if (t.is_a<pointer_type_die>())
		{
			auto target_t = t.as_a<pointer_type_die>()->find_type();
			auto unq_target_t = target_t ? target_t->get_unqualified_type() : target_t;
			if (unq_target_t && unq_target_t.is_a<base_type_die>()
				&& unq_target_t.as_a<base_type_die>()->get_encoding() == DW_ATE_signed_char
				&& *unq_target_t.as_a<base_type_die>()->get_byte_size() == 1)
			{
				// it's a signed char, so return string
				return "string";
			}
			else return equiv_for_type(target_t) + " ptr";
		}
		else if (t.is_a<base_type_die>())
		{
			return base_type_equiv_and_reified_name(t.as_a<base_type_die>()).first;
		}
		else if (t.is_a<qualified_type_die>())
		{
			return equiv_for_type(t->get_unqualified_type());
		}
		assert(false);
	};

	std::function<string(iterator_df<type_die>)> reified_type_typ
	 = [&reified_type_typ](iterator_df<type_die> t) -> string {
		if (!t)
		{
			return "void";
		}
		else if (t.is_a<with_data_members_die>())
		{
			return ocaml_type(*t.name_here());
		}
		else if (t.is_a<typedef_die>())
		{
			return ocaml_type(*t.name_here());
		}
		else if (t.is_a<base_type_die>())
		{
			/* ctypes already has our names */
			return ocaml_type(*t.name_here());
		}
		else if (t.is_a<pointer_type_die>())
		{
			/* ctypes already has our names */
			return "";
		}
		assert(false);
	};

	std::function<string(iterator_df<type_die>)> reified_type_name
	 = [&reified_type_name](iterator_df<type_die> t) -> string {
		if (!t)
		{
			return ocaml_ident("void");
		}
		else if (t.is_a<with_data_members_die>())
		{
			return ocaml_ident(*t.name_here());
		}
		else if (t.is_a<typedef_die>())
		{
			return ocaml_ident(*t.name_here());
			//return reified_type_name(t.as_a<typedef_die>()->find_type());
		}
		else if (t.is_a<pointer_type_die>())
		{
			auto target_t = t.as_a<pointer_type_die>()->find_type();
			auto unq_target_t = target_t ? target_t->get_unqualified_type() : target_t;

			string target_reified = reified_type_name(target_t);
			return ocaml_ident("ptr_to_" + target_reified);
		}
		else if (t.is_a<base_type_die>())
		{
			return base_type_equiv_and_reified_name(t.as_a<base_type_die>()).second;
		}
		else if (t.is_a<qualified_type_die>())
		{
			// just emit the name of the unqualified type
			return reified_type_name(t->get_unqualified_type());
		}
		assert(false);
	};
	
	std::function<string(iterator_df<type_die>)> reified_type_expr
	 = [&reified_type_name, &reified_type_expr, &reified_type_typ]
	 (iterator_df<type_die> t) -> string {
		if (!t)
		{
			return "void";
		}
		else if (t.is_a<with_data_members_die>())
		{
			/* define each field, then "seal" the whole thing. */
			return "structure \"" + *t.name_here() + "\"";
		}
		else if (t.is_a<typedef_die>())
		{
			// *name* the target type
			return reified_type_name(t.as_a<typedef_die>()->find_type());
		}
		else if (t.is_a<base_type_die>())
		{
			/* ctypes already has our names */
			assert(false); // should never try to define a base type
		}
		else if (t.is_a<pointer_type_die>())
		{
			auto target_t = t.as_a<pointer_type_die>()->find_type();
			auto unq_target_t = target_t ? target_t->get_unqualified_type() : target_t;
			if (unq_target_t && unq_target_t.is_a<base_type_die>()
				&& unq_target_t.as_a<base_type_die>()->get_encoding() == DW_ATE_signed_char
				&& *unq_target_t.as_a<base_type_die>()->get_byte_size() == 1)
			{
				// it's a signed char, so return Ctypes.string
				return "string";
			}			
			/* ctypes already has our names */
			return "ptr " + reified_type_name(t.as_a<pointer_type_die>()->find_type());
		}
		assert(false);
	};
	/* We need to emit the types in dependency order, or some suitable 
	 * approximation thereto. 
	 * 
	 * We do the following.
	 * 
	 * - forward-declare any struct/union/class types; 
	 * - walk each type in the set and post-emit and as-yet-unemitted types.
	 * 
	 * What does it mean to forward-declare, in ocaml-ctypes-land?
	 * It means we name the structure, but don't "seal" it until
	 * we define it. 
	 */
	cout << "open Ctypes" << endl;
	cout << "open Signed" << endl;
	cout << "open Unsigned" << endl;
	cout << "open Foreign" << endl << endl;
	dwarf::core::type_set emitted;
	dwarf::core::type_set finished_emitting;
	
	for (auto i_t = types.begin(); i_t != types.end(); ++i_t)
	{
		if (i_t->is_a<with_data_members_die>() && i_t->name_here())
		{
			/* "forward-declare" the struct/union iff it has a name */
			string name = ocaml_type(*i_t->name_here());
			cout << "(* \"forward declaration\" for " << name << " *)" << endl;
			cout << "type " << name << endl;
			string keyword = i_t->is_a<union_type_die>() ? "union" : "structure";
			cout << "let " << name << " : " << name << " " + keyword + " typ"
				<< " = " << ((keyword == "union") ? "union" : "structure") 
				<< " \"" << name << "\"" << endl;
			emitted.insert(*i_t);
		}
		// if it's a base type, it's already defined by ctypes. 
		// we just note its reified name in -- HACK -- emitted
		if (i_t->is_a<base_type_die>())
		{
			emitted.insert(*i_t);
		}
	}
	
	for (auto i_t = types.begin(); i_t != types.end(); ++i_t)
	{
		walk_type(*i_t, iterator_base::END, 
			[&equiv_for_type, &reified_type_name, &reified_type_typ, &reified_type_expr, &emitted, &finished_emitting]
			(iterator_df<type_die> t, iterator_df<program_element_die> reason) -> bool {
				/* We pre-walk typedefs,because we can emit these
				 * before we emit the definition of the named type
				 * (noting that we forward-declared all structs/unions). 
				 * We will walk the target type separately so there is 
				 * no need to recurse further.
				 */
				if (t.is_a<typedef_die>())
				{
					if (finished_emitting.find(t) == finished_emitting.end())
					{
						string reified_name = reified_type_name(t);

						string equiv = equiv_for_type(t.as_a<typedef_die>()->find_type());
						//cout << "type " << ocaml_type(*t.name_here()) 
						//	<< " = " << equiv << endl;
						// reify the typedef as well
						cout << "let " << reified_name
						// << " : " << reified_type_typ(t) << " typ"
						 << " = " 
						 << (assert(emitted.find(t.as_a<typedef_die>()->find_type()) != emitted.end()), 
							reified_type_name(t.as_a<typedef_die>()->find_type()))
						 << endl;
					} 
					else
					{
						// we've already emitted it; also needn't recurse
					}
					return false;
				} else return true;
			},
			[&equiv_for_type, &reified_type_name, &reified_type_typ, &reified_type_expr, &emitted, &finished_emitting]
			(
				iterator_df<type_die> t, iterator_df<program_element_die> reason
			) {
				//cerr << "Post-walking type " << *t
				//	<< " for reason " << (reason ? reason->summary() : "(initial reason)") << endl;

				/* We emit two things: the "OCaml equivalent" (equiv) and the "definition". */
				// if we haven't emitted this one already...
				// *and* if it's not a qualified type (which we ignore)
				if (t && 
					finished_emitting.find(t) == finished_emitting.end() &&
					t == t->get_unqualified_type()
					)
				{
					cerr << "Emitting " << *t
					<< " for reason " << (reason ? reason->summary() : "(initial reason)") << endl;
					string reified_name = reified_type_name(t);

					// if it's a struct, we emitted some stuff earlier
					if (t.is_a<with_data_members_die>())
					{
						/* define each field, then "seal" the whole thing. */
						auto members = t.as_a<with_data_members_die>().children().subseq_of<member_die>();
						for (auto i_memb = members.first; i_memb != members.second; ++i_memb)
						{
							string field_name = reified_name
								 + "_" + ocaml_ident(*i_memb.base().base().name_here());
							//opt<string> opt_field_type_name = reified_type_name(
							//	i_memb->find_type());
							auto memb_t = i_memb->find_type();
							assert(emitted.find(memb_t) != emitted.end()); // since we are post-emitting
							string field_type_name = reified_type_name(memb_t);
							cout << "let " << field_name << " = field " << reified_name 
								<< " \"" << ocaml_ident(*i_memb.base().base().name_here()) << "\" " 
								<< field_type_name << endl;
						}
						cout << "let () = seal " << reified_name << endl;
					}
					else if (t.is_a<pointer_type_die>())
					{
						opt<string> opt_reified_name = reified_type_name(t);
						assert(opt_reified_name);
						auto pointee_type = t.as_a<pointer_type_die>()->find_type();
						// cout << "type " << *opt_reified_name << endl;
						cout << "let " << *opt_reified_name 
							//<< " : " << reified_type_typ(pointee_type) << " ptr typ"
							<< " = " << reified_type_expr(t) << endl;
					}

					// we inserted the with_data_memberses when we forward-declared them
					if (!t.is_a<with_data_members_die>())
					{
						emitted.insert(t);
					}
					finished_emitting.insert(t);
				}
				else
				{
					// cerr << "Already emitted." << endl;
				}
			}
		);
	}
	
	/* Now process subprograms. */
	for (auto i_i_subp = subprograms.begin(); i_i_subp != subprograms.end(); ++i_i_subp)
	{
		auto i_subp = i_i_subp->second;
		assert(*i_subp.name_here() == i_i_subp->first);
		cout << "let " << *i_subp.name_here() << " = foreign \"" 
			<< *i_subp.name_here() << "\" (";
		auto fps = i_subp.children().subseq_of<formal_parameter_die>();
		unsigned argnum = 0;
		if (fps.first == fps.second)
		{
			cout << "void";
		}
		else for (auto i_fp = std::move(fps.first); i_fp != fps.second; ++i_fp, ++argnum)
		{
			if (argnum != 0) cout << " @-> ";
			auto t = i_fp->find_type();
			cout << reified_type_name(t);
			// cout << equiv_for_type(t);
		}
		cout << " @-> returning " << reified_type_name(i_subp->get_type())
			<< ")" << endl;
	}
	
	// success! 
	return 0;
}
