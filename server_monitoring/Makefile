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
# SubComponent: server_monitoring
#
# Created on:
#     Author:
#

PROJECT = server_monitoring

# Need to have PRJDIR before include so SRCDIR, INCDIR can be set correctly
PRJDIR=$(CURDIR)
# Specify the relative path from this project directory to the root of the directory structure
ROOTREL=..

include $(ROOTREL)/server_build/common.mk

# Add any required flags not specified in $(ROOTREL)/server_build/common.mk
IFLAGS += -I$(ROOTREL)/server_admin/deppkgs -I$(ROOTREL)/server_cluster/include
CFLAGS +=
CPPFLAGS +=
LDFLAGS +=
LDLIBS +=
XFLAGS +=
SHARED_FLAGS +=

#Tests that link against other libs
TESTLDFLAGS += $(LDFLAGS)
TESTLDLIBS += $(LDLIBS)

#Files in this component need to know paths for server
#(uses the path_parser.py/paths.properties mechanism)
PATH_VAR_CFLAGS=$(IMA_SVR_PATH_DEFINES)

# ------------------------------------------------
# Add project specific make rules
# ------------------------------------------------

libismmonitoring-FILES = \
    monitoring.c \
    restApi.c \
    monitoringutil.c \
    connectionMonData.c \
    engineMonData.c \
    endpointMonData.c \
    storeMonData.c \
    engineAdvancedPD.c \
    monitoringevent.c \
    monitoringsnapshot.c \
    monitoringSnmpTrap.c \
    removeClusterMember.c \
    forwarderMonData.c \
    copyright.c

libismmonitoring-LIBS = libismutil$(SO) libismadmin$(SO) libismstore$(SO) libismtransport$(SO) libismengine$(SO)

LIB-TARGETS += $(LIBDIR)/libismmonitoring$(SO)
$(LIBDIR)/libismmonitoring$(SO): $(call objects, $(libismmonitoring-FILES)) | \
                                 $(call libs, $(libismmonitoring-LIBS))
	$(call make-c-library)

DEBUG-LIB-TARGETS += $(DEBUG_LIBDIR)/libismmonitoring$(SO)
$(DEBUG_LIBDIR)/libismmonitoring$(SO): $(call debug-objects, $(libismmonitoring-FILES)) | \
                                       $(call debug-libs, $(libismmonitoring-LIBS))
	$(call debug-make-c-library)

COVERAGE-LIB-TARGETS += $(COVERAGE_LIBDIR)/libismmonitoring$(SO)
$(COVERAGE_LIBDIR)/libismmonitoring$(SO): $(call coverage-objects, $(libismmonitoring-FILES)) | \
                                          $(call coverage-libs, $(libismmonitoring-LIBS))
	$(call coverage-make-c-library)

LIB_S-TARGETS += $(LIBDIR_S)/libismmonitoring$(A)
$(LIBDIR_S)/libismmonitoring$(A): $(call objects, $(libismmonitoring-FILES))
	$(call make-c-static-library)

DEBUG-LIB_S-TARGETS += $(DEBUG_LIBDIR_S)/libismmonitoring$(A)
$(DEBUG_LIBDIR_S)/libismmonitoring$(A):  $(call debug-objects, $(libismmonitoring-FILES)) 
	$(call debug-make-c-static-library)

COVERAGE-LIB_S-TARGETS += $(COVERAGE_LIBDIR_S)/libismmonitoring$(A)
$(COVERAGE_LIBDIR_S)/libismmonitoring$(A):  $(call coverage-objects, $(libismmonitoring-FILES)) 
	$(call coverage-make-c-static-library)

test-LIBS = librum$(SO) libSpiderCast$(SO) libMCP_Routing$(SO) \
            $(libismmonitoring-LIBS) \
            libismmonitoring$(SO)

CUNIT-TARGETS += $(CUNITTESTDIR)/monitoring_test$(EXE)
$(CUNITTESTDIR)/monitoring_test$(EXE): $(call objects, monitoring_test.c) | \
                                       $(call libs, $(test-LIBS))
	$(call build-cunit-test)

DEBUG-CUNIT-TARGETS += $(DEBUG_CUNITTESTDIR)/monitoring_test$(EXE)
$(DEBUG_CUNITTESTDIR)/monitoring_test$(EXE): $(call debug-objects, monitoring_test.c) | \
                                             $(call debug-libs, $(test-LIBS))
	$(call debug-build-cunit-test)

COVERAGE-CUNIT-TARGETS += $(COVERAGE_CUNITTESTDIR)/monitoring_test$(EXE)
$(COVERAGE_CUNITTESTDIR)/monitoring_test$(EXE): $(call coverage-objects, monitoring_test.c) | \
                                                $(call coverage-libs, $(test-LIBS))
	$(call coverage-build-cunit-test)

CUNIT_BASIC += monitoring_testBasic
monitoring_testBasic: $(CUNITTESTDIR)/monitoring_test$(EXE)
	$(call run-cunit-test,$<)

CUNIT_FULL += monitoring_testFull
monitoring_testFull: $(CUNITTESTDIR)/monitoring_test$(EXE)
	$(call run-cunit-test,$<,FULL)

DEBUG_CUNIT_BASIC += debug_monitoring_testBasic
debug_monitoring_testBasic: $(DEBUG_CUNITTESTDIR)/monitoring_test$(EXE)
	$(call debug-run-cunit-test,$<)

DEBUG_CUNIT_FULL += debug_monitoring_testFull
debug_monitoring_testFull: $(DEBUG_CUNITTESTDIR)/monitoring_test$(EXE)
	$(call debug-run-cunit-test,$<,FULL)

COVERAGE_CUNIT_BASIC += coverage_monitoring_testBasic
coverage_monitoring_testBasic: $(COVERAGE_CUNITTESTDIR)/monitoring_test$(EXE)
	$(call coverage-run-cunit-test,$<)

COVERAGE_CUNIT_FULL += coverage_monitoring_testFull
coverage_monitoring_testFull: $(COVERAGE_CUNITTESTDIR)/monitoring_test$(EXE)
	$(call coverage-run-cunit-test,$<,FULL)


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