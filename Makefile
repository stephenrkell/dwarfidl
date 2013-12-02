default: src #lib #test

.PHONY: src
src:
	$(MAKE) -C src

.PHONY: clean
clean:
	$(MAKE) -C src clean
	#rm -f lib/*.{o,so,a} 
	#$(MAKE) -C test clean
