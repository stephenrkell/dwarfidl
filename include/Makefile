all: dwarfidl/dwarfidlNewCLexer.h dwarfidl/dwarfidlNewCParser.h

dwarfidl/%: ../parser/%
	ln -sf ../"$<" "$@"

clean:
	rm -f dwarfidl/dwarfidlNewCLexer.h dwarfidl/dwarfidlNewCParser.h
