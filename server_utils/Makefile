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
# Component: server
# SubComponent: server_utils
#
# Created on:
#     Author:
#

PROJECT = server_utils

# Need to have PRJDIR before include so SRCDIR, INCDIR can be set correctly
PRJDIR=$(CURDIR)
# Specify the relative path from this project directory to the root of the directory structure
ROOTREL=..
ifneq ($(SSL_HOME),)
SSL_INCLUDE=-I$(SSL_HOME)/include
SSL_LIB= -L$(SSL_HOME)/lib
else
SSL_INCLUDE=
endif

#We need to know paths for server AND bridge
#(uses the path_parser.py/paths.properties mechanism)
PATH_VAR_CFLAGS=$(IMA_SVR_AND_BRIDGE_PATH_DEFINES)

include $(ROOTREL)/server_build/common.mk

$(eval $(call add-search-directory,.c,$(SRCDIR)))
$(eval $(call add-search-directory,.cpp,$(SRCDIR)))
$(eval $(call add-search-directory,.c,$(ROOTREL)/server_snmp/src))
$(eval $(call add-search-directory,.c,$(SRCDIR)/../test))

# Add any required flags not specified in $(ROOTREL)/server_build/common.mk
IDFLAGS +=
IFLAGS += $(SSL_INCLUDE) -I$(ROOTREL)/server_admin/include
IFLAGS += -I$(ROOTREL)/server_monitoring/include -I$(PRJDIR)/../server_transport/include -I$(PRJDIR)/../server_snmp/src
CFLAGS +=
#CFLAGS += -DSSL_MALLOC_DEBUG
#CFLAGS += -DREMOVE_EXTRA_CONSISTENCY_CHECKING
CPPFLAGS +=
LDFLAGS +=
#LDLIBS += -ldl -licuuc $(SSL_LIB) -lssl  -lcrypto
LDLIBS += -ldl -licuuc -licui18n -licudata -lcurl
XFLAGS +=
SHARED_FLAGS +=

TESTLDFLAGS += $(LDFLAGS)
TESTLDLIBS += $(LDLIBS)

# ------------------------------------------------
# Add project specific make rules
# ------------------------------------------------

libismutil-FILES = execinfo.c      \
                   ismutil.c       \
                   properties.c    \
                   server.c        \
                   time.c          \
                   timezone.cpp    \
                   map.c           \
                   array.c         \
                   perfstat.c      \
                   timer.c	       \
                   ismlist.c	   \
                   logformat.c	   \
                   logwriter.c	   \
                   bufferpool.c	   \
                   global.c        \
                   ssl.c           \
                   tls.c           \
                   json.c          \
                   selector.c      \
                   filter.c        \
                   content.c       \
                   monserialization.c        \
                   xmlparser.c	   \
                   ismclient.c     \
                   throttle.c      \
                   simpleServer.c  \
                   trace.c         \
                   commonMemHandler.c \
                   sasl_scram.c \
                   copyright.c

CUNITFILES = testUtilCUnit.c testTLS.c testProtoex.c testTimerCUnit.c \
    testLoggingCUnit.c testLinkedListCUnit.c testHashMapCUnit.c testArrayCUnit.c testTracingCUnit.c \
    testTimeUtilsCUnit.c testUtilsOtherFuncsCUnit.c testGlobalCUnit.c testDelayCUnit.c testBufferPoolCUnit.c \
    test_utils_file.c test_utils_log.c testUtilsMemHandlerCUnit.c testSASLScram.c testFilterCUnit.c

LIB-TARGETS += $(LIBDIR)/libismutil$(SO)
$(LIBDIR)/libismutil$(SO): $(call objects, $(libismutil-FILES))
	echo label=$(BUILD_LABEL) id=$(BUILD_ID) version=$(ISM_VERSION) FIPS_BUILD=$(FI-PS_BUILD) CC=$(CC) FIPSLD_CC=$(FIPSLD_CC)
	$(call make-c-library)

LIB_S-TARGETS += $(LIBDIR_S)/libismutil$(A)
$(LIBDIR_S)/libismutil$(A): $(call objects, $(libismutil-FILES))
	echo label=$(BUILD_LABEL) id=$(BUILD_ID) version=$(ISM_VERSION) FIPS_BUILD=$(FIPS_BUILD) CC=$(CC) FIPSLD_CC=$(FIPSLD_CC)
	$(call make-c-static-library)

DEBUG-LIB-TARGETS += $(DEBUG_LIBDIR)/libismutil$(SO)
$(DEBUG_LIBDIR)/libismutil$(SO): $(call debug-objects, $(libismutil-FILES))
	$(call debug-make-c-library)

DEBUG-LIB_S-TARGETS += $(DEBUG_LIBDIR_S)/libismutil$(A)
$(DEBUG_LIBDIR_S)/libismutil$(A): $(call debug-objects, $(libismutil-FILES))
	$(call debug-make-c-static-library)

COVERAGE-LIB-TARGETS += $(COVERAGE_LIBDIR)/libismutil$(SO)
$(COVERAGE_LIBDIR)/libismutil$(SO): $(call coverage-objects, $(libismutil-FILES))
	$(call coverage-make-c-library)

COVERAGE-LIB_S-TARGETS += $(COVERAGE_LIBDIR_S)/libismutil$(A)
$(COVERAGE_LIBDIR_S)/libismutil$(A): $(call coverage-objects, $(libismutil-FILES))
	$(call coverage-make-c-static-library)

LDLIBS += $(SSL_LIB) -lssl  -lcrypto

icu_gettext-FILES = gettext.c \
                    copyright.c

icu_gettext-LIBS = libismutil$(SO)

icu_gettext-LIBS_S = libismutil$(A)

EXE-TARGETS += $(BINDIR)/icu_gettext$(EXE)
$(BINDIR)/icu_gettext$(EXE): $(call objects, $(icu_gettext-FILES)) | \
                             $(call libs, $(icu_gettext-LIBS))
	$(call link-c-program)

DEBUG-EXE-TARGETS += $(DEBUG_BINDIR)/icu_gettext$(EXE)
$(DEBUG_BINDIR)/icu_gettext$(EXE): $(call debug-objects, $(icu_gettext-FILES)) | \
                                   $(call debug-libs, $(icu_gettext-LIBS))
	$(call debug-link-c-program)

COVERAGE-EXE-TARGETS += $(COVERAGE_BINDIR)/icu_gettext$(EXE)
$(COVERAGE_BINDIR)/icu_gettext$(EXE): $(call coverage-objects, $(icu_gettext-FILES)) | \
                                      $(call coverage-libs, $(icu_gettext-LIBS))
	$(call coverage-link-c-program)

test-LIBS = libismutil$(SO)

TEST-TARGETS += $(BINDIR)/testHashMap$(EXE)
$(BINDIR)/testHashMap$(EXE): $(call objects, testHashMap.c) | \
                             $(call libs, $(test-LIBS))
	$(call build-c-test)

COVERAGE-TEST-TARGETS += $(COVERAGE_BINDIR)/testHashMap$(EXE)
$(COVERAGE_BINDIR)/testHashMap$(EXE): $(call coverage-objects, testHashMap.c) | \
    $(call coverage-libs, $(test-LIBS))
	$(call coverage-build-c-test)

CUNIT-TARGETS += $(CUNITTESTDIR)/testUtilCUnit$(EXE)
$(CUNITTESTDIR)/testUtilCUnit$(EXE): $(call objects, $(CUNITFILES)) | \
                                     $(call libs, $(test-LIBS))
	$(call build-cunit-test)

DEBUG-CUNIT-TARGETS += $(DEBUG_CUNITTESTDIR)/testUtilCUnit$(EXE)
$(DEBUG_CUNITTESTDIR)/testUtilCUnit$(EXE): $(call debug-objects, $(CUNITFILES)) |  \
                                           $(call debug-libs, $(test-LIBS))
	$(call debug-build-cunit-test)

COVERAGE-CUNIT-TARGETS += $(COVERAGE_CUNITTESTDIR)/testUtilCUnit$(EXE)
$(COVERAGE_CUNITTESTDIR)/testUtilCUnit$(EXE): $(call coverage-objects, $(CUNITFILES)) | \
                                              $(call coverage-libs, $(test-LIBS))
	$(call coverage-build-cunit-test)

CUNIT_BASIC += testUtilCUnitBasic
testUtilCUnitBasic: $(CUNITTESTDIR)/testUtilCUnit$(EXE)
	$(call run-cunit-test,$<)
.PHONY:: testUtilCUnitBasic

DEBUG_CUNIT_BASIC += debug_testUtilCUnitBasic
debug_testUtilCUnitBasic: $(DEBUG_CUNITTESTDIR)/testUtilCUnit$(EXE)
	$(call debug-run-cunit-test,$<)
.PHONY:: debug_testUtilCUnitBasic

COVERAGE_CUNIT_BASIC += coverage_testUtilCUnitBasic
coverage_testUtilCUnitBasic: $(COVERAGE_CUNITTESTDIR)/testUtilCUnit$(EXE)
	$(call coverage-run-cunit-test,$<)
.PHONY:: coverage_testUtilCUnitBasic

CUNIT_FULL += testUtilCUnitFull
testUtilCUnitFull: $(CUNITTESTDIR)/testUtilCUnit$(EXE)
	$(call run-cunit-test,$<,FULL)
.PHONY:: testUtilCUnitFull

DEBUG_CUNIT_FULL += debug_testUtilCUnitFull
debug_testUtilCUnitFull: $(DEBUG_CUNITTESTDIR)/testUtilCUnit$(EXE)
	$(call debug-run-cunit-test,$<,FULL)
.PHONY:: debug_testUtilCUnitFull

COVERAGE_CUNIT_FULL += coverage_testUtilCUnitFull
coverage_testUtilCUnitFull: $(COVERAGE_CUNITTESTDIR)/testUtilCUnit$(EXE)
	$(call coverage-run-cunit-test,$<,FULL)
.PHONY:: coverage_testUtilCUnitFull

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
fulltest: production $(CUNIT_BASIC)
moretest: production $(CUNIT_FULL)
debugtest: debug $(DEBUG_CUNIT_BASIC)
debugfulltest: debug $(DEBUG_CUNIT_BASIC)
debugmoretest: debug $(DEBUG_CUNIT_FULL)
coveragetest: coverage $(COVERAGE_CUNIT_BASIC)
coveragefulltest: coverage $(COVERAGE_CUNIT_BASIC) $(COVERAGE_CUNIT_FULL)

.PHONY:: production debug coverage all test fulltest coveragetest coveragefulltest debugtest debugfulltest
