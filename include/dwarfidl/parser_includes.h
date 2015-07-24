#ifndef _FOOTPRINT_PARSER_INCLUDES_H
#define _FOOTPRINT_PARSER_INCLUDES_H

// TODO FIXME HACK: antlr3 includes its own autotools config.h
// and I want to use -Werror
#undef PACKAGE
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION
#undef VERSION

#include <antlr3.h>
#include <antlr3defs.h>
#include "dwarfidlSimpleCLexer.h"
#include "dwarfidlSimpleCParser.h"

#endif // _FOOTPRINT_PARSER_INCLUDES_H
