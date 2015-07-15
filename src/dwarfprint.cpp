#include "dwarfprint.hpp"

using boost::format_all;
using boost::match_default;
using boost::optional;
using boost::regex;
using boost::regex_constants::egrep;
using boost::regex_match;
using boost::replace_all;
using boost::smatch;
using dwarf::encap::attribute_value;
using dwarf::core::abstract_die;
using dwarf::core::address_holding_type_die;
using dwarf::core::array_type_die;
using dwarf::core::base_type_die;
using dwarf::core::compile_unit_die;
using dwarf::core::formal_parameter_die;
using dwarf::core::iterator_base;
using dwarf::core::iterator_df;
using dwarf::core::iterator_sibs;
using dwarf::core::member_die;
using dwarf::core::program_element_die;
using dwarf::core::root_die;
using dwarf::core::string_type_die;
using dwarf::core::subprogram_die;
using dwarf::core::subroutine_type_die;
using dwarf::core::type_chain_die;
using dwarf::core::with_type_describing_layout_die;
using dwarf::core::type_describing_subprogram_die;
using dwarf::core::type_die;
using dwarf::core::type_set;
using dwarf::core::variable_die;
using dwarf::core::with_data_members_die;
using dwarf::core::with_dynamic_location_die;
using dwarf::lib::Dwarf_Off;
using dwarf::tool::abstract_c_compiler;
using dwarf::tool::gather_interface_dies;
using dwarf::spec::DEFAULT_DWARF_SPEC;
using namespace dwarf::lib;
using namespace dwarf;
using namespace srk31;
using std::cerr;
using std::cin;
using std::cout;
using std::dec;
using std::deque;
using std::dynamic_pointer_cast;
using std::endl;
using std::hex;
using std::ifstream;
using std::ios;
using std::istringstream;
using std::make_pair;
using std::make_shared;
using std::map;
using std::ostringstream;
using std::pair;
using std::set;
using std::stack;
using std::string;
using std::vector;


template <typename T> inline std::string to_hex(T input) {
	std::ostringstream ss;
	ss.clear();
	ss << "0x" << std::hex << input;
	return ss.str();
}

template <typename T> inline std::string to_dec(T input) {
	std::ostringstream ss;
	ss.clear();
	ss << std::dec << input;
	return ss.str();
}

string opcode_to_string(Dwarf_Loc expr) {
	std::ostringstream ss;
	ss.clear();
	
	auto opcode = string(DEFAULT_DWARF_SPEC.op_lookup(expr.lr_atom));
	if (opcode.compare("(unknown opcode)") == 0) {
		/* Leave unknown opcodes as hex */
		opcode = to_hex(expr.lr_atom);
	} else {
		opcode = opcode.substr(6, string::npos); // remove DW_OP_
	}

	auto arg0 = to_dec(expr.lr_number);
	auto arg1 = to_dec(expr.lr_number2);

	ss << opcode;
	int arg_count = DEFAULT_DWARF_SPEC.op_operand_count(expr.lr_atom);
	if (arg_count > 0) {
		ss << "(" << arg0;
		if (arg_count > 1) {
			ss << ", " << arg1;
		}
		ss << ")";
	}
	
	return ss.str();
}

static inline void _debug_print_dedup(iterator_df<dwarf::core::type_die> type_die, iterator_df<dwarf::core::type_die> dedup_type_iter) {
	 cerr << "Searching type set for";
	 if (type_die.name_here()) {
		  cerr << " '" << *type_die.name_here() << "'";
	 }
	 if (type_die.offset_here()) {
		  cerr << " @" << to_hex(type_die.offset_here());
	 }
	 if (!type_die.name_here() && !type_die.offset_here()) {
		  cerr << " <some anonymous DIE>";
	 }
	 cerr << ", got";
	 if (dedup_type_iter.name_here()) {
		  cerr << " '" << *dedup_type_iter.name_here() << "'";
	 }
	 if (dedup_type_iter.offset_here()) {
		  cerr << " @" << to_hex(dedup_type_iter.offset_here());
	 }
	 if (!dedup_type_iter.name_here() && !dedup_type_iter.offset_here()) {
		  cerr << " <some anonymous DIE>";
	 }
	 cerr << "." << endl;
}

static inline void _debug_print_print(optional<string> name_ptr, Dwarf_Off offset, iterator_df<dwarf::core::type_die> type_die, iterator_df<dwarf::core::type_die> concrete_die) {
	 cerr << "Printing type name for ";
	 if (name_ptr) {
		  cerr << "'" << *name_ptr << "'";
	 } else if (offset) {
		  cerr << "@0x" << to_hex(offset);
	 } else {
		  cerr << "<some anonymous DIE>";
	 }
	 cerr << ": abstract type name";
	 if (type_die->get_name()) {
		  cerr << " = '" << *(type_die->get_name()) << "'";
	 } else {
		  cerr << " is unknown";
	 }
	 if (concrete_die) {
		  cerr << ", concrete type name";
		  if (concrete_die->get_name()) {
			   cerr << " = '" << *(concrete_die->get_name()) << "'";
		  } else {
			   cerr << " is unknown";
		  }
	 } else {
		  cerr << ", no concrete type found";
	 }
	 cerr << "." << endl;

}



void print_type_die(std::ostream &_s, iterator_df<dwarf::core::basic_die> die_iter, optional<type_set&> types) {
	if (!die_iter) return;
	
	// Special case root early: just print all children
	if (die_iter.tag_here() == 0) {
		auto children = die_iter.children_here();
		for (auto iter = children.first; iter != children.second; iter++) {
			 print_type_die(_s, iter.base(), types);
			_s << endl;
		}
		return;
	}
	
	srk31::indenting_newline_ostream s(_s);
	//	auto &die = *die_iter;
	auto name_ptr = die_iter.name_here();
	auto tag = string(DEFAULT_DWARF_SPEC.tag_lookup(die_iter.tag_here())); 
	if (tag.compare("(unknown tag)") == 0) {
		// Leave unknown tags as hex
		tag = to_hex(die_iter.tag_here());
	} else {
		tag = tag.substr(7, string::npos); // remove DW_TAG_
	}
	auto offset = die_iter.offset_here();

	/* Offset, tag, name, type */

	bool offset_printed = false;
	bool name_printed = false;
	bool type_printed = false;
	
	if (offset) {
		s << "@0x" << std::hex << offset << std::dec << " ";
		offset_printed = true;
	}
	
	s << tag;

	if (name_ptr) {
		s << " " << *name_ptr;
		name_printed = true;
	}
	
	auto attrs = die_iter.copy_attrs(die_iter.get_root());
	auto &root = die_iter.get_root();

	/* Convert the type offset into a name if possible. */
	/* Two types (ha) of die with type information:
	   with_type_describing_layout_die => variables, members, etc, things *with* a type
	   type_describing_subprogram_die => subprograms (functions etc) *returning* a thing with a type
	   type_chain_die => things which are types with a type, e.g. typedefs, pointers, arrays
	   (yes, you're right, I can't count)
	*/
	dwarf::core::iterator_df<dwarf::core::type_die> type_die;
	auto with_type_iter = die_iter.as_a<with_type_describing_layout_die>();
	auto returning_type_iter = die_iter.as_a<type_describing_subprogram_die>();
	auto type_chain_iter = die_iter.as_a<type_chain_die>();
	if (with_type_iter) type_die = with_type_iter->get_type();
	else if (returning_type_iter) type_die = returning_type_iter->get_type();
	else if (type_chain_iter) type_die = type_chain_iter->get_type();
	
	if (type_die) {
		 // Dedup
		 if (types) {
			  auto dedup_type_iter = types->find(type_die);
			  //assert(dedup_type_iter != types->end());
			  if (dedup_type_iter != types->end()) {
				   _debug_print_dedup(type_die, *dedup_type_iter);
				   type_die = *dedup_type_iter;
			  }
		 }
		 auto abstract_name = type_die.name_here();
		 // Concretify (traverse typedefs etc)
		 auto concrete_die = type_die->get_concrete_type(root);
		 // Also dedup that. Just in case.
		 if (concrete_die && types) {
			  auto concrete_die_iter = types->find(concrete_die);
			  //assert(concrete_die_iter != types->end());
			  if (concrete_die_iter != types->end()) {
				   _debug_print_dedup(concrete_die, *concrete_die_iter);
				   concrete_die = *concrete_die_iter;
			  }
		 }
		 auto concrete_name = concrete_die.name_here();
		 auto type_name = (concrete_name ? concrete_name : abstract_name);
		 if (type_name) {
			  _debug_print_print(name_ptr, offset, type_die, concrete_die);
			  s << " : " << *type_name;
		 } else {
			  auto type_offset = (concrete_die ? concrete_die.offset_here() : type_die.offset_here());
			  s << " : @" << to_hex(type_offset);
			  
			  if (types->find(type_die) == types->end() && types->find(concrete_die) == types->end()) {
				   cerr << endl << "WARNING: a type was called for that wasn't in types!" << endl << "abstract: ";
				   type_die.print_with_attrs(cerr, 0);
				   cerr << endl << "concrete: ";
				   concrete_die.print_with_attrs(cerr, 0);
				   cerr << endl << "context: ";
				   die_iter.print_with_attrs(cerr, 0);
				   cerr << endl << endl;
			  }
			  type_printed = true;
		 }
		 
	} else {
		 try {
			  auto &v = attrs.at(DW_AT_type);
			  auto ref = v.get_ref();
			  s << " : " << (ref.abs ? "@" : "+") << to_hex(ref.off);
			  type_printed = true;
		 } catch (std::out_of_range) {}
	}
	
	/* Attribute list */

	if (name_printed) attrs.erase(DW_AT_name);
	if (type_printed) attrs.erase(DW_AT_type);
	if (attrs.size() > 0) {
		s << " [";
		s.inc_level();
		unsigned int attrs_printed = 0;
		for (auto iter = attrs.begin(); iter != attrs.end(); iter++) {
			auto pair = *iter;
			auto k = string(DEFAULT_DWARF_SPEC.attr_lookup(pair.first));
			if (k.compare("(unknown attribute)") == 0) {
				// FIXME FIXME FIXME
				continue;
				/* Leave unknown attributes as hex */
				k = to_hex(pair.first);
			} else {
				k = k.substr(6, string::npos); // remove DW_AT_
			}
			auto v = pair.second;
			if (attrs_printed++ != 0) {
				s << "," << endl;
			}
			s << k << " = ";

			switch (v.get_form()) {
			case attribute_value::STRING: {
				string content = v.get_string();
				replace_all(content, "\\", "\\\\");
				replace_all(content, "\n", "\\n");
				replace_all(content, "\"", "\\\"");
				s << "\"" << content << "\"";
			}
				break;
			case attribute_value::FLAG: {
				if (v.get_flag()) {
					s << "true";
				} else {
					s << "false";
				}
			}
				break;
			case attribute_value::UNSIGNED:
				s << to_dec(v.get_unsigned()) << "u";
				break;
			case attribute_value::SIGNED:
				s << to_dec(v.get_signed());
				break;
			case attribute_value::ADDR:
				s << to_hex(v.get_address());
				break;
			case attribute_value::REF: {
				auto ref = v.get_ref();
				if (ref.abs) {
					s << "@";
				} else {
					s << "+";
				}
				s << to_hex(ref.off);
			}
				break;
			case attribute_value::LOCLIST: {
				auto loclist = v.get_loclist();
				if (loclist.size() == 1 && loclist[0].lopc == 0 && (loclist[0].hipc == 0 || loclist[0].hipc == std::numeric_limits<Dwarf_Addr>::max())) {
					s << "{";
					s.inc_level();
					auto locexpr = loclist[0];
					for (auto iter = locexpr.begin(); iter != locexpr.end(); iter++) {
						if (iter != locexpr.begin()) {
							s << endl;
						}
						s << opcode_to_string(*iter) << ";";
					}
					s.dec_level();
					s << "}";
					
				} else {
					s << "/* FIXME dummy location for multiple vaddr loclist */ { addr(0); }";
				}
			}
				break;
			default:
				s << "/* FIXME */ \"" << v << "\"";
				break;
			}
		}
		s.dec_level();
		s << "]";
	}

	/* Children list (recursively) */

	// auto lambda = [] (iterator_base &iter) {
	// 	return (iter.is_a<compile_unit_die>() || iter.is_a<member_die>() || iter.is_a<formal_parameter_die>() || iter.is_a<subprogram_die>() || iter.is_a<root_die>());
	// };
	auto children = die_iter.children_here(); //.subseq_with<decltype(lambda)>(lambda);
	
	if (children.first != children.second) {
		s << " {";
		s.inc_level();
		for (auto iter = children.first; iter != children.second; iter++) {
			switch (iter.tag_here()) {
				// Only print these tags
			case DW_TAG_compile_unit:
			case DW_TAG_member:
			case DW_TAG_formal_parameter:
			case DW_TAG_subprogram:
			case 0: // root
				 print_type_die(s, iter.base(), types);
				s << endl;
				break; // definitely
			default:    // not
				 print_type_die(s, iter.base(), types);
				 s << endl;
				 break;
				 
				 //	continue; // confusing
			}
		}
		s.dec_level();
		s << "}";
	}
	
	s << ";";
}

string dies_to_idl(set<iterator_base> dies, optional<type_set&> types) {
	ostringstream ss;
	ss.clear();
	for (auto iter = dies.begin(); iter != dies.end(); iter++) {
		 print_type_die(ss, *iter, types);
		ss << endl << endl;
	}
	return ss.str();
}

void print_dies(ostream &s, set<iterator_base> dies, optional<type_set&> types) {
	for (auto iter = dies.begin(); iter != dies.end(); iter++) {
		 print_type_die(s, *iter, types);
		 s << endl << endl;
	}
}



