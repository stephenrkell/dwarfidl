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

#include "dwarfidl/dwarf_interface_walk.hpp"

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
using std::queue;

using std::make_pair;
using std::unordered_set;
using namespace dwarf;
using namespace dwarf::core;

// using dwarf::core::root_die;
// using dwarf::core::iterator_base;
// using dwarf::core::iterator_df;
// using dwarf::core::iterator_sibs;
// using dwarf::core::basic_die;
// using dwarf::core::type_set;
// using dwarf::core::type_die;
// using dwarf::core::subprogram_die;
// using dwarf::core::formal_parameter_die;
// using dwarf::core::compile_unit_die;
// using dwarf::core::typedef_die;
// using dwarf::core::member_die;
// using dwarf::core::with_data_members_die;
// using dwarf::core::variable_die;
// using dwarf::core::program_element_die;
// using dwarf::core::with_dynamic_location_die;
// using dwarf::core::with_type_describing_layout_die;
// using dwarf::core::type_describing_subprogram_die;
// using dwarf::core::address_holding_type_die;
// using dwarf::core::pointer_type_die;
// using dwarf::core::union_type_die;
// using dwarf::core::array_type_die;
// using dwarf::core::type_chain_die;
// using dwarf::core::base_type_die;
// using dwarf::core::qualified_type_die;
using dwarf::spec::opt;

using dwarf::lib::Dwarf_Off;

// regex usings
using boost::regex;
using boost::regex_match;
using boost::smatch;
using boost::regex_constants::egrep;
using boost::match_default;
using boost::format_all;

namespace dwarf {
	 namespace tool {

		  // void my_walk_type(iterator_df<type_die> t, iterator_df<program_element_die> reason, 
		  // 					const std::function<bool(iterator_df<type_die>, iterator_df<program_element_die>)>& pre_f, 
		  // 					const std::function<void(iterator_df<type_die>, iterator_df<program_element_die>)>& post_f,
		  // 					const type_set& currently_walking  = type_set())
		  // {
		  // 	   if (currently_walking.find(t) != currently_walking.end()) return; // "grey node"
			
		  // 	   bool continue_recursing;
		  // 	   if (pre_f) continue_recursing = pre_f(t, reason); // i.e. we do walk "void"
		  // 	   else continue_recursing = true;
			
		  // 	   type_set next_currently_walking = currently_walking; 
		  // 	   next_currently_walking.insert(t);
			
		  // 	   if (continue_recursing)
		  // 	   {

		  // 			if (!t) { /* void case; just post-visit */ }
		  // 			else if (t.is_a<type_chain_die>()) // unary case -- includes typedefs, arrays, pointer/reference, ...
		  // 			{
		  // 				 // recursively walk the chain's target
		  // 				 my_walk_type(t.as_a<type_chain_die>()->find_type(), t, pre_f, post_f, next_currently_walking);
		  // 			}
		  // 			else if (t.is_a<with_data_members_die>()) 
		  // 			{
		  // 				 // recursively walk all members
		  // 				 auto member_children = t.as_a<with_data_members_die>().children().subseq_of<member_die>();
		  // 				 for (auto i_child = member_children.first;
		  // 					  i_child != member_children.second; ++i_child)
		  // 				 {
		  // 					  my_walk_type(i_child->find_type(), i_child.base().base(), pre_f, post_f , next_currently_walking);
		  // 				 }
		  // 				 // visit all inheritances
		  // 				 auto inheritance_children = t.as_a<with_data_members_die>().children().subseq_of<inheritance_die>();
		  // 				 for (auto i_child = inheritance_children.first;
		  // 					  i_child != inheritance_children.second; ++i_child)
		  // 				 {
		  // 					  my_walk_type(i_child->find_type(), i_child.base().base(), pre_f, post_f , next_currently_walking);
		  // 				 }
		  // 			}
		  // 			else if (t.is_a<subrange_type_die>())
		  // 			{
		  // 				 // visit the base type
		  // 				 auto explicit_t = t.as_a<subrange_type_die>()->find_type();
		  // 				 // HACK: assume this is the same as for enums
		  // 				 my_walk_type(explicit_t ? explicit_t : t.enclosing_cu()->implicit_enum_base_type(), t, pre_f, post_f, next_currently_walking);
		  // 			}
		  // 			else if (t.is_a<enumeration_type_die>())
		  // 			{
		  // 				 // visit the base type -- HACK: assume subrange base is same as enum's
		  // 				 auto explicit_t = t.as_a<enumeration_type_die>()->find_type();
		  // 				 my_walk_type(explicit_t ? explicit_t : t.enclosing_cu()->implicit_enum_base_type(), t, pre_f, post_f, next_currently_walking);
		  // 			}
		  // 			else if (t.is_a<type_describing_subprogram_die>())
		  // 			{
		  // 				 auto sub_t = t.as_a<type_describing_subprogram_die>();
		  // 				 my_walk_type(sub_t->find_type(), sub_t, pre_f, post_f, next_currently_walking);
		  // 				 auto fps = sub_t.children().subseq_of<formal_parameter_die>();
		  // 				 for (auto i_fp = fps.first; i_fp != fps.second; ++i_fp)
		  // 				 {
		  // 					  my_walk_type(i_fp->find_type(), i_fp.base().base(), pre_f, post_f, next_currently_walking);
		  // 				 }
		  // 			}
		  // 			else
		  // 			{
		  // 				 // what are our nullary cases?
		  // 				 assert(t.is_a<base_type_die>() || t.is_a<unspecified_type_die>());
		  // 			}
		  // 	   } // end if continue_recursing

		  // 	   if (post_f) post_f(t, reason);
		  // }


		  
		  void gather_interface_dies(root_die& root, 
									 set<iterator_base>& out, type_set& dedup_types_out, 
									 std::function<bool(const iterator_base&)> pred) {
			   /* We want to deduplicate the types that we output, as we go. 
				* CARE: what about anonymous types? We might want to generate names for them 
				* based on their offset, in which case deduplication isn't transparent. 
				* For this reason we output dedup_types_out separately from the DIEs.
				* However, everything in dedup_types_out is also in out (FIXME: is this a good idea?) */
			   type_set& types = dedup_types_out;

			   deque<iterator_sibs<core::basic_die> > to_walk;

			   set<iterator_sibs<core::basic_die> > wanted;

			   std::function<void(iterator_sibs<core::basic_die>, iterator_sibs<core::basic_die>)> find_wanted = [&wanted, &pred, &find_wanted](iterator_sibs<core::basic_die> begin, iterator_sibs<core::basic_die> end) {
					for (auto iter = begin; iter != end; iter++) {
						 if (pred(iter)) {
							  wanted.insert(iter);
							  cerr << "=== Selecting: "; // << iter.summary() << endl;
							  iter.print_with_attrs(cerr, 0);
						 } else {
//							  cerr << "Not selecting: " << iter.summary() << endl;
						 }
						 find_wanted(iter.children_here().first, iter.children_here().second);
					}
			   };

			   std::function<void(const iterator_df<type_die>)> find_types = [&root, &types, &find_types](const iterator_df<type_die> die) {
					// try {
					// 	 if (types.find(die) != types.end()) return;
					// } catch (std::bad_cast e) {
					// 	 // hack fixme
					// 	 cerr << "OOPS caught bad_cast" << endl;
					// 	 return;
					// }

					if (!die || !die.is_real_die_position() || !die.is_root_position()) return;
					
					
					cerr << "::::: find_types called for " << die.summary() << endl;
					
					
					auto actual_type_die = die.as_a<type_die>();
					auto chain_die = die.as_a<type_chain_die>();
					auto die_with_type = die.as_a<with_type_describing_layout_die>();
					auto die_returning_type = die.as_a<type_describing_subprogram_die>();
					auto with_members_die = die.as_a<with_data_members_die>();
					
					bool found = false;
					
					if (chain_die && chain_die->get_type()) {
						 iterator_df<core::type_die> i = chain_die->get_type();
						 find_types(i);
					} else if (die_with_type && die_with_type->get_type()) {
						 iterator_df<core::type_die> i = die_with_type->get_type();
						 find_types(i);
					} else if (die_returning_type && die_returning_type->get_type()) {
						 iterator_df<core::type_die> i = die_returning_type->get_type();
						 find_types(i);
						 // auto param_children = die.children_here().subseq_of<formal_parameter_die>();
						 // for (auto iter = param_children.first; iter != param_children.second; iter++) {
						 // 	  find_types(iter.base().base());
						 // }
					} else if (with_members_die) {
						 // auto member_children = die.children_here().subseq_of<member_die>();
						 // auto inheritance_children = die.children_here().subseq_of<inheritance_die>();
						 // for (auto iter = member_children.first; iter != member_children.second; iter++) {
						 // 	  find_types(iter.base().base());
						 // }
						 // for (auto iter = inheritance_children.first; iter != inheritance_children.second; iter++) {
						 // 	  find_types(iter.base().base());
						 // }
						 
					}

					auto inserted = types.insert(actual_type_die);
					found = !inserted.second;

					if (!found) {
						 for (auto iter = die.children_here().first; iter != die.children_here().second; iter++) {
							  find_types(iter);
						 }
					}
			   };
			   
			   find_wanted(root.children().first.base().base(), root.children().second.base().base());
			   
			   cerr << "===== WANTED =====" << endl;
			   for (auto iter = wanted.begin(); iter != wanted.end(); iter++) {
					cerr << iter->summary() << endl;
					find_types(*iter);
					out.insert(*iter);
			   }

			   cerr << "===== TYPES =====" << endl;
			   
			   for (auto iter = types.begin(); iter != types.end(); iter++) {
					cerr << iter->summary() << endl;
					out.insert(*iter);
			   }

			   /* returned stuff is in &out and &dedup_types_out */
		  }

	 }

}

			   

			   // std::function<void(iterator_base begin, iterator_base end)> process_die = [&root, &out, &types, &pred](iterator_base begin, iterator_base end){
// 					for (auto iter = begin; iter != end; iter++) {

// 						 if (!iter) continue;
						 
// 						 bool was_chosen = pred(iter.base());

// 						 if (pred(iter.base())) {
							  
// 						 auto actual_type_die = iter.as_a<type_die>();
// 						 auto chain_die = iter.as_a<type_chain_die>();
// 						 auto die_with_type = iter.as_a<with_type_describing_layout_die>();
// 						 auto die_returning_type = iter.as_a<type_describing_subprogram_die>();

// 						 type_die *t;
						 
// 						 if (actual_type_die) {
// 							  t = &actual_type_die->get_type();
// 						 } else if (chain_die) {
// 							  t = &chain_die->get_type();
// 						 } else if (die_with_type) {
// 							  t = &die_with_type->get_type();
// 						 } else if (die_returning_type) {
// 							  t = &die_returning_type->get_type();
// 						 }

// 						 if (was_chosen && t) {
// 							  auto inserted = types.insert(t);
// 							  if (!inserted.second)
// 							  {
// 								   cerr << "Type was already present: " << *t 
// 										<< " (or something equal to it: " << *inserted.first
// 										<< ")" << endl;
// 								   cerr << "Attributes: " << t->copy_attrs(root) << endl;
// //								   return true; // was already present
// 							  }
// 							  else
// 							  {
// 								   cerr << "Inserted new type: " << *t << endl;
// 								   cerr << "Attributes: " << t->copy_attrs(root) << endl;
// 								   process_die();
// //								   return true;
// 							  }
						 
// 							  auto children = iter.children_here();
// 							  process_die(children.first, children.second);
							  
// 						 }
// 					}
// 			   }
			   
// 			   process_die(root.children().begin, root.children().second);
			   
// 			   for (auto i_d = std::move(toplevel_seq.first); i_d != toplevel_seq.second; ++i_d)
// 			   {
// 					if (pred(i_d.base().base()))
// 					{
// 						 auto add_all_types = [&types, &root](iterator_df<type_die> outer_t) {
// 							  my_walk_type(outer_t, iterator_base::END,
// //						  [](iterator_df<type_die> t, iterator_df<program_element_die> reason) -> bool { return t && (t.is_real_die_position() || t.is_root_position()); },
// 										   [&types, &root](iterator_df<type_die> t, iterator_df<program_element_die> reason) -> bool {
// 												// if (/*t && */(t.is_real_die_position() || t.is_root_position())) {
// 												// 		types.insert(t);
// 												// }
								 
// 												// auto inserted = types.insert(t);
// 												if (!t) return false; // void case
// 												// auto memb = reason.as_a<member_die>();
// 												// // if (memb && memb->get_declaration() && *memb->get_declaration()
// 												// // 	&& memb->get_external() && *memb->get_external())
// 												// // {
// 												// // 	// static member vars don't get added nor recursed on
// 												// // 	return false;
// 												// //}
// 												// // apart from that, insert all nonvoids....
// 												auto inserted = types.insert(t);
// 												if (!inserted.second)
// 												{
// 													 cerr << "Type was already present: " << *t 
// 														  << " (or something equal to it: " << *inserted.first
// 														  << ")" << endl;
// 													 cerr << "Attributes: " << t->copy_attrs(root) << endl;
// 													 return true; // was already present
// 												}
// 												else
// 												{
// 													 cerr << "Inserted new type: " << *t << endl;
// 													 cerr << "Attributes: " << t->copy_attrs(root) << endl;
// 													 return true;
// 												}
// 										   }, [](iterator_df<type_die>, iterator_df<program_element_die>){}, types
// 								   );
// 						 };

// 						 set<iterator_base> seen;

// 						 std::function<void(const iterator_base&, bool)> process_types = [&seen, &types, &out, &add_all_types, &process_types](const iterator_base &i_d, bool child) {
// 							  //if (out.find(i_d) != out.end()) return;
				  
// 							  // looks like a goer -- add it to the objs
				  
// 							  /* Also output everything that this depends on. We have to case-split
// 							   * for now. */

// 							  cerr << "==== Entering process_types for " << i_d.summary() << endl;

// 							  if (seen.find(i_d) != seen.end()) {
// 								   return;
// 							  }
				  

// 							  seen.insert(i_d);
				  				  
// 							  if (i_d.is_a<type_die>()) {
// 								   /* If it is itself a type, start walking it */
// //					   add_all_types(i_d.as_a<type_die>());
// 							  } else if (!child) {
// 								   /* Otherwise it might be something that *has* a type, e.g. variable */
// 								   auto die_with_type = i_d.as_a<with_type_describing_layout_die>();
// 								   auto die_returning_type = i_d.as_a<type_describing_subprogram_die>();
// //								   auto chain_die = i_d.as_a<type_chain_die>();
// 								   if (die_with_type) {
// 										if (die_with_type->get_type()) {
// 											 add_all_types(die_with_type->get_type());
// 										}
// 								   } else if (die_returning_type) {
// 										/* or it might be something with a return type, e.g. subprogram */
// 										if (die_returning_type->get_type()) {
// 											 add_all_types(die_returning_type->get_type());
// 										}
// //								   } else if (chain_die) {
// 										/* or it might be a type chain e.g. typedef, array, etc */
// 										/*if (chain_die->get_type()) {
// 										  add_all_types(chain_die->get_type());
// 										  }*/
// 										//	   }
// 							  }
				  
// 							  auto children = i_d.children_here();
// 							  auto children_begin = children.first;
// 							  auto children_end = children.second;
				  
// 							  for (auto child_iter = children_begin; child_iter != children_end; child_iter++) {
					   
// 								   if (seen.find(child_iter) == seen.end()) {
// 										process_types(child_iter, false);
// 								   }
// 							  }
				  
// 						 };

// 						 out.insert(i_d.base().base());
// 						 process_types(i_d.base().base(), false);
			 
			 
// 					} // end if pred
// 			   } // end for
	
// 			   /* Check that everything that's in "out", if it is a type, is also in 
// 				* types. */
// 			   for (auto i_d = out.begin(); i_d != out.end(); ++i_d)
// 			   {
// 					if (i_d->is_a<type_die>() && !i_d->is_a<subprogram_die>())
// 					{
// 						 if (types.find(i_d->as_a<type_die>()) == types.end())
// 						 {
// 							  cerr << "BUG: didn't find " << i_d->summary() << " in types list." << endl;
// 						 }
// 					}
// 			   }
	
// 			   /* Now we've gathered everything. Make sure everything in "types" is in 
// 				* "out". */
// 			   for (auto i_d = types.begin(); i_d != types.end(); ++i_d)
// 			   {
// 					out.insert(*i_d);
// 			   }
	
// 		  }
		  
// 	 } }
