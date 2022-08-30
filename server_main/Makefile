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

#
# Component:	server
# SubComponent: server_main
#
# Created on:
#     Author:
#


PROJECT = server_main

# Need to have PRJDIR before include so SRCDIR, INCDIR can be set correctly
PRJDIR=$(CURDIR)
# Specify the relative path from this project directory to the root of the directory structure
ROOTREL=..

include $(ROOTREL)/server_build/common.mk

#Files in this component need to know paths for server
#(uses the path_parser.py/paths.properties mechanism)
PATH_VAR_CFLAGS=$(IMA_SVR_PATH_DEFINES)


# Add any required flags not specified in $(ROOTREL)/server_build/common.mk
IDFLAGS += 
IFLAGS += -I$(ROOTREL)/server_snmp/src
CFLAGS +=
CPPFLAGS +=
LDFLAGS += -Wl,-rpath,$(IMASERVER_RUNPATH_DIR)
LDLIBS += -ldl -lssl -lcrypto -licuuc -licui18n -l$(OPENLDAP_LIBNAME) -llber -lcurl -lnetsnmp -lnetsnmpagent -lnetsnmphelpers -lMCP_Routing
XFLAGS +=
SHARED_FLAGS +=

# ------------------------------------------------
# Add project specific make rules
# ------------------------------------------------

imaserver-FILES = main.c copyright.c

imaserver-LIBS = libismutil$(SO)      \
                 libismtransport$(SO) \
                 libismengine$(SO)    \
                 libismadmin$(SO)     \
                 libismprotocol$(SO)  \
                 libismstore$(SO)     \
                 libismmonitoring$(SO) \
                 libismsnmp$(SO)

imaserver-LIBS_S = libismprotocol$(A)    \
                   libismmonitoring$(A)  \
                   libismengine$(A)      \
                   libismstore$(A)       \
                   libismtransport$(A)   \
                   libismadmin$(A)       \
                   libismutil$(A)

hasher-LIBS = libismutil$(SO) \
              libismtransport$(SO) \
              libismengine$(SO)    \
              libismadmin$(SO) \
              libismprotocol$(SO) \
              libismstore$(SO) \
              libismmonitoring$(SO) \
              libismsnmp$(SO)

EXE-TARGETS += $(BINDIR)/imahasher$(EXE)
$(BINDIR)/imahasher$(EXE): $(call objects, hasher.c) | \
                           $(call libs, $(hasher-LIBS))
	$(call link-c-program)

EXE-TARGETS += $(BINDIR)/imaserver$(EXE) 
$(BINDIR)/imaserver$(EXE): $(call objects, $(imaserver-FILES)) | \
                           $(call libs, $(imaserver-LIBS))
	$(call link-c-program)

#EXE_S-TARGETS += $(BINDIR)/imaserver_s$(EXE) 
#$(BINDIR)/imaserver_s$(EXE): $(call objects, $(imaserver-FILES)) \
#                             $(call libs_s, $(imaserver-LIBS_S))
#	$(call link-c-program-static)

CFG-TARGETS += $(BINDIR)/server.cfg $(BINDIR)/server_dynamic.cfg

DEBUG-EXE-TARGETS += $(DEBUG_BINDIR)/imaserver$(EXE)
$(DEBUG_BINDIR)/imaserver$(EXE): $(call debug-objects, $(imaserver-FILES)) | \
                                 $(call debug-libs, $(imaserver-LIBS))
	$(call debug-link-c-program)

#DEBUG-EXE_S-TARGETS += $(DEBUG_BINDIR)/imaserver_s$(EXE)
#$(DEBUG_BINDIR)/imaserver_s$(EXE): $(call debug-objects, $(imaserver-FILES)) \
#                                   $(call debug-libs_s, $(imaserver-LIBS_S))
#	$(call debug-link-c-program-static)

DEBUG-CFG-TARGETS += $(DEBUG_BINDIR)/server.cfg $(DEBUG_BINDIR)/server_dynamic.cfg

COVERAGE-EXE-TARGETS += $(COVERAGE_BINDIR)/imaserver$(EXE)
$(COVERAGE_BINDIR)/imaserver$(EXE): $(call coverage-objects, $(imaserver-FILES)) | \
                                    $(call coverage-libs, $(imaserver-LIBS))
	$(call coverage-link-c-program)

#COVERAGE-EXE_S-TARGETS += $(COVERAGE_BINDIR)/imaserver_s$(EXE)
#$(COVERAGE_BINDIR)/imaserver_s$(EXE): $(call coverage-objects, $(imaserver-FILES)) \
#                                      $(call coverage-libs_s, $(imaserver-LIBS_S))
#	$(call coverage-link-c-program-static)

COVERAGE-CFG-TARGETS += $(COVERAGE_BINDIR)/server.cfg $(COVERAGE_BINDIR)/server_dynamic.cfg


# ------------------------------------------------
# Define order of targets (after targets defined)
# ------------------------------------------------

# Targets, Typical order: Libs, Exes, Cfgs, Publish, Experiments, Tests
TARGETS = $(LIB-TARGETS)     \
          $(LIB_S-TARGETS)   \
          $(EXE-TARGETS)     \
          $(CFG-TARGETS)     \
          $(PUBLISH-TARGETS) \
          $(EXP-TARGETS)     \
          $(TEST-TARGETS)    \
          $(CUNIT-TARGETS)

# Debug-Targets, Typical order: Libs, Exes, Cfgs, Publish, Experiments, Tests
DEBUG-TARGETS = $(DEBUG-LIB-TARGETS)     \
                $(DEBUG-LIB_S-TARGETS)   \
                $(DEBUG-EXE-TARGETS)     \
                $(DEBUG-CFG-TARGETS)     \
                $(DEBUG-PUBLISH-TARGETS) \
                $(DEBUG-EXP-TARGETS)     \
                $(DEBUG-TEST-TARGETS)    \
                $(DEBUG-CUNIT-TARGETS)

# Coverage-Targets, Typical order: Libs, Exes, Cfgs, Publish, Experiments, Tests
COVERAGE-TARGETS = $(COVERAGE-LIB-TARGETS)        \
                   $(COVERAGE-LIB_S-TARGETS)      \
                   $(COVERAGE-EXE-TARGETS)        \
                   $(COVERAGE-CFG-TARGETS)        \
                   $(COVERAGE-PUBLISH-TARGETS)    \
                   $(COVERAGE-EXP-TARGETS)        \
                   $(COVERAGE-TEST-TARGETS)       \
                   $(COVERAGE-CUNIT-TARGETS)

# Typical order: Directories, Targets, Debug-Targets
production: $(DIR-TARGETS) $(TARGETS)
debug: $(DEBUG-DIR-TARGETS) $(DEBUG-TARGETS)
coverage: $(COVERAGE-DIR-TARGETS) $(COVERAGE-TARGETS)

all: production debug

test: production $(CUNIT_BASIC)
fulltest: production $(CUNIT_BASIC) $(CUNIT_FULL) 
debugtest: debug $(DEBUG_CUNIT_BASIC)
debugfulltest: debug $(DEBUG_CUNIT_BASIC) $(DEBUG_CUNIT_FULL)
coveragetest: coverage $(COVERAGE_CUNIT_BASIC)
coveragefulltest: coverage $(COVERAGE_CUNIT_BASIC) $(COVERAGE_CUNIT_FULL)

.PHONY:: production debug coverage all test fulltest coveragetest coveragefulltest debugtest debugfulltest
