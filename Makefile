default: src lib #test

.PHONY: src
src:
	$(MAKE) -C src

lib: src
	mkdir -p lib && cd lib && ln -sf ../src/libdwarfidl.a .

.PHONY: clean
clean:
	$(MAKE) -C src clean
	rm -f lib/*.o lib/*.so lib/*.a  || true
	#$(MAKE) -C test clean
