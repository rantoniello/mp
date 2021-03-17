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

# Non conventional:
# SRCDIRS: A list of SRCDIR's (in the same target we compile sources in 
#          several locations)

# Exports
export PATH := $(BINDIR):$(PATH)

# Common compiler options
CPPFLAGS = -Wall -O3 -fPIC -I$(INCLUDEDIR) -DPROJECT_DIR=\"$(PROJECT_DIR)\" -DPREFIX=\"$(PREFIX)\"
CPP = c++
LDLIBS = -lm -ldl

.PHONY: all clean .foldertree .conf_file .html $(PROGRAM_NAME) libmputils openssl json-c nasm ffmpeg

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

$(PROGRAM_NAME): .conf_file .html json-c openssl ffmpeg libmputils
	@$(MAKE) $(PROGRAM_NAME)-generic-build-install --no-print-directory \
SRCDIRS=$(PROJECT_DIR)/src _BUILD_DIR=$(BUILD_DIR)/$@ TARGETFILE=$(BUILD_DIR)/$@/$@.bin \
CXXFLAGS='$(CPPFLAGS) -std=c++11' CFLAGS='$(CPPFLAGS)' \
LDFLAGS+='-L$(LIBDIR)' LDLIBS+='-lpthread -lssl -lcrypto -ljson-c -lmputils' || exit 1

##############################################################################
# Rule for 'utils' library
##############################################################################

LIBUTILS_CPPFLAGS = $(CPPFLAGS) -Bdynamic -shared
LIBUTILS_CFLAGS = $(LIBUTILS_CPPFLAGS)
LIBUTILS_CXXFLAGS = $(LIBUTILS_CPPFLAGS) -std=c++11
LIBUTILS_SRCDIRS = $(PROJECT_DIR)/src/libs/mputils
LIBUTILS_HDRFILES = $(wildcard $(PROJECT_DIR)/src/libs/mputils/*.h)

libmputils: | .foldertree
	@$(MAKE) mputils-generic-build-install --no-print-directory \
SRCDIRS='$(LIBUTILS_SRCDIRS)' \
_BUILD_DIR='$(BUILD_DIR)/$@' \
TARGETFILE='$(BUILD_DIR)/$@/$@.so' \
DESTFILE='$(LIBDIR)/$@.so' \
INCLUDEFILES='$(LIBUTILS_HDRFILES)' \
CFLAGS='$(LIBUTILS_CFLAGS)' \
CXXFLAGS='$(LIBUTILS_CXXFLAGS)' || exit 1

libmputils_tests: libmputils | .foldertree
	@$(MAKE) mputils_tests-generic-build-install --no-print-directory \
SRCDIRS='$(LIBUTILS_SRCDIRS)/tests' \
_BUILD_DIR='$(BUILD_DIR)/$@' \
TARGETFILE='$(BUILD_DIR)/$@/$@.bin' \
DESTFILE='$(BINDIR)/$@' \
CFLAGS='$(CPPFLAGS)' \
CXXFLAGS='$(CPPFLAGS) -std=c++11' \
LDFLAGS+='-L$(LIBDIR)' \
LDLIBS+='-lpthread -lssl -lcrypto -ljson-c -lmputils -lcheck -lrt' || exit 1

#libmputils_tests_apps: libmputils | .foldertree #FIXME!!
	$(eval _BUILD_DIR := $(BUILD_DIR)/$@) # FIX: necessary for 'cobjs' just below
	$(eval cobjs := $(foreach dir,$(LIBUTILS_SRCDIRS)/tests/apps,$(get_cfiles_objs)))
	@for obj in $(cobjs); do \
echo "$$obj"; \
TARGETNAME=$$(basename -s .o "$$obj"); \
echo "XXXXXXXXXXXXXXXXXX $$TARGETNAME"; \
$(MAKE) mputils_tests_$$TARGETNAME-generic-build-install --no-print-directory \
cobjs=$$obj \
_BUILD_DIR='$(BUILD_DIR)/$@' \
TARGETFILE=$(BUILD_DIR)/$@/$@_$$(basename -s .o $$obj).bin \
DESTFILE=$(BINDIR)/$@_$$(basename -s .o $$obj) \
CXXFLAGS='$(CPPFLAGS) -std=c++11' CFLAGS='$(CPPFLAGS)' \
LDFLAGS+='-L$(LIBDIR)' LDLIBS+='-lpthread -lssl -lcrypto -ljson-c -lmputils -lcheck -lrt'; \
done

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
	@echo Installing target "$(TARGETFILE)" to "$(DESTFILE)";
	@echo ====================================2
	@if [ ! -z "$(INCLUDEFILES)" ] ; then\
		mkdir -p $(INCLUDEDIR)/$*;\
		cp -f $(INCLUDEFILES) $(INCLUDEDIR)/$*/;\
	fi
	@echo ====================================3
	cp -f $(TARGETFILE) $(DESTFILE) || exit 1
	@echo ====================================4

find_cfiles = $(wildcard $(shell realpath --relative-to=$(PROJECT_DIR) $(dir)/*.c))
find_cppfiles = $(wildcard $(shell realpath --relative-to=$(PROJECT_DIR) $(dir)/*.cpp))
get_cfiles_objs = $(patsubst %.c,$(_BUILD_DIR)/%.o,$(find_cfiles))
get_cppfiles_objs = $(patsubst %.cpp,$(_BUILD_DIR)/%.oo,$(find_cppfiles))

%-generic-build-source-compile:
	@echo "Building target '$*' from sources... (make $@)"
	@echo "Target file $(TARGETFILE)";
	@echo "Source directories: $(SRCDIRS)";
	@mkdir -p "$(_BUILD_DIR)"
	$(eval cobjs := $(foreach dir,$(SRCDIRS),$(get_cfiles_objs)))
	$(eval cppobjs := $(foreach dir,$(SRCDIRS),$(get_cppfiles_objs)))
	$(MAKE) $(TARGETFILE) objs="$(cobjs) $(cppobjs)" --no-print-directory;
	@echo Finished building target '$*'

$(TARGETFILE): $(objs)
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
