# Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
# 
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
# 
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0
# 
# SPDX-License-Identifier: EPL-2.0
#

# The default goal (target) when none is specified
.DEFAULT_GOAL=all

#
# Common make variables and rules for C/C++
#
# Component: server
# SubComponent: server_build
#
#   Created on:
#       Author:
#

# Default relative path from this project directory to the root of the directory structure
# note: there is no space between the '=' and '..', intentionally!
ifeq ($(ROOTREL),)
ROOTREL =..
endif

# output directory based on project directory and environment and optionally required output type information
ifeq ($(OUTPUT_TYPE),)
OUTROOTREL =$(ROOTREL)
OUTDIR_ADDITION =
else
ifneq ($(findstring ASAN,$(OUTPUT_TYPE)),)
ASAN_BUILD=y
else
ifneq ($(findstring UBSAN,$(OUTPUT_TYPE)),)
UBSAN_BUILD=y
endif
endif

OUTROOTREL =$(ROOTREL)/..
OUTDIR_ADDITION =/$(OUTPUT_TYPE)
endif

OUTDIR =$(if $(and $(BROOT),$(SROOT)),$(subst $(abspath $(SROOT)),$(abspath $(BROOT))$(OUTDIR_ADDITION),$(abspath $(PRJDIR))),$(PRJDIR))
$(info OUTDIR is $(OUTDIR))

# compiler, linker and other utilities
ifneq ($(CLANG_BUILD),)

DYNAMICFLAGS=
ifneq ($(CLANG_HOME),)
CC=$(CLANG_HOME)/bin/clang
CPP=$(CLANG_HOME)/bin/clang++
else
CC=clang
CPP=clang++
endif

else

DYNAMICFLAGS=-rdynamic
ifneq ($(GCC_HOME),)
CC=$(GCC_HOME)/bin/gcc
CPP=$(GCC_HOME)/bin/g++
else
CC=gcc
CPP=g++
endif

endif
LD = $(CC)
AR = ar
MKDIR = mkdir -p
XXD = /usr/bin/xxd

# Beam compilation
ifeq ($(BEAM_BUILD),yes)
ifneq ($(BEAM_HOME),)
BEAM_CC = $(BEAM_HOME)/bin/beam_compile
else
BEAM_CC = /opt/ibm/beam/bin/beam_compile
endif
ifeq ($(LOG_DIR),)
LOG_DIR=$(OUTDIR)/$(ROOTREL)
endif
BEAM_CC += --beam::stats_file=$(LOG_DIR)/$(PROJECT)_beamstats.txt --beam::complaint_file=$(LOG_DIR)/$(PROJECT)_beamreport.txt --beam::prefixcc
BEAM_CC += $(CC)
BEAM_CC += -DMACRO
endif

# Address Sanitizer (available from gcc 4.8 onwards)
ifneq ($(ASAN_BUILD),)
SANITIZER_FLAGS=-fsanitize=address -fno-omit-frame-pointer -DIMA_ASAN
SANITIZER_LDLIB=-lasan
else
# Undefined behaviour Sanitizer (available from gcc 4.9 onwards)
ifneq ($(UBSAN_BUILD),)
SANITIZER_FLAGS=-fsanitize=undefined -DIMA_UBSAN
SANITIZER_LDLIB=-lubsan
endif
endif

ifneq ($(SANITIZER_LDLIB),)
ifneq ($(SANITIZER_GCC_HOME),)
GCC_HOME=$(SANITIZER_GCC_HOME)
CC=$(GCC_HOME)/bin/gcc
CPP=$(GCC_HOME)/bin/g++
endif
endif

ifneq ($(SSL_HOME),)
SSL_INCLUDE=-I$(SSL_HOME)/include
SSL_LIB=-L$(SSL_HOME)/lib
SSL_RUNPATH_FLAGS=-Wl,-rpath,$(SSL_HOME)/lib
else
SSL_INCLUDE=
SSL_LIB=
SSL_RUNPATH_FLAGS=
endif

TESTLDLIBS = $(SSL_LIB) -lssl  -lcrypto -ldl

#Set Paths - for components that need to do path substitutions
#set PATH_VAR_CFLAGS to one of the following vars in the component Makefile e.g. for
#server paths:
#PATH_VAR_CFLAGS=$(IMA_SVR_PATH_DEFINES)
#
# (NB: The := mean that the variable is expanded once, so we don't run the script for each compile
#
IMA_SVR_PATH_DEFINES := $(shell python3 $(PRJDIR)/$(ROOTREL)/server_build/path_parser.py -p IMA_SVR -p IMA_PRODUCT -m=CFLAGS -s=info)
IMA_BRIDGE_PATH_DEFINES := $(shell python3 $(PRJDIR)/$(ROOTREL)/server_build/path_parser.py -p IMA_BRIDGE -p IMA_PRODUCT  -m=CFLAGS -s=info)

IMA_SVR_AND_BRIDGE_PATH_DEFINES := $(IMA_SVR_PATH_DEFINES) $(IMA_BRIDGE_PATH_DEFINES)

RM = rm
CD = cd
CP = cp

# File extension
SO = .so
A  = .a
EXE =
OBJ = .o
MAP = .map
DEP = .u

# common directory locations
SRCDIR = $(PRJDIR)/src
INCDIR = $(PRJDIR)/include
EXPDIR = $(PRJDIR)/export
CFGDIR = $(PRJDIR)/config
TSTDIR = $(PRJDIR)/test
UTILSRCDIR = $(abspath $(PRJDIR)/$(ROOTREL)/server_utils/src)
BLDINCDIR = $(abspath $(PRJDIR)/$(ROOTREL)/server_build/include)

BINDIR = $(OUTDIR)/bin
LIBDIR = $(OUTDIR)/lib
LIBDIR_S = $(OUTDIR)/lib_s
LIBDIR_64 = $(OUTDIR)/lib64
TSODIR = $(OUTDIR)/test
DOXYDIR = $(OUTDIR)/doxygen
OUTSRCDIR = $(OUTDIR)/src

DEBUG_BINDIR = $(OUTDIR)/debug/bin
DEBUG_LIBDIR = $(OUTDIR)/debug/lib
DEBUG_LIBDIR_S = $(OUTDIR)/debug/lib_s
DEBUG_LIBDIR_64 = $(OUTDIR)/debug/lib64
DEBUG_TSODIR = $(OUTDIR)/debug/test

COVERAGE_BINDIR = $(OUTDIR)/coverage/bin
COVERAGE_LIBDIR = $(OUTDIR)/coverage/lib
COVERAGE_LIBDIR_S = $(OUTDIR)/coverage/lib_s
COVERAGE_LIBDIR_64 = $(OUTDIR)/coverage/lib64
COVERAGE_TSODIR = $(OUTDIR)/coverage/test

COMMON_DIR = $(abspath $(OUTDIR)/$(OUTROOTREL)$(OUTDIR_ADDITION)/server_ship)
COMMON_INCDIR = $(COMMON_DIR)/include
COMMON_LIBDIR = $(COMMON_DIR)/lib
COMMON_LIBDIR_S = $(COMMON_DIR)/lib_s
COMMON_LIBDIR_64 = $(COMMON_DIR)/lib64
COMMON_BINDIR = $(COMMON_DIR)/bin

COMMON_DEBUG_LIBDIR = $(COMMON_DIR)/debug/lib
COMMON_DEBUG_LIBDIR_S = $(COMMON_DIR)/debug/lib_s
COMMON_DEBUG_LIBDIR_64 = $(COMMON_DIR)/debug/lib64
COMMON_DEBUG_BINDIR = $(COMMON_DIR)/debug/bin

COMMON_COVERAGE_LIBDIR = $(COMMON_DIR)/coverage/lib
COMMON_COVERAGE_LIBDIR_S = $(COMMON_DIR)/coverage/lib_s
COMMON_COVERAGE_LIBDIR_64 = $(COMMON_DIR)/coverage/lib64
COMMON_COVERAGE_BINDIR = $(COMMON_DIR)/coverage/bin

# Calculate the files to be published using specified targets
PUBLISH-LIB-TARGETS = $(subst $(LIBDIR),$(COMMON_LIBDIR),$(LIB-TARGETS))
PUBLISH-LIB_S-TARGETS = $(subst $(LIBDIR_S),$(COMMON_LIBDIR_S),$(LIB_S-TARGETS))
PUBLISH-EXE-TARGETS = $(subst $(BINDIR),$(COMMON_BINDIR),$(EXE-TARGETS))
PUBLISH-EXE_S-TARGETS = $(subst $(BINDIR),$(COMMON_BINDIR),$(EXE_S-TARGETS))
PUBLISH-CFG-TARGETS = $(subst $(BINDIR),$(COMMON_BINDIR),$(CFG-TARGETS))
PUBLISH-INC-TARGETS = $(subst $(INCDIR),$(COMMON_INCDIR),$(wildcard $(INCDIR)/*.h))
PUBLISH-TARGETS = $(PUBLISH-LIB-TARGETS) \
                  $(PUBLISH-LIB_S-TARGETS) \
                  $(PUBLISH-EXE-TARGETS) \
                  $(PUBLISH-EXE_S-TARGETS) \
                  $(PUBLISH-CFG-TARGETS) \
                  $(PUBLISH-INC-TARGETS)

# Calculate the debug files to be published
DEBUG-PUBLISH-LIB-TARGETS = $(subst $(DEBUG_LIBDIR),$(COMMON_DEBUG_LIBDIR),$(DEBUG-LIB-TARGETS))
DEBUG-PUBLISH-LIB_S-TARGETS = $(subst $(DEBUG_LIBDIR_S),$(COMMON_DEBUG_LIBDIR_S),$(DEBUG-LIB_S-TARGETS))
DEBUG-PUBLISH-EXE-TARGETS = $(subst $(DEBUG_BINDIR),$(COMMON_DEBUG_BINDIR),$(DEBUG-EXE-TARGETS))
DEBUG-PUBLISH-EXE_S-TARGETS = $(subst $(DEBUG_BINDIR),$(COMMON_DEBUG_BINDIR),$(DEBUG-EXE_S-TARGETS))
DEBUG-PUBLISH-CFG-TARGETS = $(subst $(DEBUG_BINDIR),$(COMMON_DEBUG_BINDIR),$(DEBUG-CFG-TARGETS))
DEBUG-PUBLISH-TARGETS = $(DEBUG-PUBLISH-LIB-TARGETS) \
                        $(DEBUG-PUBLISH-LIB_S-TARGETS) \
                        $(DEBUG-PUBLISH-EXE-TARGETS) \
                        $(DEBUG-PUBLISH-EXE_S-TARGETS) \
                        $(DEBUG-PUBLISH-CFG-TARGETS) \
                        $(PUBLISH-INC-TARGETS)

# Calculate the coverage files to be published
COVERAGE-PUBLISH-LIB-TARGETS = $(subst $(COVERAGE_LIBDIR),$(COMMON_COVERAGE_LIBDIR),$(COVERAGE-LIB-TARGETS))
COVERAGE-PUBLISH-LIB_S-TARGETS = $(subst $(COVERAGE_LIBDIR_S),$(COMMON_COVERAGE_LIBDIR_S),$(COVERAGE-LIB_S-TARGETS))
COVERAGE-PUBLISH-EXE-TARGETS = $(subst $(COVERAGE_BINDIR),$(COMMON_COVERAGE_BINDIR),$(COVERAGE-EXE-TARGETS))
COVERAGE-PUBLISH-EXE_S-TARGETS = $(subst $(COVERAGE_BINDIR),$(COMMON_COVERAGE_BINDIR),$(COVERAGE-EXE_S-TARGETS))
COVERAGE-PUBLISH-CFG-TARGETS = $(subst $(COVERAGE_BINDIR),$(COMMON_COVERAGE_BINDIR),$(COVERAGE-CFG-TARGETS))
COVERAGE-PUBLISH-TARGETS = $(COVERAGE-PUBLISH-LIB-TARGETS) \
                           $(COVERAGE-PUBLISH-LIB_S-TARGETS) \
                           $(COVERAGE-PUBLISH-EXE-TARGETS) \
                           $(COVERAGE-PUBLISH-EXE_S-TARGETS) \
                           $(COVERAGE-PUBLISH-CFG-TARGETS) \
                           $(PUBLISH-INC-TARGETS)

CLIENT_COMMON_DIR = $(abspath $(OUTDIR)/$(OUTROOTREL)$(OUTDIR_ADDITION)/client_ship)
CLIENT_COMMON_INCDIR = $(CLIENT_COMMON_DIR)/include
CLIENT_COMMON_LIBDIR = $(CLIENT_COMMON_DIR)/lib
CLIENT_COMMON_LIBDIR_64 = $(CLIENT_COMMON_DIR)/lib64
CLIENT_COMMON_BINDIR = $(CLIENT_COMMON_DIR)/bin

CLIENT_COMMON_DEBUG_LIBDIR = $(CLIENT_COMMON_DIR)/debug/lib
CLIENT_COMMON_DEBUG_LIBDIR_64 = $(CLIENT_COMMON_DIR)/debug/lib64
CLIENT_COMMON_DEBUG_BINDIR = $(CLIENT_COMMON_DIR)/debug/bin

CLIENT_COMMON_COVERAGE_LIBDIR = $(CLIENT_COMMON_DIR)/coverage/lib
CLIENT_COMMON_COVERAGE_LIBDIR_S = $(CLIENT_COMMON_DIR)/coverage/lib_s
CLIENT_COMMON_COVERAGE_LIBDIR_64 = $(CLIENT_COMMON_DIR)/coverage/lib64
CLIENT_COMMON_COVERAGE_BINDIR = $(CLIENT_COMMON_DIR)/coverage/bin

# Calculate the client files to be published using specified targets
CLIENT-PUBLISH-LIB-TARGETS = $(subst $(LIBDIR),$(CLIENT_COMMON_LIBDIR),$(LIB-TARGETS))
CLIENT-PUBLISH-EXE-TARGETS = $(subst $(BINDIR),$(CLIENT_COMMON_BINDIR),$(EXE-TARGETS))
CLIENT-PUBLISH-CFG-TARGETS = $(subst $(BINDIR),$(CLIENT_COMMON_BINDIR),$(CFG-TARGETS))
CLIENT-PUBLISH-INC-TARGETS = $(subst $(INCDIR),$(CLIENT_COMMON_INCDIR),$(wildcard $(INCDIR)/*.h))
CLIENT-PUBLISH-TARGETS = $(CLIENT-PUBLISH-LIB-TARGETS) \
                         $(CLIENT-PUBLISH-EXE-TARGETS) \
                         $(CLIENT-PUBLISH-CFG-TARGETS) \
                         $(CLIENT-PUBLISH-INC-TARGETS)

CLIENT-PUBLISH-LIB64-TARGETS = $(subst $(LIBDIR_64),$(CLIENT_COMMON_LIBDIR_64),$(LIB-TARGETS))
CLIENT-PUBLISH-TARGETS-64 = $(CLIENT-PUBLISH-LIB64-TARGETS)

# Calculate the debug client files to be published
DEBUG-CLIENT-PUBLISH-LIB-TARGETS = $(subst $(DEBUG_LIBDIR),$(CLIENT_COMMON_DEBUG_LIBDIR),$(DEBUG-LIB-TARGETS))
DEBUG-CLIENT-PUBLISH-EXE-TARGETS = $(subst $(DEBUG_BINDIR),$(CLIENT_COMMON_DEBUG_BINDIR),$(DEBUG-EXE-TARGETS))
DEBUG-CLIENT-PUBLISH-CFG-TARGETS = $(subst $(DEBUG_BINDIR),$(CLIENT_COMMON_DEBUG_BINDIR),$(DEBUG-CFG-TARGETS))
DEBUG-CLIENT-PUBLISH-TARGETS = $(DEBUG-CLIENT-PUBLISH-LIB-TARGETS) \
                               $(DEBUG-CLIENT-PUBLISH-EXE-TARGETS) \
                               $(DEBUG-CLIENT-PUBLISH-CFG-TARGETS) \
                               $(CLIENT-PUBLISH-INC-TARGETS)

DEBUG-CLIENT-PUBLISH-LIB64-TARGETS = $(subst $(DEBUG_LIBDIR_64),$(CLIENT_COMMON_DEBUG_LIBDIR_64),$(DEBUG-LIB-TARGETS))
DEBUG-CLIENT-PUBLISH-TARGETS-64 = $(DEBUG-CLIENT-PUBLISH-LIB64-TARGETS)

# Calculate the coverage client files to be published
COVERAGE-CLIENT-PUBLISH-LIB-TARGETS = $(subst $(COVERAGE_LIBDIR),$(CLIENT_COMMON_COVERAGE_LIBDIR),$(COVERAGE-LIB-TARGETS))
COVERAGE-CLIENT-PUBLISH-EXE-TARGETS = $(subst $(COVERAGE_BINDIR),$(CLIENT_COMMON_COVERAGE_BINDIR),$(COVERAGE-EXE-TARGETS))
COVERAGE-CLIENT-PUBLISH-CFG-TARGETS = $(subst $(COVERAGE_BINDIR),$(CLIENT_COMMON_COVERAGE_BINDIR),$(COVERAGE-CFG-TARGETS))
COVERAGE-CLIENT-PUBLISH-TARGETS = $(COVERAGE-CLIENT-PUBLISH-LIB-TARGETS) \
                                  $(COVERAGE-CLIENT-PUBLISH-EXE-TARGETS) \
                                  $(COVERAGE-CLIENT-PUBLISH-CFG-TARGETS) \
                                  $(CLIENT-PUBLISH-INC-TARGETS)

COVERAGE-CLIENT-PUBLISH-LIB64-TARGETS = $(subst $(COVERAGE_LIBDIR_64),$(CLIENT_COMMON_COVERAGE_LIBDIR_64),$(COVERAGE-LIB-TARGETS))
COVERAGE-CLIENT-PUBLISH-TARGETS-64 = $(COVERAGE-CLIENT-PUBLISH-LIB64-TARGETS)

# CUnit
# CUNIT_HOME=/opt/CUnit-2.1-2_64
ifneq ($(CUNIT_HOME),)
CUNIT_LIBDIR=$(CUNIT_HOME)/lib
CUNIT_INC=$(CUNIT_HOME)/include
CUNIT_FLAGS=-I$(CUNIT_INC)
CUNIT_LIBS=-L$(CUNIT_LIBDIR)
CUNIT_RUNPATH_FLAGS= -Wl,-rpath,$(CUNIT_LIBDIR)
CUNIT_VERSION=$(shell grep 'define CU_VERSION' $(CUNIT_INC)/CUnit/CUnit.h | sed 's/[^0-9]*//g')
else
ifneq ($(wildcard /usr/include/CUnit/CUnit.h*),)
CUNIT_VERSION=$(shell grep 'define CU_VERSION' /usr/include/CUnit/CUnit.h | sed 's/[^0-9]*//g')
endif
endif

# CUnit has changed structures at version 2.1.3, CUnitVersionFix.h deals with this but it needs to be
# included for all compilations.
ifneq ($(CUNIT_VERSION),)
CUNIT_FLAGS+=-DIMA_CUNIT_VERSION=$(CUNIT_VERSION) -include $(BLDINCDIR)/CUnitVersionFix.h
endif

CUNITSOURCEDIR=$(SRCDIR)/cunit/src
CUNITTESTDIR=$(TSODIR)/cunit
DEBUG_CUNITTESTDIR=$(DEBUG_TSODIR)/cunit
COVERAGE_CUNITTESTDIR=$(COVERAGE_TSODIR)/cunit
CUNIT_LIBS+=-lcunit

# ICU
# To use an ICU not installed specify IMA_ICU_HOME
ifneq ($(IMA_ICU_HOME),)
IMA_ICU_LIBDIR=$(IMA_ICU_HOME)/lib
IMA_ICU_INC=$(IMA_ICU_HOME)/include
IMA_ICU_FLAGS=-I$(IMA_ICU_INC)
IMA_ICU_LIBS=-L$(IMA_ICU_LIBDIR)
IMA_ICU_RUNPATH_FLAGS= -Wl,-rpath,$(IMA_ICU_LIBDIR)
endif

# BOOST
# To use an BOOST not installed specify IMA_BOOST_HOME
ifneq ($(IMA_NO_BOOST),TRUE)
ifneq ($(IMA_BOOST_HOME),)
IMA_BOOST_LIBDIR=$(IMA_BOOST_HOME)/lib
IMA_BOOST_INC=$(IMA_BOOST_HOME)/include
IMA_BOOST_FLAGS=-I$(IMA_BOOST_INC)
IMA_BOOST_LIBS=-L$(IMA_BOOST_LIBDIR)
IMA_BOOST_RUNPATH_FLAGS= -Wl,-rpath,$(IMA_BOOST_LIBDIR)
endif
endif

# OpenLDAP
# To use an OpenLDAP not installed with the system specify OPENLDAP_HOME
ifneq ($(OPENLDAP_HOME),)
OPENLDAP_INC=-I$(OPENLDAP_HOME)/include
OPENLDAP_LIBS=-L$(OPENLDAP_HOME)/lib
OPENLDAP_RUNPATH_FLAGS= -Wl,-rpath,$(OPENLDAP_HOME)/lib
endif

# Curl
# To use a Curl not installed with the system specify CURL_HOME
ifneq ($(CURL_HOME),)
CURL_INC=-I$(CURL_HOME)/include
CURL_LIBS=-L$(CURL_HOME)/lib
CURL_RUNPATH_FLAGS= -Wl,-rpath,$(CURL_HOME)/lib
endif

# NetSNMP
# To use a NetSNMP not installed with the system specify NETSNMP_HOME
ifneq ($(NETSNMP_HOME),)
NETSNMP_INC=-I$(NETSNMP_HOME)/include
NETSNMP_LIBS=-L$(NETSNMP_HOME)/lib
NETSNMP_RUNPATH_FLAGS= -Wl,-rpath,$(NETSNMP_HOME)/lib
endif

# common flags
OPTFLAGS = -O3 -g -DNDEBUG $(ASAN_OPTFLAGS)  -D_FORTIFY_SOURCE=1
WARNFLAGS = -Wall -Wold-style-definition -Wshadow $(SANITIZER_FLAGS)
DEBUGFLAGS = -O0 -g -DDEBUG $(SANITIZER_FLAGS)

CPPOPTFLAGS = -O3 -g -DNDEBUG $(ASAN_OPTFLAGS)
CPPWARNFLAGS = -Wall $(SANITIZER_FLAGS)

IDFLAGS = -DISMDATE=`date +%Y-%m-%d` -DISMTIME=`date +%H:%M` -DBUILD_LABEL=$(BUILD_LABEL) -DBUILD_ID=$(BUILD_ID) -DISM_VERSION="$(ISM_VERSION)"
IFLAGS = -I. -I$(OUTSRCDIR) -I$(SRCDIR) $(SSL_INCLUDE) $(OPENLDAP_INC) $(CURL_INC) $(NETSNMP_INC) -I$(INCDIR) -I$(COMMON_INCDIR) -I$(TSTDIR) $(IMA_ICU_FLAGS) $(IMA_BOOST_FLAGS) $(CUNIT_FLAGS)
CFLAGS = $(WARNFLAGS) $(PERFFLAGS) $(IMA_EXTRA_CFLAGS) -fPIC $(DYNAMICFLAGS) -DCOMMON_MALLOC_WRAPPER -D_GNU_SOURCE -std=gnu99 -fno-omit-frame-pointer -fstack-protector-strong
#DEBUG_CFLAGS = $(WARNFLAGS) $(DEBUGFLAGS) $(PERFFLAGS) -fPIC -rdynamic  -D_GNU_SOURCE
CPPFLAGS = $(CPPWARNFLAGS) $(PERFFLAGS) $(IMA_EXTRA_CPPFLAGS) -fPIC -rdynamic -DCOMMON_MALLOC_WRAPPER -D_GNU_SOURCE -std=gnu++11
LFLAGS = -fPIC -rdynamic
LDFLAGS = -pthread -Wl,--no-as-needed -Wl,--no-warn-search-mismatch $(SSL_LIB) $(OPENLDAP_LIBS) $(CURL_LIBS) $(NETSNMP_LIBS)
#DEBUG_LDFLAGS = -L$(COMMON_DEBUG_LIBDIR)
# LDLIBS = -lrt -lm -ldl -lpthread -lssl -lrum -lfmdutil
LDLIBS = -lrt -lm
XFLAGS = -Xlinker -Map -Xlinker $(BINDIR)/$(notdir $@)$(MAP)
SHARED_FLAGS = -shared -shared-libgcc
SONAME_FLAGS = -Wl,-soname,$(notdir $@)
ifneq ($(GCC_HOME),)
RUNPATH_FLAGS = -Wl,--enable-new-dtags -Wl,-rpath,$(GCC_HOME)/lib64
DEBUG_RUNPATH_FLAGS = -g -Wl,--enable-new-dtags -Wl,-rpath,$(GCC_HOME)/lib64
COVERAGE_RUNPATH_FLAGS =  -Wl,--enable-new-dtags -Wl,-rpath,$(GCC_HOME)/lib64
else
RUNPATH_FLAGS = -Wl,--enable-new-dtags
DEBUG_RUNPATH_FLAGS = -g -Wl,--enable-new-dtags
COVERAGE_RUNPATH_FLAGS =  -Wl,--enable-new-dtags
endif

IMASERVER_RUNPATH_DIR := $(shell python3 $(PRJDIR)/$(ROOTREL)/server_build/path_parser.py -p IMA_RPATH_SVR -m=value)

IMASERVER_RUNPATH_COMPS = server_utils server_admin server_spidercast server_cluster server_store server_transport server_engine \
                          server_monitoring server_protocol server_ismc server_snmp server_mqcbridge
ifeq ($(PROJECT),$(filter $(PROJECT),$(IMASERVER_RUNPATH_COMPS)))
	# For all imaserver components, add MessageSightServer library path (/opt/ibm/imaserver/lib64) to the runpath
	# We also add $ORIGIN so that if a library is dlopened by a unit test, the dependencies of the library are found
	# (The runpath of the unit test itself finds the library, but doesn't seem to affect searching for dependencies of the library)
	IMA_RUNPATH_FLAGS = $(RUNPATH_FLAGS) -Wl,-rpath,'$$ORIGIN' -Wl,-rpath,$(IMASERVER_RUNPATH_DIR)
	IMA_DEBUG_RUNPATH_FLAGS = $(DEBUG_RUNPATH_FLAGS) -Wl,-rpath,'$$ORIGIN' -Wl,-rpath,$(IMASERVER_RUNPATH_DIR)
	IMA_COVERAGE_RUNPATH_FLAGS = $(COVERAGE_RUNPATH_FLAGS) -Wl,-rpath,'$$ORIGIN' -Wl,-rpath,$(IMASERVER_RUNPATH_DIR)
endif

define eyecatcher
    @echo ==== $(1) ==== $(shell date +%T)
endef

define ensure-output-dir
	@$(MKDIR) $(dir $(1))
endef

# Function: Convert a list of sources into fully-qualified object paths
define objects
	$(addsuffix $(OBJ),$(addprefix $(BINDIR)/,$(basename $(notdir $(1)))))
endef

# Function: Convert a list of sources into fully-qualified debug object paths
define debug-objects
	$(addsuffix $(OBJ),$(addprefix $(DEBUG_BINDIR)/,$(basename $(notdir $(1)))))
endef

# Function: Convert a list of sources into fully-qualified coverage object paths
define coverage-objects
	$(addsuffix $(OBJ),$(addprefix $(COVERAGE_BINDIR)/,$(basename $(notdir $(1)))))
endef

# Function: Convert a list of libs into fully-qualified (server) lib paths
define libs
	$(addsuffix $(SO),$(addprefix $(COMMON_LIBDIR)/,$(basename $(notdir $(1)))))
endef

# Function: Convert a list of libs into fully-qualified (server) debug lib paths
define debug-libs
	$(addsuffix $(SO),$(addprefix $(COMMON_DEBUG_LIBDIR)/,$(basename $(notdir $(1)))))
endef

# Function: Convert a list of libs into fully-qualified (server) coverage lib paths
define coverage-libs
	$(addsuffix $(SO),$(addprefix $(COMMON_COVERAGE_LIBDIR)/,$(basename $(notdir $(1)))))
endef

# Function: Convert a list of libs into fully-qualified (server) lib paths
define libs_s
	$(addsuffix $(A),$(addprefix $(COMMON_LIBDIR_S)/,$(basename $(notdir $(1)))))
endef

# Function: Convert a list of libs into fully-qualified (server) debug lib paths
define debug-libs_s
	$(addsuffix $(A),$(addprefix $(COMMON_DEBUG_LIBDIR_S)/,$(basename $(notdir $(1)))))
endef

# Function: Convert a list of libs into fully-qualified (server) coverage lib paths
define coverage-libs_s
	$(addsuffix $(A),$(addprefix $(COMMON_COVERAGE_LIBDIR_S)/,$(basename $(notdir $(1)))))
endef

# Function: Convert a list of libs into fully-qualified (client) lib paths
define client-libs
	$(addsuffix $(SO),$(addprefix $(CLIENT_COMMON_LIBDIR)/,$(basename $(notdir $(1)))))
endef

# Function: Convert a list of libs into fully-qualified (client) debug lib paths
define debug-client-libs
	$(addsuffix $(SO),$(addprefix $(CLIENT_COMMON_DEBUG_LIBDIR)/,$(basename $(notdir $(1)))))
endef

# Invoke the C compiler to produce object and dependencies and BEAM to produce beam report
define invoke-beam-c-compiler
	$(call ensure-output-dir, $(1))
	$(CC) -MM -MT $(1) -o $(subst $(OBJ),$(DEP),$(1)) $(2) $(3)
	$(CC) -c -o $(1) $(2) $(3)
	$(BEAM_CC) -c -o $(1) $(2) $(3)
endef

# Invoke the C++ compiler to produce object and dependencies and BEAM to produce beam report
define invoke-beam-cpp-compiler
	$(call ensure-output-dir, $(1))
	$(CPP) -MM -MT $(1) -o $(subst $(OBJ),$(DEP),$(1)) $(2) $(3)
	$(CPP) -c -o $(1) $(2) $(3)
	$(BEAM_CC) -c -o $(1) $(2) $(3)
endef

# Function: Compile and invoke BEAM on a C source file
define beam-c-src
	$(call eyecatcher, Compile Target:$(notdir $(1)) Project:$(PROJECT) Beam version)
	$(call invoke-beam-c-compiler,$(1),$(2),$(if $(3),$(3),$(CFLAGS)) $(OPTFLAGS) $(IFLAGS) $(IDFLAGS))
endef

# Function: Compile and invoke BEAM on a C++ source file
define beam-cpp-src
	$(call eyecatcher, C++ Compile Target:$(notdir $(1)) Project:$(PROJECT) Beam version)
	$(call invoke-beam-cpp-compiler,$(1),$(2),$(if $(3),$(3),$(CPPFLAGS)) $(OPTFLAGS) $(IFLAGS) $(IDFLAGS))
endef

# Invoke the C compiler to produce object and dependencies
define invoke-c-compiler
	$(call ensure-output-dir, $(1))
	$(CC) -MM -MT $(1) -o $(subst $(OBJ),$(DEP),$(1)) $(2) $(3)
	$(CC) -c -o $(1) $(2) $(3) -DTRACEFILE_IDENT=0x$(shell echo -n $(notdir $(2)) | md5sum | cut -c 1-8)
endef

# Invoke the C++ compiler to produce object and dependencies
define invoke-cpp-compiler
	$(call ensure-output-dir, $(1))
	$(CPP) -MM -MT $(1) -o $(subst $(OBJ),$(DEP),$(1)) $(2) $(3)
	$(CPP) -c -o $(1) $(2) $(3) -DTRACEFILE_IDENT=0x$(shell echo -n $(notdir $(2)) | md5sum | cut -c 1-8)
endef

# Function: Compile a C source file (production)
define compile-c-src
	$(call eyecatcher, C Compile Target:$(notdir $(1)) Project:$(PROJECT) Production version)
	$(call invoke-c-compiler,$(1),$(2),$(if $(3),$(3),$(CFLAGS)) $(PATH_VAR_CFLAGS) $(OPTFLAGS) $(IFLAGS) $(IDFLAGS))
endef

# Function: Compile a C source file (debug)
define debug-compile-c-src
	$(call eyecatcher, C Compile Target:$(notdir $(1)) Project:$(PROJECT) Debug version)
	$(call invoke-c-compiler,$(1),$(2),$(if $(3),$(3),$(CFLAGS)) $(PATH_VAR_CFLAGS) $(DEBUGFLAGS) $(IFLAGS) $(IDFLAGS))
endef

# Function: Compile a C source file (coverage)
define coverage-compile-c-src
	$(call eyecatcher, C Compile Target:$(notdir $(1)) Project:$(PROJECT) Coverage version)
	$(call invoke-c-compiler,$(1),$(2),$(if $(3),$(3),$(CFLAGS)) -g --coverage -O0 -DNDEBUG $(PATH_VAR_CFLAGS) $(IFLAGS) $(IDFLAGS))
endef

# Function: Compile a C++ source file (production)
define compile-cpp-src
	$(call eyecatcher, C++ Compile Target:$(notdir $(1)) Project:$(PROJECT) Production version)
	$(call invoke-cpp-compiler,$(1),$(2),$(if $(3),$(3),$(CPPFLAGS)) $(OPTFLAGS) $(IFLAGS) $(IDFLAGS))
endef

# Function: Compile a C source file (debug)
define debug-compile-cpp-src
	$(call eyecatcher, C++ Compile Target:$(notdir $(1)) Project:$(PROJECT) Debug version)
	$(call invoke-cpp-compiler,$(1),$(2),$(if $(3),$(3),$(CPPFLAGS)) $(DEBUGFLAGS) $(IFLAGS) $(IDFLAGS))
endef

# Function: Compile a C source file (coverage)
define coverage-compile-cpp-src
	$(call eyecatcher, C++ Compile Target:$(notdir $(1)) Project:$(PROJECT) Coverage version)
	$(call invoke-cpp-compiler,$(1),$(2),$(if $(3),$(3),$(CPPFLAGS)) -g --coverage -O0 -DNDEBUG $(IFLAGS) $(IDFLAGS))
endef

# Function: Link objects into a program
define link-c-program
	$(call eyecatcher, Link Program Target:${notdir $@} Project:$(PROJECT))
	$(call ensure-output-dir, $@)
	$(CC) $(LDFLAGS) -o $@ \
	$(IMA_ICU_RUNPATH_FLAGS) $(IMA_BOOST_RUNPATH_FLAGS) $(IMA_RUNPATH_FLAGS) $(RUNPATH_FLAGS) -Wl,-rpath-link,$(COMMON_LIBDIR):$(CLIENT_COMMON_LIBDIR) $(OPTFLAGS) \
	$(filter %$(OBJ),$^) \
	$(SANITIZER_LDLIB) \
	$(addprefix -l,$(subst lib,,$(basename $(notdir $(filter %$(SO),$^ $|))))) \
	-pie -Wl,-z,relro,-z,now \
	-L$(COMMON_LIBDIR) $(IMA_ICU_LIBS) $(IMA_BOOST_LIBS) $(if $(1),$(1),$(LDLIBS)) $($(notdir $@)-LDLIBS)
endef

# Function: Link debug objects into a program
define debug-link-c-program
	$(call eyecatcher, Link Program Target:${notdir $@} Project:$(PROJECT) Debug version)
	$(call ensure-output-dir, $@)
	$(CC) $(LDFLAGS) -o $@ \
	$(IMA_ICU_RUNPATH_FLAGS) $(IMA_BOOST_RUNPATH_FLAGS) $(IMA_DEBUG_RUNPATH_FLAGS) $(DEBUG_RUNPATH_FLAGS) -Wl,-rpath-link,$(COMMON_DEBUG_LIBDIR):$(CLIENT_COMMON_DEBUG_LIBDIR) \
	$(filter %$(OBJ),$^) \
	$(SANITIZER_LDLIB) \
	$(addprefix -l,$(subst lib,,$(basename $(notdir $(filter %$(SO),$^ $|))))) \
	-pie -Wl,-z,relro,-z,now \
	-L$(COMMON_DEBUG_LIBDIR) $(IMA_ICU_LIBS) $(IMA_BOOST_LIBS) $(if $(1),$(1),$(LDLIBS)) $($(notdir $@)-LDLIBS)
endef

# Function: Link coverage objects into a program
define coverage-link-c-program
	$(call eyecatcher, Link Program Target:${notdir $@} Project:$(PROJECT) Coverage version)
	$(call ensure-output-dir, $@)
	$(CC) $(LDFLAGS) -o $@ \
	$(IMA_ICU_RUNPATH_FLAGS) $(IMA_BOOST_RUNPATH_FLAGS) $(IMA_COVERAGE_RUNPATH_FLAGS) $(COVERAGE_RUNPATH_FLAGS) -Wl,-rpath-link,$(COMMON_COVERAGE_LIBDIR):$(CLIENT_COMMON_COVERAGE_LIBDIR) -g --coverage \
	$(filter %$(OBJ),$^) \
	$(SANITIZER_LDLIB) \
	$(addprefix -l,$(subst lib,,$(basename $(notdir $(filter %$(SO),$^ $|))))) \
	-pie -Wl,-z,relro,-z,now \
	-L$(COMMON_COVERAGE_LIBDIR) $(IMA_ICU_LIBS) $(IMA_BOOST_LIBS) $(if $(1),$(1),$(LDLIBS)) $($(notdir $@)-LDLIBS)
endef

# Function: Statically Link objects into a program
define link-c-program-static
	$(call eyecatcher, Statically Link Program Target:${notdir $@} Project:$(PROJECT))
	$(call ensure-output-dir, $@)
	$(CC) $(LDFLAGS) -o $@ \
	$(SANITIZER_LDLIB) \
	-L$(COMMON_LIBDIR_S) $(IMA_ICU_LIBS) $(IMA_BOOST_LIBS) $(filter %$(OBJ),$^) -rdynamic \
	$(addprefix -l,$(subst lib,,$(basename $(notdir $(filter %$(A),$^))))) \
	$(if $(1),$(1),$(LDLIBS))  $($(notdir $@)-LDLIBS)
endef

# Function: Statically Link debug objects into a program
define debug-link-c-program-static
	$(call eyecatcher, Statically Link Program Target:${notdir $@} Project:$(PROJECT) Debug version)
	$(call ensure-output-dir, $@)
	$(CC) $(LDFLAGS) -o $@ \
	$(SANITIZER_LDLIB) \
	-L$(COMMON_DEBUG_LIBDIR_S) $(IMA_ICU_LIBS) $(IMA_BOOST_LIBS) $(filter %$(OBJ),$^) -rdynamic \
	$(addprefix -l,$(subst lib,,$(basename $(notdir $(filter %$(A),$^))))) \
	$(if $(1),$(1),$(LDLIBS)) $($(notdir $@)-LDLIBS)
endef

# Function: Statically Link coverage objects into a program
define coverage-link-c-program-static
	$(call eyecatcher, Statically Link Program Target:${notdir $@} Project:$(PROJECT) Coverage version)
	$(call ensure-output-dir, $@)
	$(CC) $(LDFLAGS) -o $@ -g --coverage \
	$(SANITIZER_LDLIB) \
	-L$(COMMON_COVERAGE_LIBDIR_S) $(IMA_ICU_LIBS) $(IMA_BOOST_LIBS) $(filter %$(OBJ),$^) -rdynamic \
	$(addprefix -l,$(subst lib,,$(basename $(notdir $(filter %$(A),$^))))) \
	$(if $(1),$(1),$(LDLIBS)) $($(notdir $@)-LDLIBS)
endef

# Function: Link objects into a library
define make-c-library
	$(call eyecatcher, Link Library Target:${notdir $@} Project:$(PROJECT))
	$(call show-version)
	$(call ensure-output-dir, $@)
	$(CC) $(LDFLAGS) -o $@ \
	$(OPTFLAGS) $(WARNFLAGS) $(SHARED_FLAGS) $(XFLAGS) $(SONAME_FLAGS) \
	$(IMA_ICU_RUNPATH_FLAGS) $(IMA_BOOST_RUNPATH_FLAGS) $(IMA_RUNPATH_FLAGS) $(RUNPATH_FLAGS) -Wl,-rpath-link,$(COMMON_LIBDIR):$(CLIENT_COMMON_LIBDIR) \
	$(filter %$(OBJ),$^) \
	$(SANITIZER_LDLIB) \
	$(addprefix -l,$(subst lib,,$(basename $(notdir $(filter %$(SO),$^ $|))))) \
	-L$(COMMON_LIBDIR) $(IMA_ICU_LIBS) $(IMA_BOOST_LIBS) $(if $(1),$(1),$(LDLIBS)) $($(notdir $@)-LDLIBS)
endef

# Function: Link debug objects into a library
define debug-make-c-library
	$(call eyecatcher, Link Library Target:${notdir $@} Project:$(PROJECT) Debug version)
	$(call show-version)
	$(call ensure-output-dir, $@)
	$(CC) $(LDFLAGS) -o $@ \
	$(DEBUG_CFLAGS) $(SHARED_FLAGS) $(subst $(BINDIR),$(DEBUG_BINDIR),$(XFLAGS)) $(SONAME_FLAGS) \
	$(IMA_ICU_RUNPATH_FLAGS) $(IMA_BOOST_RUNPATH_FLAGS) $(IMA_DEBUG_RUNPATH_FLAGS) $(DEBUG_RUNPATH_FLAGS) -Wl,-rpath-link,$(COMMON_DEBUG_LIBDIR):$(CLIENT_COMMON_DEBUG_LIBDIR) \
	$(filter %$(OBJ),$^) \
	$(SANITIZER_LDLIB) \
	$(addprefix -l,$(subst lib,,$(basename $(notdir $(filter %$(SO),$^ $|))))) \
	-L$(COMMON_DEBUG_LIBDIR) $(IMA_ICU_LIBS) $(IMA_BOOST_LIBS) $(if $(1),$(1),$(LDLIBS)) $($(notdir $@)-LDLIBS)
endef

# Function: Link coverage objects into a library
define coverage-make-c-library
	$(call eyecatcher, Link Library Target:${notdir $@} Project:$(PROJECT) Coverage version)
	$(call show-version)
	$(call ensure-output-dir, $@)
	$(CC) $(LDFLAGS) -o $@ -g --coverage \
	$(COVERAGE_CFLAGS) $(SHARED_FLAGS) $(subst $(BINDIR),$(COVERAGE_BINDIR),$(XFLAGS)) $(SONAME_FLAGS) \
	$(IMA_ICU_RUNPATH_FLAGS) $(IMA_BOOST_RUNPATH_FLAGS) $(IMA_COVERAGE_RUNPATH_FLAGS) $(COVERAGE_RUNPATH_FLAGS) -Wl,-rpath-link,$(COMMON_COVERAGE_LIBDIR):$(CLIENT_COMMON_COVERAGE_LIBDIR) \
	$(filter %$(OBJ),$^) \
	$(SANITIZER_LDLIB) \
	$(addprefix -l,$(subst lib,,$(basename $(notdir $(filter %$(SO),$^ $|))))) \
	-L$(COMMON_COVERAGE_LIBDIR) $(IMA_ICU_LIBS) $(IMA_BOOST_LIBS) $(if $(1),$(1),$(LDLIBS)) $($(notdir $@)-LDLIBS)
endef

# Function: Link objects into a static library
define make-c-static-library
	$(call eyecatcher, Link Static Library Target:${notdir $@} Project:$(PROJECT))
	$(call show-version)
	$(call ensure-output-dir, $@)
	$(AR) -cvq -o $@ $(filter %$(OBJ),$^)
endef

# Function: Link debug objects into a static library
define debug-make-c-static-library
	$(call eyecatcher, Link Static Library Target:${notdir $@} Project:$(PROJECT) Debug version)
	$(call show-version)
	$(call ensure-output-dir, $@)
	$(AR) -cvq -o $@ $(filter %$(OBJ),$^)
endef

# Function: Link coverage objects into a static library
define coverage-make-c-static-library
	$(call eyecatcher, Link Static Library Target:${notdir $@} Project:$(PROJECT) Coverage version)
	$(call show-version)
	$(call ensure-output-dir, $@)
	$(AR) -cvq -o $@ $(filter %$(OBJ),$^)
endef

# Function: Link test objects and libraries into a program
define build-c-test
	$(call eyecatcher, Link Test Target:${notdir $@} Project:$(PROJECT))
	$(call ensure-output-dir, $@)
	$(CC) $(TESTLDFLAGS) -o $@ \
	$(IMA_ICU_RUNPATH_FLAGS) $(IMA_BOOST_RUNPATH_FLAGS) $(RUNPATH_FLAGS) $(SSL_RUNPATH_FLAGS) -Wl,-rpath,$(COMMON_LIBDIR):$(CLIENT_COMMON_LIBDIR) \
	$(filter %$(OBJ),$^) \
	$(SANITIZER_LDLIB) \
	$(addprefix -l,$(subst lib,,$(basename $(notdir $(filter %$(SO),$^ $|))))) \
	-L$(COMMON_LIBDIR) $(IMA_ICU_LIBS) $(IMA_BOOST_LIBS) $(TESTLDLIBS) $($(notdir $@)-LDLIBS)
endef

# Function: Link debug test objects and libraries into a program
define debug-build-c-test
	$(call eyecatcher, Link Test Target:${notdir $@} Project:$(PROJECT) Debug version)
	$(call ensure-output-dir, $@)
	$(CC) $(TESTLDFLAGS) -o $@ \
	$(IMA_ICU_RUNPATH_FLAGS) $(IMA_BOOST_RUNPATH_FLAGS) $(DEBUG_RUNPATH_FLAGS) $(SSL_RUNPATH_FLAGS) -Wl,-rpath,$(COMMON_DEBUG_LIBDIR):$(CLIENT_COMMON_DEBUG_LIBDIR) \
	$(filter %$(OBJ),$^) \
	$(SANITIZER_LDLIB) \
	$(addprefix -l,$(subst lib,,$(basename $(notdir $(filter %$(SO),$^ $|))))) \
	-L$(COMMON_DEBUG_LIBDIR) $(IMA_ICU_LIBS) $(IMA_BOOST_LIBS) $(TESTLDLIBS) $($(notdir $@)-LDLIBS)
endef

# Function: Link coverage test objects and libraries into a program
define coverage-build-c-test
	$(call eyecatcher, Link Test Target:${notdir $@} Project:$(PROJECT) Coverage version)
	$(call ensure-output-dir, $@)
	$(CC) $(TESTLDFLAGS) -o $@ -g --coverage \
	$(IMA_ICU_RUNPATH_FLAGS) $(IMA_BOOST_RUNPATH_FLAGS) $(COVERAGE_RUNPATH_FLAGS) $(SSL_RUNPATH_FLAGS) -Wl,-rpath,$(COMMON_COVERAGE_LIBDIR):$(CLIENT_COMMON_COVERAGE_LIBDIR) \
	$(filter %$(OBJ),$^) \
	$(SANITIZER_LDLIB) \
	$(addprefix -l,$(subst lib,,$(basename $(notdir $(filter %$(SO),$^ $|))))) \
	-L$(COMMON_COVERAGE_LIBDIR) $(IMA_ICU_LIBS) $(IMA_BOOST_LIBS) $(TESTLDLIBS) $($(notdir $@)-LDLIBS)
endef

# Function: build c source file from a json file
define build-c-source-from-json
        $(XXD) -i $^ > $@
endef


# Function: Link many variants of a single test
# Parameter 1 specifies whether the test should be built as production (prod),
# debug (debug), coverage (coverage) or all 3 (all), and whether to add it to the list of full test
# targets (full) as opposed to the standard set.
# Parameter 2 is the name of the test program.
# Parameter 3 is the list of source files used (these are converted to object names)
# Parameter 4 is the list of libs to link to (these are converted to lib names)
define build-c-tests
ifneq ($(findstring all,$(1))$(findstring prod,$(1)),)
ifneq ($(findstring full,$(1)),)
TEST-TARGETS-FULL += $(BINDIR)/$(strip $(2))$(EXE)
ifeq ($(findstring manual,$(1)),)
LEGACY_FULL += legacy_$(strip $(2))_full
endif
else
TEST-TARGETS += $(BINDIR)/$(strip $(2))$(EXE)
ifeq ($(findstring manual,$(1)),)
LEGACY_BASIC += legacy_$(strip $(2))
endif
endif
$(BINDIR)/$(strip $(2))$(EXE): $$(call objects, $(3)) | $$(call libs, $(4))
	$$(call build-c-test)
legacy_$(strip $(2)): $(BINDIR)/$(strip $(2))$(EXE)
	$$(call run-legacy-test,$$<)
.PHONY:: legacy_$(strip $(2))
legacy_$(strip $(2))_full: $(BINDIR)/$(strip $(2))$(EXE)
	$$(call run-legacy-test,$$<)
.PHONY:: legacy_$(strip $(2))_full
endif

ifneq ($(findstring all,$(1))$(findstring debug,$(1)),)
ifneq ($(findstring full,$(1)),)
DEBUG-TEST-TARGETS-FULL += $(DEBUG_BINDIR)/$(strip $(2))$(EXE)
ifeq ($(findstring manual,$(1)),)
DEBUG_LEGACY_FULL += legacy_$(strip $(2))_debug_full
endif
else
DEBUG-TEST-TARGETS += $(DEBUG_BINDIR)/$(strip $(2))$(EXE)
ifeq ($(findstring manual,$(1)),)
DEBUG_LEGACY_BASIC += legacy_$(strip $(2))_debug
endif
endif
$(DEBUG_BINDIR)/$(strip $(2))$(EXE): $$(call debug-objects, $(3)) | $$(call debug-libs, $(4))
	$$(call debug-build-c-test)
legacy_$(strip $(2))_debug: $(DEBUG_BINDIR)/$(strip $(2))$(EXE)
	$$(call debug-run-legacy-test,$$<)
.PHONY:: legacy_$(strip $(2))_debug
legacy_$(strip $(2))_debug_full: $(DEBUG_BINDIR)/$(strip $(2))$(EXE)
	$$(call debug-run-legacy-test,$$<)
.PHONY:: legacy_$(strip $(2))_debug_full
endif

ifneq ($(findstring all,$(1))$(findstring coverage,$(1)),)
ifneq ($(findstring full,$(1)),)
COVERAGE-TEST-TARGETS-FULL += $(COVERAGE_BINDIR)/$(strip $(2))$(EXE)
ifeq ($(findstring manual,$(1)),)
COVERAGE_LEGACY_FULL += legacy_$(strip $(2))_coverage_full
endif
else
COVERAGE-TEST-TARGETS += $(COVERAGE_BINDIR)/$(strip $(2))$(EXE)
ifeq ($(findstring manual,$(1)),)
COVERAGE_LEGACY_BASIC += legacy_$(strip $(2))_coverage
endif
endif
$(COVERAGE_BINDIR)/$(strip $(2))$(EXE): $$(call coverage-objects, $(3)) | $$(call coverage-libs, $(4))
	$$(call coverage-build-c-test)
legacy_$(strip $(2))_coverage: $(COVERAGE_BINDIR)/$(strip $(2))$(EXE)
	$$(call coverage-run-legacy-test,$$<)
.PHONY:: legacy_$(strip $(2))_coverage
legacy_$(strip $(2))_coverage_full: $(COVERAGE_BINDIR)/$(strip $(2))$(EXE)
	$$(call coverage-run-legacy-test,$$<)
.PHONY:: legacy_$(strip $(2))_coverage_full
endif

endef

# Function: Link many variants of a single cunit test
# Parameter 1 specifies whether the test should be built as production (prod),
# debug (debug) or both (all), and whether to add it to the list of full test
# targets (full) as opposed to the standard set.
# Parameter 2 is the name of the test program.
# Parameter 3 is the list of source files used (these are converted to object names)
# Parameter 4 is the list of libs to link to (these are converted to lib names)
define build-cunit-tests
ifneq ($(findstring all,$(1))$(findstring prod,$(1)),)
ifneq ($(findstring full,$(1)),)
CUNIT-TARGETS-FULL += $(CUNITTESTDIR)/$(strip $(2))$(EXE)
ifeq ($(findstring manual,$(1)),)
CUNIT_FULL += cunit_$(strip $(2))_full
endif
else
CUNIT-TARGETS += $(CUNITTESTDIR)/$(strip $(2))$(EXE)
ifeq ($(findstring manual,$(1)),)
CUNIT_BASIC += cunit_$(strip $(2))
endif
endif
$(CUNITTESTDIR)/$(strip $(2))$(EXE): $$(call objects, $(3)) | $$(call libs, $(4))
	$$(call build-cunit-test)
cunit_$(strip $(2)): $(CUNITTESTDIR)/$(strip $(2))$(EXE)
	$$(call run-cunit-test,$$<)
.PHONY:: cunit_$(strip $(2))
cunit_$(strip $(2))_full: $(CUNITTESTDIR)/$(strip $(2))$(EXE)
	$$(call run-cunit-test,$$<,FULL)
.PHONY:: cunit_$(strip $(2))_full
endif

ifneq ($(findstring all,$(1))$(findstring debug,$(1)),)
ifneq ($(findstring full,$(1)),)
DEBUG-CUNIT-TARGETS-FULL += $(DEBUG_CUNITTESTDIR)/$(strip $(2))$(EXE)
ifeq ($(findstring manual,$(1)),)
DEBUG_CUNIT_FULL += cunit_$(strip $(2))_debug_full
endif
else
DEBUG-CUNIT-TARGETS += $(DEBUG_CUNITTESTDIR)/$(strip $(2))$(EXE)
ifeq ($(findstring manual,$(1)),)
DEBUG_CUNIT_BASIC += cunit_$(strip $(2))_debug
endif
endif
$(DEBUG_CUNITTESTDIR)/$(strip $(2))$(EXE): $$(call debug-objects, $(3)) | $$(call debug-libs, $(4))
	$$(call debug-build-cunit-test)
cunit_$(strip $(2))_debug: $(DEBUG_CUNITTESTDIR)/$(strip $(2))$(EXE)
	$$(call debug-run-cunit-test,$$<)
.PHONY:: cunit_$(strip $(2))_debug
cunit_$(strip $(2))_debug_full: $(DEBUG_CUNITTESTDIR)/$(strip $(2))$(EXE)
	$$(call debug-run-cunit-test,$$<,FULL)
.PHONY:: cunit_$(strip $(2))_debug_full
endif

ifneq ($(findstring all,$(1))$(findstring coverage,$(1)),)
ifneq ($(findstring full,$(1)),)
COVERAGE-CUNIT-TARGETS-FULL += $(COVERAGE_CUNITTESTDIR)/$(strip $(2))$(EXE)
ifeq ($(findstring manual,$(1)),)
COVERAGE_CUNIT_FULL += cunit_$(strip $(2))_coverage_full
endif
else
COVERAGE-CUNIT-TARGETS += $(COVERAGE_CUNITTESTDIR)/$(strip $(2))$(EXE)
ifeq ($(findstring manual,$(1)),)
COVERAGE_CUNIT_BASIC += cunit_$(strip $(2))_coverage
endif
endif
$(COVERAGE_CUNITTESTDIR)/$(strip $(2))$(EXE): $$(call coverage-objects, $(3)) | $$(call coverage-libs, $(4))
	$$(call coverage-build-cunit-test)
cunit_$(strip $(2))_coverage: $(COVERAGE_CUNITTESTDIR)/$(strip $(2))$(EXE)
	$$(call coverage-run-cunit-test,$$<)
.PHONY:: cunit_$(strip $(2))_coverage
cunit_$(strip $(2))_coverage_full: $(COVERAGE_CUNITTESTDIR)/$(strip $(2))$(EXE)
	$$(call coverage-run-cunit-test,$$<,FULL)
.PHONY:: cunit_$(strip $(2))_coverage_full
endif
endef

# Function: Link many variants of a single cunit test with a nonstandard target type
# Parameter 1 specifies the target type to use
# Parameter 2 is the name of the test program.
# Parameter 3 is the list of source files used (these are converted to object names)
# Parameter 4 is the list of libs to link to (these are converted to lib names)
define build-cunit-tests-target
$(strip $(1))-CUNIT-TARGETS += $(CUNITTESTDIR)/$(strip $(2))$(EXE)
CUNIT_$(strip $(1)) += cunit_$(strip $(2))
$(CUNITTESTDIR)/$(strip $(2))$(EXE): $$(call objects, $(3)) | $$(call libs, $(4))
	$$(call build-cunit-test)
cunit_$(strip $(2)): $(CUNITTESTDIR)/$(strip $(2))$(EXE)
	$$(call run-cunit-test,$$<)
.PHONY:: cunit_$(strip $(2))

$(strip $(1))-DEBUG-CUNIT-TARGETS += $(DEBUG_CUNITTESTDIR)/$(strip $(2))$(EXE)
DEBUG_CUNIT_$(strip $(1)) += cunit_$(strip $(2))_debug
$(DEBUG_CUNITTESTDIR)/$(strip $(2))$(EXE): $$(call debug-objects, $(3)) | $$(call debug-libs, $(4))
	$$(call debug-build-cunit-test)
cunit_$(strip $(2))_debug: $(DEBUG_CUNITTESTDIR)/$(strip $(2))$(EXE)
	$$(call debug-run-cunit-test,$$<)
.PHONY:: cunit_$(strip $(2))_debug

$(strip $(1))-COVERAGE-CUNIT-TARGETS += $(COVERAGE_CUNITTESTDIR)/$(strip $(2))$(EXE)
COVERAGE_CUNIT_$(strip $(1)) += cunit_$(strip $(2))_coverage
$(COVERAGE_CUNITTESTDIR)/$(strip $(2))$(EXE): $$(call coverage-objects, $(3)) | $$(call coverage-libs, $(4))
	$$(call coverage-build-cunit-test)
cunit_$(strip $(2))_coverage: $(COVERAGE_CUNITTESTDIR)/$(strip $(2))$(EXE)
	$$(call coverage-run-cunit-test,$$<)
.PHONY:: cunit_$(strip $(2))_coverage
endef

# Function: Link cunit objects into a program
define build-cunit-test
	$(call eyecatcher, Link CUnit Test Target:${notdir $@} Project:$(PROJECT))
	$(call ensure-output-dir, $@)
	$(CC) $(TESTLDFLAGS) -o $@ \
	$(IMA_ICU_RUNPATH_FLAGS) $(IMA_BOOST_RUNPATH_FLAGS) $(SSL_RUNPATH_FLAGS) $(RUNPATH_FLAGS) -Wl,-rpath,$(COMMON_LIBDIR):$(CLIENT_COMMON_LIBDIR) $(CUNIT_RUNPATH_FLAGS) \
	$(filter %$(OBJ),$^) \
	$(SANITIZER_LDLIB) \
	$(addprefix -l,$(subst lib,,$(basename $(notdir $(filter %$(SO),$^ $|))))) \
	-L$(COMMON_LIBDIR) $(IMA_ICU_LIBS) $(IMA_BOOST_LIBS) $(TESTLDLIBS) $($(notdir $@)-LDLIBS) $(CUNIT_FLAGS) $(CUNIT_LIBS)
endef

# Function: Link debug cunit objects into a program
define debug-build-cunit-test
	$(call eyecatcher, Link CUnit Test Target:${notdir $@} Project:$(PROJECT) Debug version)
	$(call ensure-output-dir, $@)
	$(CC) $(TESTLDFLAGS) -o $@ \
	$(IMA_ICU_RUNPATH_FLAGS) $(IMA_BOOST_RUNPATH_FLAGS) $(SSL_RUNPATH_FLAGS) $(DEBUG_RUNPATH_FLAGS) -Wl,-rpath,$(COMMON_DEBUG_LIBDIR):$(CLIENT_COMMON_DEBUG_LIBDIR) $(CUNIT_RUNPATH_FLAGS) \
	$(filter %$(OBJ),$^) \
	$(SANITIZER_LDLIB) \
	$(addprefix -l,$(subst lib,,$(basename $(notdir $(filter %$(SO),$^ $|))))) \
	-L$(COMMON_DEBUG_LIBDIR) $(IMA_ICU_LIBS) $(IMA_BOOST_LIBS) $(TESTLDLIBS) $($(notdir $@)-LDLIBS) $(CUNIT_FLAGS) $(CUNIT_LIBS)
endef

# Function: Link coverage cunit objects into a program
define coverage-build-cunit-test
	$(call eyecatcher, Link CUnit Test Target:${notdir $@} Project:$(PROJECT) Coverage version)
	$(call ensure-output-dir, $@)
	$(CC) $(TESTLDFLAGS) -o $@ -g --coverage \
	$(IMA_ICU_RUNPATH_FLAGS) $(IMA_BOOST_RUNPATH_FLAGS) $(SSL_RUNPATH_FLAGS) $(COVERAGE_RUNPATH_FLAGS) -Wl,-rpath,$(COMMON_COVERAGE_LIBDIR):$(CLIENT_COMMON_COVERAGE_LIBDIR) $(CUNIT_RUNPATH_FLAGS) \
	$(filter %$(OBJ),$^) \
	$(SANITIZER_LDLIB) \
	$(addprefix -l,$(subst lib,,$(basename $(notdir $(filter %$(SO),$^ $|))))) \
	-L$(COMMON_COVERAGE_LIBDIR) $(IMA_ICU_LIBS) $(TESTLDLIBS) $($(notdir $@)-LDLIBS) $(CUNIT_FLAGS) $(CUNIT_LIBS)
endef

# Function: Run a cunit test program - for ASAN (or GCC_HOME?), runpath doesn't seem to work for dlopen
define run-cunit-test
	$(call eyecatcher, Run Test Target:$(notdir $@) Binary:$(notdir $(1)) Project:$(PROJECT))
	PRJDIR=$(PRJDIR) ROOTREL=$(ROOTREL) \
	$(if $(GCC_HOME)$(ASAN_BUILD),LD_LIBRARY_PATH=$(COMMON_LIBDIR),) \
	$(1) $(2)
endef

# Function: Run a debug cunit test program
define debug-run-cunit-test
	$(call eyecatcher, Run Test Target:$(notdir $@) Binary:$(notdir $(1)) Project:$(PROJECT) Debug version)
	PRJDIR=$(PRJDIR) ROOTREL=$(ROOTREL) \
	$(if $(GCC_HOME)$(ASAN_BUILD),LD_LIBRARY_PATH=$(COMMON_DEBUG_LIBDIR),) \
	$(1) $(2)
endef

# Function: Run a coverage cunit test program
# Prefix the recipe lines with - so they keep going (and coverage is maximised) even if tests fail.
define coverage-run-cunit-test
	$(call eyecatcher, Run Test Target:$(notdir $@) Binary:$(notdir $(1)) Project:$(PROJECT) Coverage version)
	-PRJDIR=$(PRJDIR) ROOTREL=$(ROOTREL) \
	$(if $(GCC_HOME)$(ASAN_BUILD),LD_LIBRARY_PATH=$(COMMON_COVERAGE_LIBDIR),) \
	$(1) $(2)
	-cd $(SRCDIR) && gcov -o $(COVERAGE_BINDIR) $(COVERAGE_BINDIR)/*.gcda
endef

# Function: Run a non-cunit (legacy) test program
define run-legacy-test
	$(call eyecatcher, Run Legacy Test Target:$(notdir $@) Binary:$(notdir $(1)) Project:$(PROJECT))
	PRJDIR=$(PRJDIR) ROOTREL=$(ROOTREL) \
	$(if $(GCC_HOME)$(ASAN_BUILD),LD_LIBRARY_PATH=$(COMMON_LIBDIR),) \
	$(1) $(2)
endef

# Function: Run a debug non-cunit (legacy) test program
define debug-run-legacy-test
	$(call eyecatcher, Run Legacy Test Target:$(notdir $@) Binary:$(notdir $(1)) Project:$(PROJECT) Debug version)
	PRJDIR=$(PRJDIR) ROOTREL=$(ROOTREL) \
	$(if $(GCC_HOME)$(ASAN_BUILD),LD_LIBRARY_PATH=$(COMMON_DEBUG_LIBDIR),) \
	$(1) $(2)
endef

# Function: Run a coverage non-cunit (legacy) test program
define coverage-run-legacy-test
	$(call eyecatcher, Run Legacy Test Target:$(notdir $@) Binary:$(notdir $(1)) Project:$(PROJECT) Coverage version)
	PRJDIR=$(PRJDIR) ROOTREL=$(ROOTREL) \
	$(if $(GCC_HOME)$(ASAN_BUILD),LD_LIBRARY_PATH=$(COMMON_COVERAGE_LIBDIR),) \
	$(1) $(2)
endef

# Function: Clean up (remove) targets
define clean-targets
	$(call eyecatcher, Clean Target:$@ Project:$(PROJECT))
	$(CD) $(BINDIR); \
	$(RM) -f *$(OBJ) *$(DEP) *.map
	$(RM) -f $(LIB-TARGETS) $(LIB_S-TARGETS) $(EXE-TARGETS) $(EXE_S-TARGETS) \
	         $(CFG-TARGETS) $(EXP-TARGETS) \
	         $(TEST-TARGETS) $(CUNIT-TARGETS) \
	         $(PUBLISH-TARGETS) $(CLIENT-PUBLISH-TARGETS)
	$(CD) $(DEBUG_BINDIR); \
	$(RM) -f *$(OBJ) *$(DEP) *.map
	$(RM) -f $(DEBUG-LIB-TARGETS) $(DEBUG-LIB_S-TARGETS) $(DEBUG-EXE-TARGETS) $(DEBUG-EXE_S-TARGETS) \
	         $(DEBUG-CFG-TARGETS) $(DEBUG-EXP-TARGETS) \
	         $(DEBUG-TEST-TARGETS) $(DEBUG-CUNIT-TARGETS) \
	         $(DEBUG-PUBLISH-TARGETS) $(DEBUG-CLIENT-PUBLISH-TARGETS)
	$(CD) $(COVERAGE_BINDIR); \
	$(RM) -f *$(OBJ) *$(DEP) *.gcda *.gcno *.map
	$(RM) -f $(COVERAGE-LIB-TARGETS) $(COVERAGE-LIB_S-TARGETS) $(COVERAGE-EXE-TARGETS) $(COVERAGE-EXE_S-TARGETS) \
	         $(COVERAGE-CFG-TARGETS) $(COVERAGE-EXP-TARGETS) \
	         $(COVERAGE-TEST-TARGETS) $(COVERAGE-CUNIT-TARGETS) \
	         $(COVERAGE-PUBLISH-TARGETS) $(COVERAGE-CLIENT-PUBLISH-TARGETS)
	$(RM) -rf $(DOXYDIR)
endef

# Function: Show the version of gcc
define show-version
	$(CC) -v 2>&1
endef

# Function: Add a directory to the search path for a given file type
define add-search-directory
ifneq ($(PROOT),)
ifneq ($(SROOT),)
vpath %$(1) $(subst $(abspath $(SROOT)),$(abspath $(PROOT)),$(2))
endif
endif
vpath %$(1) $(2)
endef

COMMON-DIR-TARGETS =  $(COMMON_INCDIR) $(CLIENT_COMMON_INCDIR) $(OUTSRCDIR)

DIR-TARGETS = $(BINDIR) $(LIBDIR) $(LIBDIR_S) \
              $(COMMON_BINDIR) $(COMMON_LIBDIR) $(COMMON_LIBDIR_S) \
              $(CUNITTESTDIR) \
              $(CLIENT_COMMON_BINDIR) $(CLIENT_COMMON_LIBDIR)

DEBUG-DIR-TARGETS = $(DEBUG_BINDIR) $(DEBUG_LIBDIR) $(DEBUG_LIBDIR_S) \
                    $(COMMON_DEBUG_BINDIR) $(COMMON_DEBUG_LIBDIR) $(COMMON_DEBUG_LIBDIR_S) \
                    $(DEBUG_CUNITTESTDIR) \
                    $(CLIENT_COMMON_DEBUG_BINDIR) $(CLIENT_COMMON_DEBUG_LIBDIR)

COVERAGE-DIR-TARGETS = $(COVERAGE_BINDIR) $(COVERAGE_LIBDIR) $(COVERAGE_LIBDIR_S) \
                       $(COMMON_COVERAGE_BINDIR) $(COMMON_COVERAGE_LIBDIR) $(COMMON_COVERAGE_LIBDIR_S) \
                       $(COVERAGE_CUNITTESTDIR) \
                       $(CLIENT_COMMON_COVERAGE_BINDIR) $(CLIENT_COMMON_COVERAGE_LIBDIR)

DIR-TARGETS-64 = $(LIBDIR_64) $(CLIENT_COMMON_LIBDIR_64)
DEBUG-DIR-TARGETS-64 = $(DEBUG_LIBDIR_64) $(CLIENT_COMMON_DEBUG_LIBDIR_64)
COVERAGE-DIR-TARGETS-64 = $(COVERAGE_LIBDIR_64) $(CLIENT_COMMON_COVERAGE_LIBDIR_64)

$(COMMON-DIR-TARGETS):
ifeq ($(realpath $@),)
	$(call eyecatcher, Create Common Directory Target:$@ Project:$(PROJECT))
	$(MKDIR) $@
endif

$(DIR-TARGETS):
ifeq ($(realpath $@),)
	$(call eyecatcher, Create Directory Target:$@ Project:$(PROJECT))
	$(MKDIR) $@
endif

$(DIR-TARGETS-64):
ifeq ($(realpath $@),)
	$(call eyecatcher, Create Directory Target:$@ Project:$(PROJECT))
	$(MKDIR) $@
endif

$(DEBUG-DIR-TARGETS):
ifeq ($(realpath $@),)
	$(call eyecatcher, Create Debug Directory Target:$@ Project:$(PROJECT))
	$(MKDIR) $@
endif

$(DEBUG-DIR-TARGETS-64):
ifeq ($(realpath $@),)
	$(call eyecatcher, Create Debug Directory Target:$@ Project:$(PROJECT))
	$(MKDIR) $@
endif

$(COVERAGE-DIR-TARGETS):
ifeq ($(realpath $@),)
	$(call eyecatcher, Create Coverage Directory Target:$@ Project:$(PROJECT))
	$(MKDIR) $@
endif

$(COVERAGE-DIR-TARGETS-64):
ifeq ($(realpath $@),)
	$(call eyecatcher, Create Coverage Directory Target:$@ Project:$(PROJECT))
	$(MKDIR) $@
endif

$(DOXYDIR):
ifeq ($(realpath $@),)
	$(call eyecatcher, Create Doxygen directory Target:$@ Project:$(PROJECT))
	$(MKDIR) $@
endif

show_version:
	$(call eyecatcher, Target:$@  Project:$(PROJECT))
	$(call show-version)
.PHONY:: show_version

clean:
	$(call clean-targets)
.PHONY:: clean

doxygen: $(DOXYDIR)
	$(call eyecatcher, Create Doxygen documentation Project:$(PROJECT))
	sed -e "s/ISM_ENGINE_INPUT/$(subst /,\/,$(PRJDIR))/g" -e "s/ISM_ENGINE_OUTPUT/$(subst /,\/,$(DOXYDIR))/g" $(ROOTREL)/server_build/Doxyfile.template | doxygen -
.PHONY:: doxygen

coveragereport:
	$(call eyecatcher, Create Coverage documentation Project:$(PROJECT))
	$(MKDIR) $(OUTDIR)/$(OUTROOTREL)/coverage
	geninfo -o $(OUTDIR)/$(OUTROOTREL)/coverage/coverage.info $(OUTDIR)/$(OUTROOTREL)
	genhtml -o $(OUTDIR)/$(OUTROOTREL)/coverage --ignore-errors source $(OUTDIR)/$(OUTROOTREL)/coverage/coverage.info
.PHONY:: coveragereport

# Paths to search for for .c files - Additional ones can be added in Makefile.
$(eval $(call add-search-directory,.c,$(SRCDIR)))
$(eval $(call add-search-directory,.c,$(TSTDIR)))
$(eval $(call add-search-directory,.c,$(UTILSRCDIR)))
$(eval $(call add-search-directory,.c,$(SRCDIR)/play))
# Paths to search for .cpp files - Additional ones can be added in Makefile
$(eval $(call add-search-directory,.cpp,$(SRCDIR)))
$(eval $(call add-search-directory,.cpp,$(TSTDIR)))

define recompile-as
ifeq ($(BEAM_BUILD),yes)
$(BINDIR)/$(2)$(OBJ): $(1)
	$$(call beam-c-src,$$@,$$<,$(3))
else
$(BINDIR)/$(2)$(OBJ): $(1)
	$$(call compile-c-src,$$@,$$<,$(3))
endif

$(DEBUG_BINDIR)/$(2)$(OBJ): $(1)
	$$(call debug-compile-c-src,$$@,$$<,$(3))

$(COVERAGE_BINDIR)/$(2)$(OBJ): $(1)
	$$(call coverage-compile-c-src,$$@,$$<,$(3))
endef

ifeq ($(BEAM_BUILD),yes)
$(BINDIR)/%$(OBJ): %.c $(MAKEFILE_LIST)
	$(call beam-c-src,$@,$<)

$(BINDIR)/%$(OBJ): %.cpp $(MAKEFILE_LIST)
	$(call beam-cpp-src,$@,$<)
else
$(BINDIR)/%$(OBJ): %.c $(MAKEFILE_LIST)
	$(call compile-c-src,$@,$<)

$(BINDIR)/%$(OBJ): %.cpp $(MAKEFILE_LIST)
	$(call compile-cpp-src,$@,$<)
endif

$(DEBUG_BINDIR)/%$(OBJ): %.c $(MAKEFILE_LIST)
	$(call debug-compile-c-src,$@,$<)

$(DEBUG_BINDIR)/%$(OBJ): %.cpp $(MAKEFILE_LIST)
	$(call debug-compile-cpp-src,$@,$<)

$(COVERAGE_BINDIR)/%$(OBJ): %.c $(MAKEFILE_LIST)
	$(call coverage-compile-c-src,$@,$<)

$(COVERAGE_BINDIR)/%$(OBJ): %.cpp $(MAKEFILE_LIST)
	$(call coverage-compile-cpp-src,$@,$<)

# Paths to search for .cfg files - Additional ones can be added in Makefile.
$(eval $(call add-search-directory,.cfg,$(CFGDIR)))

$(BINDIR)/%.cfg: %.cfg $(MAKEFILE_LIST)
	$(call eyecatcher, Config Target:$(notdir $@) Project:$(PROJECT))
	$(call ensure-output-dir, $@)
	$(CP) $< $@

$(DEBUG_BINDIR)/%.cfg: %.cfg $(MAKEFILE_LIST)
	$(call eyecatcher, Config Target:$(notdir $@) Project:$(PROJECT) Debug version)
	$(call ensure-output-dir, $@)
	$(CP) $< $@

$(COVERAGE_BINDIR)/%.cfg: %.cfg $(MAKEFILE_LIST)
	$(call eyecatcher, Config Target:$(notdir $@) Project:$(PROJECT) Coverage version)
	$(call ensure-output-dir, $@)
	$(CP) $< $@

$(COMMON_LIBDIR)/%$(SO): $(LIBDIR)/%$(SO) $(MAKEFILE_LIST)
	$(call eyecatcher, Publish Target:$(notdir $@) Project:$(PROJECT))
	$(call ensure-output-dir, $@)
	$(CP) $< $@

$(COMMON_LIBDIR_S)/%$(A): $(LIBDIR_S)/%$(A) $(MAKEFILE_LIST)
	$(call eyecatcher, Publish Target:$(notdir $@) Project:$(PROJECT))
	$(call ensure-output-dir, $@)
	$(CP) $< $@

$(COMMON_BINDIR)/%$(EXE): $(BINDIR)/%$(EXE) $(MAKEFILE_LIST)
	$(call eyecatcher, Publish Target:$(notdir $@) Project:$(PROJECT))
	$(call ensure-output-dir, $@)
	$(CP) $< $@

$(COMMON_BINDIR)/%.cfg: $(BINDIR)/%.cfg $(MAKEFILE_LIST)
	$(call eyecatcher, Publish Target:$(notdir $@) Project:$(PROJECT))
	$(call ensure-output-dir, $@)
	$(CP) $< $@

$(COMMON_INCDIR)/%.h: $(INCDIR)/%.h $(MAKEFILE_LIST)
	$(call eyecatcher, Publish Target:$(notdir $@) Project:$(PROJECT))
	$(call ensure-output-dir, $@)
	$(CP) $< $@

$(COMMON_DEBUG_LIBDIR)/%$(SO): $(DEBUG_LIBDIR)/%$(SO) $(MAKEFILE_LIST)
	$(call eyecatcher, Publish Target:$(notdir $@) Project:$(PROJECT))
	$(call ensure-output-dir, $@)
	$(CP) $< $@

$(COMMON_DEBUG_LIBDIR_S)/%$(A): $(DEBUG_LIBDIR_S)/%$(A) $(MAKEFILE_LIST)
	$(call eyecatcher, Publish Target:$(notdir $@) Project:$(PROJECT))
	$(call ensure-output-dir, $@)
	$(CP) $< $@

$(COMMON_DEBUG_BINDIR)/%$(EXE): $(DEBUG_BINDIR)/%$(EXE) $(MAKEFILE_LIST)
	$(call eyecatcher, Publish Target:$(notdir $@) Project:$(PROJECT))
	$(call ensure-output-dir, $@)
	$(CP) $< $@

$(COMMON_COVERAGE_LIBDIR)/%$(SO): $(COVERAGE_LIBDIR)/%$(SO) $(MAKEFILE_LIST)
	$(call eyecatcher, Publish Target:$(notdir $@) Project:$(PROJECT))
	$(call ensure-output-dir, $@)
	$(CP) $< $@

$(COMMON_COVERAGE_LIBDIR_S)/%$(A): $(COVERAGE_LIBDIR_S)/%$(A) $(MAKEFILE_LIST)
	$(call eyecatcher, Publish Target:$(notdir $@) Project:$(PROJECT))
	$(call ensure-output-dir, $@)
	$(CP) $< $@

$(COMMON_COVERAGE_BINDIR)/%$(EXE): $(COVERAGE_BINDIR)/%$(EXE) $(MAKEFILE_LIST)
	$(call eyecatcher, Publish Target:$(notdir $@) Project:$(PROJECT))
	$(call ensure-output-dir, $@)
	$(CP) $< $@

$(CLIENT_COMMON_LIBDIR)/%$(SO): $(LIBDIR)/%$(SO) $(MAKEFILE_LIST)
	$(call eyecatcher, Publish Target:$(notdir $@) Project:$(PROJECT))
	$(call ensure-output-dir, $@)
	$(CP) $< $@

$(CLIENT_COMMON_BINDIR)/%$(EXE): $(BINDIR)/%$(EXE) $(MAKEFILE_LIST)
	$(call eyecatcher, Publish Target:$(notdir $@) Project:$(PROJECT))
	$(call ensure-output-dir, $@)
	$(CP) $< $@

$(CLIENT_COMMON_INCDIR)/%.h: $(INCDIR)/%.h $(MAKEFILE_LIST)
	$(call eyecatcher, Publish Target:$(notdir $@) Project:$(PROJECT))
	$(call ensure-output-dir, $@)
	$(CP) $< $@

$(CLIENT_COMMON_DEBUG_LIBDIR)/%$(SO): $(DEBUG_LIBDIR)/%$(SO) $(MAKEFILE_LIST)
	$(call eyecatcher, Publish Target:$(notdir $@) Project:$(PROJECT))
	$(call ensure-output-dir, $@)
	$(CP) $< $@

$(CLIENT_COMMON_DEBUG_BINDIR)/%$(EXE): $(DEBUG_BINDIR)/%$(EXE) $(MAKEFILE_LIST)
	$(call eyecatcher, Publish Target:$(notdir $@) Project:$(PROJECT))
	$(call ensure-output-dir, $@)
	$(CP) $< $@

# 64-bit publishes
$(CLIENT_COMMON_LIBDIR_64)/%$(SO): $(LIBDIR_64)/%$(SO) $(MAKEFILE_LIST)
	$(call eyecatcher, Publish Target:$(notdir $@) Project:$(PROJECT))
	$(call ensure-output-dir, $@)
	$(CP) $< $@

$(CLIENT_COMMON_DEBUG_LIBDIR_64)/%$(SO): $(DEBUG_LIBDIR_64)/%$(SO) $(MAKEFILE_LIST)
	$(call eyecatcher, Publish Target:$(notdir $@) Project:$(PROJECT) Debug version)
	$(call ensure-output-dir, $@)
	$(CP) $< $@

$(CLIENT_COMMON_COVERAGE_LIBDIR_64)/%$(SO): $(COVERAGE_LIBDIR_64)/%$(SO) $(MAKEFILE_LIST)
	$(call eyecatcher, Publish Target:$(notdir $@) Project:$(PROJECT) Coverage version)
	$(call ensure-output-dir, $@)
	$(CP) $< $@

# include source file dependencies
include $(wildcard $(BINDIR)/*$(DEP))
include $(wildcard $(DEBUG_BINDIR)/*$(DEP))
include $(wildcard $(COVERAGE_BINDIR)/*$(DEP))

# Rule to print out the value, origin and flavor of a make variable, for instance
# make print-CFLAGS will print the value of the variable CFLAGS
print-%: ; @echo '$(subst ','\'',$*=$($*)) (origin: $(origin $*), flavor: $(flavor $*))'

war:
	@echo make test not war!

tea:
	@echo Would you like a biscuit and a nice sit-down with that?

hay:
	@echo But only while the sun shines!

# Shorthand for common make targets
prod: production
prd: production
tst: test
full: fulltest
dbg: debug
dbgtst: debugtest
dbgfull: debugfulltest
cov: coverage
covrep: coveragereport
covtst: coveragetest
covfull: coveragefulltest
dox: doxygen
cln: clean
ver: show_version

.PHONY:: war tea hay prd tst full dbg dbgtst dbgfull cov covrep covtst covfull dox cln ver
