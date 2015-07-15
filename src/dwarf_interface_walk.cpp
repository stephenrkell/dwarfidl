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
using std::make_pair;
using std::unordered_set;
using namespace dwarf;
using namespace dwarf::core;

// //using boost::filesystem::path;
// using dwarf::core::root_die;
// using dwarf::core::iterator_base;
// using dwarf::core::iterator_df;
// using dwarf::core::iterator_sibs;
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

using std::function;


namespace dwarf { namespace tool {


		  
// 		  template <typename Item>
// class mru_list
// {
//   typedef multi_index_container<
//     Item,
//     indexed_by<
//       sequenced<>,
//       hashed_unique<identity<Item> >
//     >
//   > item_list;

// public:
//   typedef Item                         item_type;
//   typedef typename item_list::iterator iterator;

//   mru_list(std::size_t max_num_items_):max_num_items(max_num_items_){}

//   void insert(const item_type& item)
//   {
//     std::pair<iterator,bool> p=il.push_front(item);

//     if(!p.second){                     /* duplicate item */
//       il.relocate(il.begin(),p.first); /* put in front */
//     }
//     else if(il.size()>max_num_items){  /* keep the length <= max_num_items */
//       il.pop_back();
//     }
//   }

//   iterator begin(){return il.begin();}
//   iterator end(){return il.end();}

// private:
//   item_list   il;
//   std::size_t max_num_items;


// };



		  
		  void my_walk_type(iterator_df<type_die> t, iterator_df<program_element_die> reason, 
							const std::function<bool(iterator_df<type_die>, iterator_df<program_element_die>)>& pre_f, 
							const std::function<void(iterator_df<type_die>, iterator_df<program_element_die>)>& post_f
							= std::function<void(core::iterator_df<core::type_die>, core::iterator_df<core::program_element_die>)>(),
							const type_set& currently_walking = type_set()
							// const mru_list<type_die> &cache = mru_list<type_die>()
			   )
		  {
//			   if (cache.find(t) != cache.end()) return;
			if (currently_walking.find(t) != currently_walking.end()) return; // "grey node"
			
			bool continue_recursing;
			if (pre_f) continue_recursing = pre_f(t, reason); // i.e. we do walk "void"
			else continue_recursing = true;
			
			type_set next_currently_walking = currently_walking; 
			next_currently_walking.insert(t);
			
			if (continue_recursing)
			{

				if (!t) { /* void case; just post-visit */ }
				else if (t.is_a<type_chain_die>()) // unary case -- includes typedefs, arrays, pointer/reference, ...
				{
					// recursively walk the chain's target
					 my_walk_type(t.as_a<type_chain_die>()->find_type(), t, pre_f, post_f, next_currently_walking);
				}
				else if (t.is_a<with_data_members_die>()) 
				{
					// recursively walk all members
					auto member_children = t.as_a<with_data_members_die>().children().subseq_of<member_die>();
					for (auto i_child = member_children.first;
						i_child != member_children.second; ++i_child)
					{
						my_walk_type(i_child->find_type(), i_child.base().base(), pre_f, post_f , next_currently_walking);
					}
					// visit all inheritances
					auto inheritance_children = t.as_a<with_data_members_die>().children().subseq_of<inheritance_die>();
					for (auto i_child = inheritance_children.first;
						i_child != inheritance_children.second; ++i_child)
					{
						 my_walk_type(i_child->find_type(), i_child.base().base(), pre_f, post_f , next_currently_walking);
					}

					/*auto children_with_type = t.as_a<with_data_members_die>().children().subseq_of<with_type_describing_layout_die>();
					for (auto i_child = children_with_type.first; i_child != children_with_type.second; i_child++) {
						 my_walk_type(i_child->find_type(), i_child.base().base(), pre_f, post_f , next_currently_walking);
						 }*/
					}
				else if (t.is_a<subrange_type_die>())
				{
					// visit the base type
					auto explicit_t = t.as_a<subrange_type_die>()->find_type();
					// HACK: assume this is the same as for enums
					my_walk_type(explicit_t ? explicit_t : t.enclosing_cu()->implicit_enum_base_type(), t, pre_f, post_f, next_currently_walking);
				}
				else if (t.is_a<enumeration_type_die>())
				{
					// visit the base type -- HACK: assume subrange base is same as enum's
					auto explicit_t = t.as_a<enumeration_type_die>()->find_type();
					my_walk_type(explicit_t ? explicit_t : t.enclosing_cu()->implicit_enum_base_type(), t, pre_f, post_f, next_currently_walking);
				}
				else if (t.is_a<type_describing_subprogram_die>())
				{
					auto sub_t = t.as_a<type_describing_subprogram_die>();
					my_walk_type(sub_t->find_type(), sub_t, pre_f, post_f);
					auto fps = sub_t.children().subseq_of<formal_parameter_die>();
					for (auto i_fp = fps.first; i_fp != fps.second; ++i_fp)
					{
						my_walk_type(i_fp->find_type(), i_fp.base().base(), pre_f, post_f, next_currently_walking);
					}
					/*auto vars = sub_t.children().subseq_of<variable_die>();
					for (auto i_fp = vars.first; i_fp != vars.second; ++i_fp)
					{
						my_walk_type(i_fp->find_type(), i_fp.base().base(), pre_f, post_f, next_currently_walking);
						}*/
				
				}
				else
				{
					// what are our nullary cases?
					assert(t.is_a<base_type_die>() || t.is_a<unspecified_type_die>());

/*					function<void(iterator_df<type_die>,
								  iterator_df<program_element_die>)> traverse_children = [&traverse_children,
																						  &pre_f,
																						  &post_f,
																						  &next_currently_walking](iterator_df<type_die> t,
																										 iterator_df<program_element_die> reason){
						 auto children = t.children();
						 for (auto iter = children.first; iter != children.second; iter++) {
							  auto child_type = iter.base().as_a<type_die>();
							  auto child_with_type = iter.base().as_a<with_type_describing_layout_die>();
							  auto child_returning_type = iter.base().as_a<type_describing_subprogram_die>();
							  if (child_type) {
								   my_walk_type(child_type, t, pre_f, post_f, next_currently_walking);
							  } else if (child_with_type) {
								   my_walk_type(child_with_type->get_type(), t, pre_f, post_f, next_currently_walking);
							  } else if (child_returning_type) {
								   my_walk_type(child_returning_type->get_type(), t, pre_f, post_f, next_currently_walking);
							  } else {
								   traverse_children(iter, t);
							  }
						 }
					};

					traverse_children(t, reason);*/
					
				}
			} // end if continue_recursing

			if (post_f) post_f(t, reason);
		}


		  

void 
gather_interface_dies(root_die& root, 
	set<iterator_base>& out, type_set& dedup_types_out, 
	std::function<bool(const iterator_base&)> pred)
{
	/* We want to deduplicate the types that we output, as we go. 
	 * CARE: what about anonymous types? We might want to generate names for them 
	 * based on their offset, in which case deduplication isn't transparent. 
	 * For this reason we output dedup_types_out separately from the DIEs.
	 * However, everything in dedup_types_out is also in out (FIXME: is this a good idea?) */
	auto toplevel_seq = root.grandchildren();
	type_set& types = dedup_types_out;
	/* FIXME: it needn't be just grandchildren. */
	for (auto i_d = std::move(toplevel_seq.first); i_d != toplevel_seq.second; ++i_d)
	{
		if (pred(i_d.base().base()))
		{
			// looks like a goer -- add it to the objs
			out.insert(i_d.base().base());
			
			/* utility that will come in handy */
			auto add_all_types = [&types, &root](iterator_df<type_die> outer_t) {
				 if (outer_t && outer_t.offset_here() == 0x5ae511d) {
					  cerr << "found the thing." << endl;
				 }

				 if (outer_t) {
					  cerr << "add_all_types processing offset 0x" << std::hex <<  outer_t.offset_here() << std::dec << ": " << outer_t.summary() << endl;
					  
				 }
				 
				my_walk_type(outer_t, iterator_base::END, 
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
							cerr << "Type was already present: " << *t 
								<< " (or something equal to it: " << *inserted.first
								<< ")" << endl;
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
			
			/* Also output everything that this depends on. We have to case-split
			 * for now. */
			if (i_d.base().base().is_a<variable_die>())
			{
				add_all_types(i_d.base().base().as_a<variable_die>()->get_type());
			}
			else if (i_d.base().base().is_a<type_die>())
			{
				/* Just walk it. */
				add_all_types(i_d.base().base().as_a<type_die>());
			}
		} // end if pred
	} // end for
	
	/* Now we've gathered everything. Make sure everything in "types" is in 
	 * "out". */
	for (auto i_d = types.begin(); i_d != types.end(); ++i_d)
	{
		out.insert(*i_d);
	}
	
	/* Check that everything that's in "out", if it is a type, is also in 
	 * types. */
	for (auto i_d = out.begin(); i_d != out.end(); ++i_d)
	{
		if (i_d->is_a<type_die>() && !i_d->is_a<subprogram_die>())
		{
			if (types.find(i_d->as_a<type_die>()) == types.end())
			{
				cerr << "BUG: didn't find " << i_d->summary() << " in types list." << endl;
			}
		}
	}
}

} }
