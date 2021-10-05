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
# Component:    server
# SubComponent: server_store
#
# Created on:
#     Author:
#

PROJECT = server_store

# Need to have PRJDIR before include so SRCDIR, INCDIR can be set correctly
PRJDIR=$(CURDIR)
# Specify the relative path from this project directory to the root of the directory structure
ROOTREL=..

PATH_VAR_CFLAGS=$(IMA_SVR_PATH_DEFINES)
BEAM_BUILD=no
include $(ROOTREL)/server_build/common.mk

# Add any required flags not specified in $(ROOTREL)/server_build/common.mk
IFLAGS += -I$(PRJDIR)/../server_transport/include

ifneq (,$(wildcard $(ROOTREL)/server_certificates_ext/include))
     IFLAGS += -I$(ROOTREL)/server_certificates_ext/include
else
     IFLAGS += -I$(ROOTREL)/server_certificates/include
endif

CFLAGS +=
CPPFLAGS +=
LDFLAGS +=
LDLIBS +=
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

libismstore-FILES = store.c \
                    storeRecovery.c \
                    storeDiskUtils.c \
                    storeHighAvailability.c \
                    storeMemoryHA.c \
                    storeMemory.c \
                    storeLockFile.c \
                    storeHAadmin.c \
                    storeShmPersist.c \
                    copyright.c

libismstore-LIBS = libismutil$(SO) libismadmin$(SO)

LIB-TARGETS += $(LIBDIR)/libismstore$(SO)
$(LIBDIR)/libismstore$(SO): $(call objects, $(libismstore-FILES)) | \
                            $(call libs, $(libismstore-LIBS))
	$(call make-c-library)

DEBUG-LIB-TARGETS += $(DEBUG_LIBDIR)/libismstore$(SO)
$(DEBUG_LIBDIR)/libismstore$(SO): $(call debug-objects, $(libismstore-FILES)) | \
                                  $(call debug-libs, $(libismstore-LIBS))
	$(call debug-make-c-library)

COVERAGE-LIB-TARGETS += $(COVERAGE_LIBDIR)/libismstore$(SO)
$(COVERAGE_LIBDIR)/libismstore$(SO): $(call coverage-objects, $(libismstore-FILES)) | \
                                     $(call coverage-libs, $(libismstore-LIBS))
	$(call coverage-make-c-library)

LIB_S-TARGETS += $(LIBDIR_S)/libismstore$(A)
$(LIBDIR_S)/libismstore$(A): $(call objects, $(libismstore-FILES)) 
	$(call make-c-static-library)

DEBUG-LIB_S-TARGETS += $(DEBUG_LIBDIR_S)/libismstore$(A)
$(DEBUG_LIBDIR_S)/libismstore$(A): $(call debug-objects, $(libismstore-FILES)) 
	$(call debug-make-c-static-library)

COVERAGE-LIB_S-TARGETS += $(COVERAGE_LIBDIR_S)/libismstore$(A)
$(COVERAGE_LIBDIR_S)/libismstore$(A): $(call coverage-objects, $(libismstore-FILES)) 
	$(call coverage-make-c-static-library)

test-LIBS = $(libismstore-LIBS) libismstore$(SO)

TEST-TARGETS += $(BINDIR)/testStoreLockFile$(EXE)
$(BINDIR)/testStoreLockFile$(EXE): $(call objects, testStoreLockFile.c) | \
                                   $(call libs, $(test-LIBS))
	$(call build-c-test)

DEBUG-TEST-TARGETS += $(DEBUG_BINDIR)/testStoreLockFile$(EXE)
$(DEBUG_BINDIR)/testStoreLockFile$(EXE): $(call debug-objects, testStoreLockFile.c) | \
                                         $(call debug-libs, $(test-LIBS))
	$(call debug-build-c-test)

COVERAGE-TEST-TARGETS += $(COVERAGE_BINDIR)/testStoreLockFile$(EXE)
$(COVERAGE_BINDIR)/testStoreLockFile$(EXE): $(call coverage-objects, testStoreLockFile.c) | \
                                            $(call coverage-libs, $(test-LIBS))
	$(call coverage-build-c-test)

TEST-TARGETS += $(BINDIR)/testDiskUtils$(EXE)
$(BINDIR)/testDiskUtils$(EXE): $(call objects, testDiskUtils.c) | \
                                   $(call libs, $(test-LIBS))
	$(call build-c-test)

DEBUG-TEST-TARGETS += $(DEBUG_BINDIR)/testDiskUtils$(EXE)
$(DEBUG_BINDIR)/testDiskUtils$(EXE): $(call debug-objects, testDiskUtils.c) | \
                                     $(call debug-libs, $(test-LIBS))
	$(call debug-build-c-test)

COVERAGE-TEST-TARGETS += $(COVERAGE_BINDIR)/testDiskUtils$(EXE)
$(COVERAGE_BINDIR)/testDiskUtils$(EXE): $(call coverage-objects, testDiskUtils.c) | \
                                        $(call coverage-libs, $(test-LIBS))
	$(call coverage-build-c-test)

TEST-TARGETS += $(BINDIR)/readStore$(EXE)
$(BINDIR)/readStore$(EXE): $(call objects, readStore.c) | \
                                   $(call libs, $(test-LIBS))
	$(call build-c-test)

DEBUG-TEST-TARGETS += $(DEBUG_BINDIR)/readStore$(EXE)
$(DEBUG_BINDIR)/readStore$(EXE): $(call debug-objects, readStore.c) | \
                                     $(call debug-libs, $(test-LIBS))
	$(call debug-build-c-test)

COVERAGE-TEST-TARGETS += $(COVERAGE_BINDIR)/readStore$(EXE)
$(COVERAGE_BINDIR)/readStore$(EXE): $(call coverage-objects, readStore.c) | \
                                        $(call coverage-libs, $(test-LIBS))
	$(call coverage-build-c-test)

CUNIT-TARGETS += $(CUNITTESTDIR)/store_test$(EXE)
$(CUNITTESTDIR)/store_test$(EXE): $(call objects, store_test.c         \
                                                  store_unit_test.c) | \
                                      $(call libs, $(test-LIBS))
	$(call build-cunit-test)

DEBUG-CUNIT-TARGETS += $(DEBUG_CUNITTESTDIR)/store_test$(EXE)
$(DEBUG_CUNITTESTDIR)/store_test$(EXE): $(call debug-objects, store_test.c         \
                                                              store_unit_test.c) | \
                                        $(call debug-libs, $(test-LIBS))
	$(call debug-build-cunit-test)

COVERAGE-CUNIT-TARGETS += $(COVERAGE_CUNITTESTDIR)/store_test$(EXE)
$(COVERAGE_CUNITTESTDIR)/store_test$(EXE): $(call coverage-objects, store_test.c         \
                                                                    store_unit_test.c) | \
                                           $(call coverage-libs, $(test-LIBS))
	$(call coverage-build-cunit-test)

CUNIT_FULL += store_test
store_test: $(CUNITTESTDIR)/store_test$(EXE)
	$(call run-cunit-test,$<)
.PHONY:: store_test

DEBUG_CUNIT_FULL += debug_store_test
debug_store_test: $(DEBUG_CUNITTESTDIR)/store_test$(EXE)
	$(call debug-run-cunit-test,$<)
.PHONY:: debug_store_test

COVERAGE_CUNIT_FULL += coverage_store_test
coverage_store_test: $(COVERAGE_CUNITTESTDIR)/store_test$(EXE)
	$(call coverage-run-cunit-test,$<,-d $(COVERAGE_TSODIR))
.PHONY:: coverage_store_test

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
