default: src printer lib #test

.PHONY: parser
parser:
	$(MAKE) -C parser

.PHONY: src
src: parser
	$(MAKE) -C src

.PHONY: printer
printer: src lib
	$(MAKE) -C printer

lib: src
	mkdir -p lib && cd lib && ln -sf ../src/libdwarfidl.a ../src/libdwarfidl.so .

.PHONY: clean
clean:
	$(MAKE) -C src clean
	rm -f lib/*.o lib/*.so lib/*.a  || true
	#$(MAKE) -C test clean
