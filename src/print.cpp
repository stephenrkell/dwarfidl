#include <dwarfpp/adt.hpp>

/* Here we print a libdwarfpp dieset in dwarfidl syntax. 
 *
 * Test 1: our output should always parse as dwarfidl. 
 * Test 2: our output should parse to a dieset similar to the input! 
 */


// for each kind of DIE, define 
// pre-children and post-children print methods;
// the main algorithm does depth-first traversal,
// running pre-children in the pre-order
// and     post-children in the post-order;
// how to achieve this with our iterators?
// depthfirst iterators give us pre-order;
// we speculatively increment, and if moving up or across (i.e. not down), do the post thing

// write templates parameterised by
// tag
// address of the dwarf::spec object (noting that we can do template spec on this!)

// then dispatch in a custom dispatcher
