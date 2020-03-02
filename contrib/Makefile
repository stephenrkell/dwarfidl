THIS_MAKEFILE := $(realpath $(lastword $(MAKEFILE_LIST)))
CONTRIB_ROOT := $(realpath $(dir $(THIS_MAKEFILE)))

default: env.sh config.mk build-libcxxgen \
build-libantlr3c++ build-libantlr3c build-libdwarfpp

.PHONY: clean default

CONFIG_MK += include $(realpath $(dir $(lastword $(MAKEFILE_LIST))))/libdwarfpp/contrib/config.mk

# ----------------------------antlr things
antlr-3.4.tar.gz:
	wget http://www.antlr3.org/download/antlr-3.4.tar.gz

libantlr3c-3.4/configure: antlr-3.4.tar.gz
	mkdir -p libantlr3c-3.4 && cd libantlr3c-3.4 && tar --strip-components=3 --keep-newer-files -xvzf "$(CONTRIB_ROOT)/$<" antlr-3.4/runtime/C
	# HACK: if the patch fails, it's probably because we've already applied it in an earlier failed run of these rules
	cd libantlr3c-3.4 && patch -p1 < "$(CONTRIB_ROOT)"/libantlr3c-3.4-getparent-allow-null.patch </dev/null || true
	cd libantlr3c-3.4 && aclocal && \
                autoconf && \
                autoheader && \
                libtoolize && \
                automake --add-missing && \
                automake
	chmod +x "$@"
	touch "$@"

antlr-3.4-complete.jar: antlr-3.4.tar.gz
	tar --strip-components=2 --keep-newer-files -xvzf "$(CONTRIB_ROOT)/$<" antlr-3.4/lib/antlr-3.4-complete.jar

libantlr3c-3.4/Makefile: libantlr3c-3.4/configure
	cd libantlr3c-3.4 && ./configure --enable-64bit

.PHONY: build-libantlr3c
build-libantlr3c: libantlr3c-3.4/Makefile
	$(MAKE) -C libantlr3c-3.4

libantlr3c-3.4/antlr3config.h: build-libantlr3c

clean::
	rm -f antlr-3.4.tar.gz
	rm -rf libantlr3c-3.4

CONFIG_MK += \nANTLR3C_DIR := $(CONTRIB_ROOT)/libantlr3c-3.4\n
CONFIG_MK += \nCXXFLAGS += -I\$$(ANTLR3C_DIR)/include -I\$$(ANTLR3C_DIR)/\n
CONFIG_MK += \nLDFLAGS += -L\$$(ANTLR3C_DIR)/.libs -Wl,-rpath,\$$(ANTLR3C_DIR)/.libs\n

# ----------------------------libdwarfpp

LIBDWARFPP_DIR ?= $(CONTRIB_ROOT)/libdwarfpp
LIBCXXFILENO_DIR ?= $(LIBDWARFPP_DIR)/contrib/libc++fileno
LIBSRK31CXX_DIR ?= $(LIBDWARFPP_DIR)/contrib/libsrk31c++

libdwarfpp/config.status:
	cd libdwarfpp && ./autogen.sh && git submodule init && git submodule update && \
	$(MAKE) -C contrib && ( . contrib/env.sh && \
	 ./configure --prefix=$(CONTRIB_ROOT)/usr $(LIBDWARFPP_CONFIGURE_FLAGS) )

.PHONY: build-libdwarfpp
build-libdwarfpp: libdwarfpp/config.status
	$(MAKE) -C libdwarfpp

clean::
	if test -f libdwarfpp/Makefile; then $(MAKE) -C libdwarfpp clean; fi

CONFIG_MK += \nLIBDWARFPP_CXXFLAGS += -I$(LIBDWARFPP_DIR)/include\n
CONFIG_MK += \nLIBDWARFPP_LIBS += -L$(LIBDWARFPP_DIR)/lib -Wl,-rpath,$(LIBDWARFPP_DIR)/lib -ldwarfpp\n
CONFIG_MK += \nLIBCXXFILENO_CXXFLAGS += -I$(LIBCXXFILENO_DIR)/include\n
CONFIG_MK += \nLIBCXXFILENO_LIBS += -L$(LIBCXXFILENO_DIR)/lib -Wl,-rpath,$(LIBCXXFILENO_DIR)/lib -lc++fileno\n
CONFIG_MK += \nLIBSRK31CXX_CXXFLAGS += -I$(LIBSRK31CXX_DIR)/include\n
CONFIG_MK += \nLIBSRK31CXX_LIBS += -L$(LIBSRK31CXX_DIR)/lib -Wl,-rpath,$(LIBSRK31CXX_DIR)/lib -lsrk31c++\n

# ----------------------------libcxxgen

libcxxgen/config.status: build-libdwarfpp
	cd libcxxgen &&  aclocal && \
                autoconf && \
                libtoolize && \
                automake --add-missing && \
                automake &&  \
	LIBCXXFILENO_CXXFLAGS="-I$(LIBCXXFILENO_DIR)/include" \
	LIBCXXFILENO_LIBS="-L$(LIBCXXFILENO_DIR)/lib -Wl,-rpath,$(LIBCXXFILENO_DIR)/lib" \
	LIBSRK31CXX_CXXFLAGS="-I$(LIBSRK31CXX_DIR)/include" \
	LIBSRK31CXX_LIBS="-L$(LIBSRK31CXX_DIR)/lib -Wl,-rpath,$(LIBSRK31CXX_DIR)/lib" \
	LIBDWARFPP_CXXFLAGS=-I$(LIBDWARFPP_DIR)/include \
	LIBDWARFPP_LIBS="-L$(LIBDWARFPP_DIR)/lib \
        -Wl,-rpath,$(LIBDWARFPP_DIR)/lib -ldwarfpp" \
	 ./configure --prefix=$(CONTRIB_ROOT)/usr $(LIBCXXGEN_CONFIGURE_FLAGS)

.PHONY: build-libcxxgen
build-libcxxgen: libcxxgen/config.status
	$(MAKE) -C libcxxgen

clean::
	if test -e libcxxgen/Makefile; then $(MAKE) -C libcxxgen clean; fi

CONFIG_MK += \nLIBCXXGEN_CXXFLAGS += -I$(CONTRIB_ROOT)/libcxxgen/include\n
CONFIG_MK += \nLIBCXXGEN_LIBS += -L$(CONTRIB_ROOT)/libcxxgen/lib -Wl,-rpath,$(CONTRIB_ROOT)/libcxxgen/lib -lcxxgen\n

# ----------------------------libantlr3c++

libantlr3c++/config.status:
	cd libantlr3c++ && ./autogen.sh && \
	 ./configure --prefix=$(CONTRIB_ROOT)/usr

.PHONY: build-libantlr3c++
build-libantlr3c++: libantlr3c++/config.status
	$(MAKE) -C libantlr3c++

clean::
	if test -e libantlr3c++/Makefile; then $(MAKE) -C libantlr3c++ clean; fi

CONFIG_MK += \nLIBANTLR3CXX_CXXFLAGS += -I$(CONTRIB_ROOT)/libantlr3c++/include\n
CONFIG_MK += \nLIBANTLR3CXX_LIBS += -L$(CONTRIB_ROOT)/libantlr3c++/lib -Wl,-rpath,$(CONTRIB_ROOT)/libantlr3c++/lib\n

# ----------------------------config.mk

varlist := CXXFLAGS CPPFLAGS CFLAGS LDFLAGS \
LIBANTLR3CXX_CXXFLAGS LIBANTLR3CXX_LIBS \
LIBCXXGEN_CXXFLAGS LIBCXXGEN_LIBS \
LIBDWARFPP_CXXFLAGS LIBDWARFPP_LIBS \
LIBCXXFILENO_CXXFLAGS LIBCXXFILENO_LIBS \
LIBSRK31CXX_CXXFLAGS LIBSRK31CXX_LIBS

CONFIG_MK += \n\nenv.sh:\n\tprintf '"'"'$(foreach var,$(varlist),export $(var)="$$($(var))"; )'"'"' >"$$@" || (rm -f "$$@"; false)

config.mk:
	printf '$(CONFIG_MK)' > "$@" || (rm -f "$@"; false)

env.sh: config.mk build-libdwarfpp # because our config.mk includes libdwarfpp's
	$(MAKE) -f "$<" "$@"

clean::
	rm -f config.mk env.sh
