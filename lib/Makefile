all: libdwarfidl.so libdwarfidl.so.0

%: ../src/.libs/%
	ln -sf "$<" "$@"

clean:
	rm -f libdwarfidl.so libdwarfidl.so.0
