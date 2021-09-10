THIS_MAKEFILE := $(realpath $(lastword $(MAKEFILE_LIST)))
CONTRIB := $(realpath $(dir $(THIS_MAKEFILE)))

default: $(CONTRIB)/env.sh config.mk build-libcxxgen \
build-libantlr3c++ build-libantlr3c build-libdwarfpp antlr3.jar

.PHONY: clean default

# force build of config.mk (this file describes how), and merge
# its vars into our own (avoiding repetition among defs/meta-defs)
include config.mk

MVN ?= $(shell which mvn)
ifeq ($(MVN),)
# We're still using the tarball-download method, for now
#ifeq ($(realpath antlr3.jar),)
#$(info Can't build without antlr3.jar or mvn (to build it))
#$(error Either install maven or drop an antlr3.jar v3.4 (e.g. copy from antlr-3.4-complete.jar) into $(CONTRIB))
#endif
endif

varlist :=
subconfigs :=

# ----------------------------antlr things
# HERE BE DRAGONS. Building antlr3 is tricky because its
# build system expects to be able to request java 1.5,
# and modern JDKs will refuse this. We also can't say
# "use your system-provided antlr3" because Debian only
# has antlr3.2, and we need antlr3.4 exactly. It's bad
# taste to wget a blob of code... so for now we try to
# to the build and hope the host system's JDK will
# grok it. In practice, wgetting the JAR is the
# easier option.
# MORE DRAGONS: note that we are *very* sensitive
# to the version of antlr3. version 3.5 outputs lots of
# warnings during parser generation and then yields a
# broken (non-compilable) parser.
# FIXME: I can't find a git revision of antlr3 which
# (1) builds and (2) generates working parsers.
# If I use the "release-3.4" tag, it does not build
#   (I haven't debugged this -- Maven foo)
# If I use the "3.5" tag, or a later one, it builds
# but can't make a working parser.
# FOR NOW, just use the tarball.
#antlr3.jar: | antlr3
#	cd antlr3 && $(MVN) -Dmaven.test.skip=true
#	ln -sf -T antlr3/antlr-complete/target/antlr-complete-3.*.[0-9].jar antlr3.jar
#libantlr3c/Makefile.in: antlr3/runtime/C/configure.ac
#	ln -sf -T antlr3/runtime/C libantlr3c && \
#	cd libantlr3c && aclocal && \
#                autoconf && \
#                autoheader && \
#                libtoolize && \
#                automake --add-missing && \
#                automake
#	ANTLR3_JAR ?= $(CONTRIB)/antlr3.jar
#chmod +x "$@"

# BUT ARGH. The 3.4 runtime still has the getParent bug.
# So reinstate the patch fix.
libantlr3c-3.4/configure.ac: antlr-3.4.tar.gz
	mkdir -p libantlr3c-3.4 && cd libantlr3c-3.4 && tar --strip-components=3 --keep-newer-files -xvzf "$(CONTRIB)/$<" antlr-3.4/runtime/C
libantlr3c/configure.ac: libantlr3c-3.4/configure.ac
	ln -sf -T "$$(dirname $<)" "$$(dirname $@)"
libantlr3c/Makefile.in: libantlr3c/configure.ac
	# HACK: if the patch fails, it's probably because we've already applied it in an earlier failed run of these rules
	cd libantlr3c && patch -p1 < "$(CONTRIB)"/libantlr3c-3.4-getparent-allow-null.patch </dev/null || true
	cd libantlr3c && aclocal && \
                autoconf && \
                autoheader && \
                libtoolize && \
                automake --add-missing && \
                automake
	chmod +x "$@"
antlr-3.4.tar.gz:
	wget http://www.antlr3.org/download/antlr-3.4.tar.gz
antlr-3.4-complete.jar: antlr-3.4.tar.gz
	tar --strip-components=2 --keep-newer-files -xvzf "$(CONTRIB)/$<" antlr-3.4/lib/antlr-3.4-complete.jar
antlr3.jar: antlr-3.4-complete.jar
	ln -sf -T antlr-3.4-complete.jar $@
ANTLR3_JAR ?= $(CONTRIB)/antlr3.jar

libantlr3c/config.status: libantlr3c/Makefile.in
	cd libantlr3c && ./configure --enable-64bit

.PHONY: build-libantlr3c
build-libantlr3c: libantlr3c/config.status
	$(MAKE) -C libantlr3c

libantlr3c/antlr3config.h: build-libantlr3c

clean::
	rm -f antlr-3.4.tar.gz
	rm -f libantlr3c

CONFIG_MK += \nANTLR3_JAR := $(ANTLR3_JAR)\n
CONFIG_MK += \nANTLR3C_DIR := $(CONTRIB)/libantlr3c\n
CONFIG_MK += \nCPPFLAGS += -I$$(ANTLR3C_DIR)/include -I$$(ANTLR3C_DIR)/\n
CONFIG_MK += \nLDFLAGS += -L$$(ANTLR3C_DIR)/.libs -Wl,-rpath,$$(ANTLR3C_DIR)/.libs\n
# Helpful hint, pasted from configure.ac, about why libantlr3c is different:
# For libantlr3c, there's no pkg-config, so we just use CPPFLAGS and LDFLAGS
# to pass the configuration. So we need to make sure that those variables,
# active at configure time, are passed through.
varlist += ANTLR3_JAR ANTLR3C_DIR CPPFLAGS LDFLAGS

# ----------------------------m4ntlr
M4NTLR_PATH := $(CONTRIB)/m4ntlr
varlist += M4NTLR_PATH

# ----------------------------libdwarfpp

LIBDWARFPP_DIR ?= $(CONTRIB)/libdwarfpp
LIBCXXFILENO_DIR ?= $(LIBDWARFPP_DIR)/contrib/libc++fileno
LIBSRK31CXX_DIR ?= $(LIBDWARFPP_DIR)/contrib/libsrk31c++

libdwarfpp/config.status:
	cd libdwarfpp && ./autogen.sh && git submodule init && git submodule update && \
	$(MAKE) -C contrib && ( . contrib/env.sh && \
	 ./configure --prefix=$(CONTRIB)/usr $(LIBDWARFPP_CONFIGURE_FLAGS) )

$(CONTRIB)/libdwarfpp/contrib/config.mk libdwarfpp/contrib/env.sh:
	$(MAKE) -C libdwarfpp/contrib config.mk env.sh

.PHONY: build-libdwarfpp
build-libdwarfpp: libdwarfpp/config.status
	$(MAKE) -C libdwarfpp

clean::
	if test -f libdwarfpp/Makefile; then $(MAKE) -C libdwarfpp clean; fi

CONFIG_MK += \nLIBDWARFPP_CXXFLAGS += -I$(LIBDWARFPP_DIR)/include\n
CONFIG_MK += \nLIBDWARFPP_LIBS += -L$(LIBDWARFPP_DIR)/lib -Wl,-rpath,$(LIBDWARFPP_DIR)/lib -ldwarfpp\n
varlist += LIBDWARFPP_CXXFLAGS LIBDWARFPP_LIBS
subconfigs += libdwarfpp/contrib/config.mk

# ----------------------------libcxxgen

libcxxgen/Makefile.in:
	cd libcxxgen &&  aclocal && \
                autoconf && \
                libtoolize && \
                automake --add-missing && \
                automake

libcxxgen/config.status: libcxxgen/Makefile.in
	cd libcxxgen && \
	LIBCXXFILENO_CXXFLAGS="-I$(LIBCXXFILENO_DIR)/include" \
	LIBCXXFILENO_LIBS="-L$(LIBCXXFILENO_DIR)/lib -Wl,-rpath,$(LIBCXXFILENO_DIR)/lib" \
	LIBSRK31CXX_CXXFLAGS="-I$(LIBSRK31CXX_DIR)/include" \
	LIBSRK31CXX_LIBS="-L$(LIBSRK31CXX_DIR)/lib -Wl,-rpath,$(LIBSRK31CXX_DIR)/lib" \
	LIBDWARFPP_CXXFLAGS=-I$(LIBDWARFPP_DIR)/include \
	LIBDWARFPP_LIBS="-L$(LIBDWARFPP_DIR)/lib \
        -Wl,-rpath,$(LIBDWARFPP_DIR)/lib -ldwarfpp" \
	 ./configure --prefix=$(CONTRIB)/usr $(LIBCXXGEN_CONFIGURE_FLAGS)

.PHONY: build-libcxxgen
build-libcxxgen: libcxxgen/config.status
	$(MAKE) -C libcxxgen

clean::
	if test -e libcxxgen/Makefile; then $(MAKE) -C libcxxgen clean; fi

CONFIG_MK += \nLIBCXXGEN_CXXFLAGS += -I$(CONTRIB)/libcxxgen/include\n
CONFIG_MK += \nLIBCXXGEN_LIBS += -L$(CONTRIB)/libcxxgen/lib -Wl,-rpath,$(CONTRIB)/libcxxgen/lib -lcxxgen\n
varlist += LIBCXXGEN_CXXFLAGS LIBCXXGEN_LIBS

# ----------------------------libantlr3c++

libantlr3c++/Makefile.in:
	cd libantlr3c++ && ./autogen.sh

libantlr3c++/config.status: libantlr3c++/Makefile.in
	cd libantlr3c++ && ./configure --prefix=$(CONTRIB)/usr

.PHONY: build-libantlr3c++
build-libantlr3c++: libantlr3c++/config.status
	$(MAKE) -C libantlr3c++

clean::
	if test -e libantlr3c++/Makefile; then $(MAKE) -C libantlr3c++ clean; fi

CONFIG_MK += \nLIBANTLR3CXX_CXXFLAGS += -I$(CONTRIB)/libantlr3c++/include\n
CONFIG_MK += \nLIBANTLR3CXX_LIBS += -L$(CONTRIB)/libantlr3c++/lib -Wl,-rpath,$(CONTRIB)/libantlr3c++/lib\n
varlist += LIBANTLR3CXX_CXXFLAGS LIBANTLR3CXX_LIBS

# ----------------------------config.mk
# If the user nukes a sub-config.mk that our config.mk depends on,
# we'd like to be able to rebuild it. So we include a generic rule
# for rebuilding config.mks.
config.mk: $(subconfigs)
	(printf '%%/config.mk:\n\t$$(MAKE) -C $$* config.mk\n\n'; \
	printf "$(foreach cfg,$(subconfigs),\ninclude $(CONTRIB)/$(cfg))"; \
	printf '$(CONFIG_MK)'; echo) > "$@" || (rm -f "$@"; false)
# env.sh exports some vars
env.sh $(CONTRIB)/env.sh: $(foreach cfg,$(subconfigs),$(dir $(CONTRIB)/$(cfg))env.sh)
	(printf "$(foreach cfg,$(subconfigs),\n. $$(dirname $(CONTRIB)/$(cfg))/env.sh)"; \
	printf '$(foreach var,$(varlist),\nexport $(var)="$($(var))")'; echo )>"$@" || (rm -f "$@"; false)
# Similarly when we make an env.sh, have Make ensure existence of
# its depended-on env.sh too, so that we don't get 'source'-time
# failures of sub-env.sh.
$(foreach cfg,$(subconfigs),$(dir $(CONTRIB)/$(cfg))env.sh): %/env.sh:
	$(MAKE) -C $* env.sh
clean::
	rm -f config.mk env.sh
