# Define scripting
SHELL:=/bin/bash

# General directories definitions
PROJECT_DIR = $(shell readlink -f $(shell pwd)/$(dir $(lastword $(MAKEFILE_LIST))))
BUILD_DIR = $(PROJECT_DIR)/build

# Variables for Installation Directories defaults (following the GNU Makefile conventions)
PROGRAM_NAME = mp
PREFIX = $(PROJECT_DIR)/_install_dir
EXEC_PREFIX = $(PREFIX)
BINDIR = $(EXEC_PREFIX)/bin
LOCALSTATEDIR = $(PREFIX)/var
RUNSTATEDIR = $(LOCALSTATEDIR)/run
INCLUDEDIR = $(PREFIX)/include
LIBDIR = $(EXEC_PREFIX)/lib
DATAROOTDIR = $(PREFIX)/share
HTMLDIR = $(DATAROOTDIR)/$(PROGRAM_NAME)/html

# Exports
export PATH := $(BINDIR):$(PATH)

# Common compiler options
CPPFLAGS = -Wall -O3 -fPIC -I$(INCLUDEDIR) -DPROJECT_DIR=\"$(PROJECT_DIR)\" -DPREFIX=\"$(PREFIX)\"
CPP = c++
LDLIBS = -lm -ldl

.PHONY: all clean .foldertree .conf_file .html $(PROGRAM_NAME) libutils openssl json-c nasm ffmpeg

all: $(PROGRAM_NAME)

clean:
	@rm -rf $(BUILD_DIR)
	@rm -rf $(PREFIX)

#Note: Use dot as a "trick" to hide this target from shell when user applies "autocomplete"
.foldertree:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(LIBDIR)
	@mkdir -p $(INCLUDEDIR)
	@mkdir -p $(BINDIR)
	@mkdir -p $(PREFIX)/etc
	@mkdir -p $(PREFIX)/certs

.conf_file: | .foldertree
	@cp -a $(PROJECT_DIR)/certs/* $(PREFIX)/certs/ || true
	@sed 's~<PREFIX>~${PREFIX}~g' $(PROJECT_DIR)/conf/mp.conf.template > $(PREFIX)/etc/mp.conf |true

.html: | .foldertree
	@cp -a $(PROJECT_DIR)/www/html $(PREFIX)/ || true

##############################################################################
# Rule for '$(PROGRAM_NAME)' application
##############################################################################

$(PROGRAM_NAME): .conf_file .html json-c openssl libutils ffmpeg
	@$(MAKE) $(PROGRAM_NAME)-generic-build-install --no-print-directory \
SRCDIRS=$(PROJECT_DIR)/src _BUILD_DIR=$(BUILD_DIR)/$@ TARGETFILE=$(BUILD_DIR)/$@/$@.bin \
CXXFLAGS='$(CPPFLAGS) -std=c++11' CFLAGS='$(CPPFLAGS)' \
LDFLAGS+='-L$(LIBDIR)' LDLIBS+='-lpthread -lutils -lssl -lcrypto -ljson-c'

##############################################################################
# Rule for 'utils' library
##############################################################################

LIBUTILS_CPPFLAGS = $(CPPFLAGS) -Bdynamic -shared -I$(PROJECT_DIR)/libs
LIBUTILS_CFLAGS = $(LIBUTILS_CPPFLAGS)
LIBUTILS_CXXFLAGS = $(LIBUTILS_CPPFLAGS) -std=c++11
LIBUTILS_SRCDIRS = $(PROJECT_DIR)/src/libs/utils
LIBUTILS_HDRFILES = $(wildcard $(PROJECT_DIR)/src/libs/utils/*.h)

libutils: | .foldertree
	@$(MAKE) libutils-generic-build-install --no-print-directory \
SRCDIRS='$(LIBUTILS_SRCDIRS)' _BUILD_DIR='$(BUILD_DIR)/$@' TARGETFILE='$(BUILD_DIR)/$@/$@.so' \
INCLUDEFILES='$(LIBUTILS_HDRFILES)' CFLAGS='$(LIBUTILS_CFLAGS)' CXXFLAGS='$(LIBUTILS_CXXFLAGS)' || exit 1

##############################################################################
# Rule for 'OpenSSL' library and apps.
##############################################################################

OPENSSL_SRCDIRS = $(PROJECT_DIR)/3rdplibs/openssl
.ONESHELL: # Note: ".ONESHELL" only applies to 'openssl' target
openssl: | .foldertree
	@$(eval _BUILD_DIR := $(BUILD_DIR)/$@)
	@mkdir -p "$(_BUILD_DIR)"
	@if [ ! -f "$(_BUILD_DIR)"/Makefile ] ; then \
		echo "Configuring $@..."; \
		cd "$(_BUILD_DIR)" && "$(OPENSSL_SRCDIRS)"/config --prefix="$(PREFIX)" --openssldir="$(PREFIX)" || exit 1; \
	fi
	@$(MAKE) -C "$(_BUILD_DIR)" -j1 install_sw || exit 1

##############################################################################
# Rule for 'json-c' library
##############################################################################

JSONC_SRCDIRS = $(PROJECT_DIR)/3rdplibs/json-c
.ONESHELL:
json-c: | .foldertree
	@$(eval _BUILD_DIR := $(BUILD_DIR)/$@)
	@mkdir -p "$(_BUILD_DIR)"
	@if [ ! -f "$(_BUILD_DIR)"/Makefile ] ; then \
		echo "Auto configuring $@..."; \
		cd "$(JSONC_SRCDIRS)" && ./autogen.sh || exit 1; \
		echo "Configuring $@..."; \
		cd "$(_BUILD_DIR)" && "$(JSONC_SRCDIRS)"/configure --prefix="$(PREFIX)" --srcdir="$(JSONC_SRCDIRS)" \
--enable-static=no || exit 1; \
	fi
	@$(MAKE) -C "$(_BUILD_DIR)" install || exit 1

###############################################################################
## Rule for 'nasm' library
###############################################################################

NASM_SRCDIRS = $(PROJECT_DIR)/3rdplibs/nasm
.ONESHELL:
nasm: | .foldertree
	@$(eval _BUILD_DIR := $(BUILD_DIR)/$@)
	@mkdir -p "$(_BUILD_DIR)"
	@if [ ! -f "$(_BUILD_DIR)"/Makefile ] ; then \
		echo "Configuring $@..."; \
		cp -a "$(NASM_SRCDIRS)"/* "$(_BUILD_DIR)"
		cd "$(_BUILD_DIR)" && ./autogen.sh && ./configure --prefix="$(PREFIX)" || exit 1; \
	fi
	@$(MAKE) -C "$(_BUILD_DIR)" install || exit 1

##############################################################################
# Rule for 'ffmpeg' library
##############################################################################

FFMPEG_SRCDIRS = $(PROJECT_DIR)/3rdplibs/ffmpeg
.ONESHELL:
ffmpeg: nasm | .foldertree
	@$(eval _BUILD_DIR := $(BUILD_DIR)/$@)
	@mkdir -p "$(_BUILD_DIR)"
	@if [ ! -f "$(_BUILD_DIR)"/Makefile ] ; then \
		echo "Configuring $@..."; \
		cd "$(_BUILD_DIR)" && "$(FFMPEG_SRCDIRS)"/configure --prefix="$(PREFIX)" --disable-static --enable-shared \
--enable-gpl --disable-all --extra-cflags="-I${PREFIX}/include -fopenmp" --extra-ldflags="-L${PREFIX}/lib -fopenmp" \
|| exit 1; \
	fi
	@$(MAKE) -C "$(_BUILD_DIR)" install || exit 1

##############################################################################
# Rule for 'check' library
##############################################################################

CHECK_SRCDIRS = $(PROJECT_DIR)/3rdplibs/check
.ONESHELL:
check: | .foldertree
	@$(eval _BUILD_DIR := $(BUILD_DIR)/$@)
	@mkdir -p "$(_BUILD_DIR)"
	@if [ ! -f "$(_BUILD_DIR)"/Makefile ] ; then \
		echo "Configuring $@..."; \
		cd "$(CHECK_SRCDIRS)" && autoreconf --install || exit 1; \
		cd "$(_BUILD_DIR)" && "$(CHECK_SRCDIRS)"/configure --prefix="$(PREFIX)" --enable-static=no \
--enable-build-docs=no || exit 1; \
	fi
	@$(MAKE) -C "$(_BUILD_DIR)" install || exit 1

##############################################################################
# Generic rules to compile and install any library or app. from source
############################################################################## 
# Implementation note: 
# -> ’%’ below is used to write target with wildcards (e.g. ‘%-build’ matches all called targets with this pattern);
# -> ‘$*’ takes exactly the value of the substring matched by ‘%’ in the correspondent target itself. 
# For example, if the main Makefile performs 'make mylibname-build', ’%’ will match 'mylibname' and ‘$*’ will exactly 
# take the value 'mylibname'.

%-generic-build-install: %-generic-build-source-compile
	@if [ -f $(_BUILD_DIR)/*.so ] ; then\
		cp -f $(_BUILD_DIR)/*.so $(LIBDIR)/;\
	fi
	@if [ ! -z "$(INCLUDEFILES)" ] ; then\
		mkdir -p $(INCLUDEDIR)/$*;\
		cp -f $(INCLUDEFILES) $(INCLUDEDIR)/$*/;\
	fi
	@if [ -f $(_BUILD_DIR)/*.bin ] ; then\
		cp -f $(_BUILD_DIR)/*.bin $(BINDIR)/$*;\
	fi

find_cfiles = $(wildcard $(shell realpath --relative-to=$(PROJECT_DIR) $(dir)/*.c))
find_cppfiles = $(wildcard $(shell realpath --relative-to=$(PROJECT_DIR) $(dir)/*.cpp))
get_cfiles_objs = $(patsubst %.c,$(_BUILD_DIR)/%.o,$(find_cfiles))
get_cppfiles_objs = $(patsubst %.cpp,$(_BUILD_DIR)/%.oo,$(find_cppfiles))

%-generic-build-source-compile:
	@mkdir -p "$(_BUILD_DIR)"
	@echo "Building target '$*' from sources... (make $@)"
	@echo "Target file $(TARGETFILE)";
	@echo "Source directories: $(SRCDIRS)";
	$(eval cobjs := $(foreach dir,$(SRCDIRS),$(get_cfiles_objs)))
	$(eval cppobjs := $(foreach dir,$(SRCDIRS),$(get_cppfiles_objs)))
	$(MAKE) $(TARGETFILE) objs="$(cobjs) $(cppobjs)" --no-print-directory;
	@echo Finished building target '$*'

$(_BUILD_DIR)/%.so $(_BUILD_DIR)/%.bin: $(objs)
	$(CPP) -o $@ $^ $(CXXFLAGS) $(LDFLAGS) $(LDLIBS)

.PRECIOUS: $(_BUILD_DIR)%/.
$(_BUILD_DIR)%/.:
	mkdir -p $@

.SECONDEXPANSION:

# Note: The "$(@D)" expands to the directory part of the target path. 
# We escape the $(@D) reference so that we expand it only during the second expansion.
$(_BUILD_DIR)/%.o: $(PROJECT_DIR)/%.c | $$(@D)/.
	$(CPP) -c -o $@ $< $(CFLAGS) $(LDFLAGS) $(LDLIBS)

$(_BUILD_DIR)/%.oo: $(PROJECT_DIR)/%.cpp | $$(@D)/.
	$(CPP) -c -o $@ $< $(CXXFLAGS) $(LDFLAGS) $(LDLIBS)
