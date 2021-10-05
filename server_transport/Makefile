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
# SubComponent: server_transport
#
# Created on:
#     Author:
#

PROJECT = server_transport

# Need to have PRJDIR before include so SRCDIR, INCDIR can be set correctly
PRJDIR=$(CURDIR)
# Specify the relative path from this project directory to the root of the directory structure
ROOTREL=..

include $(ROOTREL)/server_build/common.mk
ifneq ($(SSL_HOME),)
SSL_INCLUDE=-I$(SSL_HOME)/include
SSL_LIB= -L$(SSL_HOME)/lib
else
SSL_INCLUDE=
SSL_LIB=
endif
# Add any required flags not specified in $(ROOTREL)/server_build/common.mk
IDFLAGS += 
IFLAGS += $(SSL_INCLUDE)

ifneq (,$(wildcard $(ROOTREL)/server_certificates_ext/include))
     IFLAGS += -I$(ROOTREL)/server_certificates_ext/include
else
     IFLAGS += -I$(ROOTREL)/server_certificates/include
endif

CFLAGS +=
CPPFLAGS +=
LDFLAGS += 
LDLIBS += -lpthread -lismadmin -lismstore -ldl  -lMCP_Routing
XFLAGS +=
SHARED_FLAGS +=

TESTLDFLAGS += $(LDFLAGS)
TESTLDLIBS += $(LDLIBS)

#Files in this component need to know paths for server
#(uses the path_parser.py/paths.properties mechanism)
PATH_VAR_CFLAGS=$(IMA_SVR_PATH_DEFINES)

# ------------------------------------------------
# Add project specific make rules
# ------------------------------------------------

libismtransport-FILES = tcp.c transport.c wstcp.c copyright.c

LIB-TARGETS += $(LIBDIR)/libismtransport$(SO) 
$(LIBDIR)/libismtransport$(SO): $(call objects, $(libismtransport-FILES))
	$(call make-c-library)

DEBUG-LIB-TARGETS += $(DEBUG_LIBDIR)/libismtransport$(SO) 
$(DEBUG_LIBDIR)/libismtransport$(SO): $(call debug-objects, $(libismtransport-FILES))
	$(call debug-make-c-library)

COVERAGE-LIB-TARGETS += $(COVERAGE_LIBDIR)/libismtransport$(SO) 
$(COVERAGE_LIBDIR)/libismtransport$(SO): $(call coverage-objects, $(libismtransport-FILES))
	$(call coverage-make-c-library)

LIB_S-TARGETS += $(LIBDIR_S)/libismtransport$(A) 
$(LIBDIR_S)/libismtransport$(A): $(call objects, $(libismtransport-FILES))
	$(call make-c-static-library)

DEBUG-LIB_S-TARGETS += $(DEBUG_LIBDIR_S)/libismtransport$(A) 
$(DEBUG_LIBDIR_S)/libismtransport$(A): $(call debug-objects, $(libismtransport-FILES))
	$(call debug-make-c-static-library)

COVERAGE-LIB_S-TARGETS += $(COVERAGE_LIBDIR_S)/libismtransport$(A) 
$(COVERAGE_LIBDIR_S)/libismtransport$(A): $(call coverage-objects, $(libismtransport-FILES))
	$(call coverage-make-c-static-library)

test-LIBS = libismutil$(SO) libismtransport$(SO) librum$(SO) libSpiderCast$(SO)

CUNIT-TARGETS += $(CUNITTESTDIR)/transport_test$(EXE)
$(CUNITTESTDIR)/transport_test$(EXE): $(call objects, transport_test.c        \
                                                      wstcp_unit_test.c       \
                                                      transport_unit_test.c   \
                                                      tcp_unit_test.c)      | \
                                      $(call libs, $(test-LIBS))
	$(call build-cunit-test)

DEBUG-CUNIT-TARGETS += $(DEBUG_CUNITTESTDIR)/transport_test$(EXE)
$(DEBUG_CUNITTESTDIR)/transport_test$(EXE): $(call debug-objects, transport_test.c        \
                                                                  wstcp_unit_test.c       \
                                                                  transport_unit_test.c   \
                                                                  tcp_unit_test.c)      | \
                                      $(call debug-libs, $(test-LIBS))
	$(call debug-build-cunit-test)

COVERAGE-CUNIT-TARGETS += $(COVERAGE_CUNITTESTDIR)/transport_test$(EXE)
$(COVERAGE_CUNITTESTDIR)/transport_test$(EXE): $(call coverage-objects, transport_test.c        \
                                                                        wstcp_unit_test.c       \
                                                                        transport_unit_test.c   \
                                                                        tcp_unit_test.c)      | \
                                      $(call coverage-libs, $(test-LIBS))
	$(call coverage-build-cunit-test)

CUNIT_BASIC += transport_test
CUNIT_FULL += transport_test
transport_test: $(CUNITTESTDIR)/transport_test$(EXE)
	$(call run-cunit-test,$<)
.PHONY:: transport_test

DEBUG_CUNIT_BASIC += debug_transport_test
DEBUG_CUNIT_FULL += debug_transport_test
debug_transport_test: $(DEBUG_CUNITTESTDIR)/transport_test$(EXE)
	$(call debug-run-cunit-test,$<)
.PHONY:: debug_transport_test

COVERAGE_CUNIT_BASIC += coverage_transport_test
COVERAGE_CUNIT_FULL += coverage_transport_test
coverage_transport_test: $(COVERAGE_CUNITTESTDIR)/transport_test$(EXE)
	$(call coverage-run-cunit-test,$<)
.PHONY:: coverage_transport_test

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
COVERAGE-TARGETS = $(COVERAGE-LIB-TARGETS)     \
                   $(COVERAGE-LIB_S-TARGETS)   \
                   $(COVERAGE-EXE-TARGETS)     \
                   $(COVERAGE-CFG-TARGETS)     \
                   $(COVERAGE-PUBLISH-TARGETS) \
                   $(COVERAGE-EXP-TARGETS)     \
                   $(COVERAGE-TEST-TARGETS)    \
                   $(COVERAGE-CUNIT-TARGETS)

# Typical order: Directories, Targets, Debug-Targets
production: $(COMMON-DIR-TARGETS) $(DIR-TARGETS) $(TARGETS)
debug: $(COMMON-DIR-TARGETS) $(DEBUG-DIR-TARGETS) $(DEBUG-TARGETS)
coverage: $(COMMON-DIR-TARGETS) $(COVERAGE-DIR-TARGETS) $(COVERAGE-TARGETS)

all: production debug

test: production $(CUNIT_BASIC)
fulltest: production $(CUNIT_BASIC) $(CUNIT_FULL) 
debugtest: debug $(DEBUG_CUNIT_BASIC)
debugfulltest: debug $(DEBUG_CUNIT_BASIC) $(DEBUG_CUNIT_FULL)
coveragetest: coverage $(COVERAGE_CUNIT_BASIC)
coveragefulltest: coverage $(COVERAGE_CUNIT_BASIC) $(COVERAGE_CUNIT_FULL)

.PHONY:: production debug coverage all test fulltest coveragetest coveragefulltest debugtest debugfulltest
