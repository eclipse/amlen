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
# SubComponent: server_protocol
#
# Created on:
#     Author:
#

PROJECT = server_protocol

# Need to have PRJDIR before include so SRCDIR, INCDIR can be set correctly
PRJDIR=$(CURDIR)
# Specify the relative path from this project directory to the root of the directory structure
ROOTREL=..

include $(ROOTREL)/server_build/common.mk

# Add any required flags not specified in $(ROOTREL)/server_build/common.mk
IDFLAGS += 
IFLAGS += -I$(ROOTREL)/server_transport/src
CFLAGS += 
CPPFLAGS +=
LDFLAGS +=
# LDLIBS += -lpthread -lismadmin -lismstore 
LDLIBS += -lpthread -lismadmin -lismstore -lMCP_Routing
XFLAGS +=
SHARED_FLAGS +=

#Files in this component need to know paths for server
#(uses the path_parser.py/paths.properties mechanism)
PATH_VAR_CFLAGS=$(IMA_SVR_PATH_DEFINES)

ifeq ($(ENABLE_DELAY),1)
CFLAGS += -DALLOW_DELAY
endif

TESTLDFLAGS += $(LDFLAGS)
TESTLDLIBS += $(LDLIBS)

# ------------------------------------------------
# Add project specific make rules
# ------------------------------------------------

libismprotocol-FILES =  rmsg.c msgid.c protocol.c wsproto.c mqtt.c iotmonitor.c jms.c plugin.c pluginAdmin.c mux.c forwarder.c fwdreceiver.c fwdsender.c copyright.c  

libismprotocol-LIBS = libismutil$(SO)      \
                      libismtransport$(SO) \
                      libismengine$(SO)    \
                      libismadmin$(SO)     \
                      libismstore$(SO)     \
                      libismmonitoring$(SO)

ifneq (,$(wildcard $(ROOTREL)/server_protocol_ext/src))
     CFLAGS += -DPROTOCOL_EXTRA_SRC
     IFLAGS += -I$(ROOTREL)/server_protocol_ext/src
     $(eval $(call add-search-directory,.c,$(ROOTREL)/server_protocol_ext/src))
     libismprotocol-FILES += protocol_extra.c
endif

LIB-TARGETS += $(LIBDIR)/libismprotocol$(SO)
$(LIBDIR)/libismprotocol$(SO): $(call objects, $(libismprotocol-FILES)) | \
                               $(call libs, $(libismprotocol-LIBS))
	$(call make-c-library)

DEBUG-LIB-TARGETS += $(DEBUG_LIBDIR)/libismprotocol$(SO)
$(DEBUG_LIBDIR)/libismprotocol$(SO): $(call debug-objects, $(libismprotocol-FILES)) | \
                                     $(call debug-libs, $(libismprotocol-LIBS))
	$(call debug-make-c-library)

COVERAGE-LIB-TARGETS += $(COVERAGE_LIBDIR)/libismprotocol$(SO)
$(COVERAGE_LIBDIR)/libismprotocol$(SO): $(call coverage-objects, $(libismprotocol-FILES)) | \
                                        $(call coverage-libs, $(libismprotocol-LIBS))
	$(call coverage-make-c-library)

LIB_S-TARGETS += $(LIBDIR_S)/libismprotocol$(A)
$(LIBDIR_S)/libismprotocol$(A): $(call objects, $(libismprotocol-FILES)) 
	$(call make-c-static-library)

DEBUG-LIB_S-TARGETS += $(DEBUG_LIBDIR_S)/libismprotocol$(A)
$(DEBUG_LIBDIR_S)/libismprotocol$(A): $(call debug-objects, $(libismprotocol-FILES)) 
	$(call debug-make-c-static-library)

COVERAGE-LIB_S-TARGETS += $(COVERAGE_LIBDIR_S)/libismprotocol$(A)
$(COVERAGE_LIBDIR_S)/libismprotocol$(A): $(call coverage-objects, $(libismprotocol-FILES)) 
	$(call coverage-make-c-static-library)

protocol_test-FILES = protocol_test.c test_jms.c test_mqtt.c xidtest.c proptest.c test_msgid.c fwdtest.c test_iotmonitor.c

test-LIBS = librum$(SO) libSpiderCast$(SO) \
            $(libismprotocol-LIBS) \
            libismprotocol$(SO)

CUNIT-TARGETS += $(CUNITTESTDIR)/protocol_test$(EXE) 
$(CUNITTESTDIR)/protocol_test$(EXE): $(call objects, $(protocol_test-FILES)) | \
                                     $(call libs, $(test-LIBS))
	$(call build-cunit-test)

DEBUG-CUNIT-TARGETS += $(DEBUG_CUNITTESTDIR)/protocol_test$(EXE)  
$(DEBUG_CUNITTESTDIR)/protocol_test$(EXE): $(call debug-objects, $(protocol_test-FILES)) | \
                                           $(call debug-libs, $(test-LIBS))
	$(call debug-build-cunit-test)

COVERAGE-CUNIT-TARGETS += $(COVERAGE_CUNITTESTDIR)/protocol_test$(EXE)  
$(COVERAGE_CUNITTESTDIR)/protocol_test$(EXE): $(call coverage-objects, $(protocol_test-FILES)) | \
                                              $(call coverage-libs, $(test-LIBS))
	$(call coverage-build-cunit-test)

CUNIT_BASIC += protocol_testBasic
protocol_testBasic: $(CUNITTESTDIR)/protocol_test$(EXE) 
	$(call run-cunit-test,$<)
.PHONY:: protocol_testBasic

DEBUG_CUNIT_BASIC += debug_protocol_testBasic
debug_protocol_testBasic: $(DEBUG_CUNITTESTDIR)/protocol_test$(EXE) 
	$(call debug-run-cunit-test,$<)
.PHONY:: debug_protocol_testBasic


DEBUG_CUNIT_FULL += debug_protocol_testFull
debug_protocol_testFull: $(DEBUG_CUNITTESTDIR)/protocol_test$(EXE)
	$(call debug-run-cunit-test,$<,FULL)
.PHONY:: debug_protocol_testFull

COVERAGE_CUNIT_FULL += coverage_protocol_testFull
coverage_protocol_testFull: $(COVERAGE_CUNITTESTDIR)/protocol_test$(EXE)
	$(call coverage-run-cunit-test,$<,FULL)
.PHONY:: coverage_protocol_testFull

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
