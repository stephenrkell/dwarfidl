default: src.build printer.build lib.build #test

.PHONY: parser.build
parser.build:
	$(MAKE) -C parser

.PHONY: src.build
src.build: parser.build
	$(MAKE) -C src

.PHONY: printer.build
printer.build: src.build lib.build
	$(MAKE) -C printer

lib.build: src.build
	mkdir -p lib && cd lib && ln -sf ../src/libdwarfidl.a ../src/libdwarfidl.so .

.PHONY: clean
clean:
	$(MAKE) -C src clean
	rm -f lib/*.o lib/*.so lib/*.a  || true
	#$(MAKE) -C test clean
