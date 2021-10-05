# Copyright (c) 2013-2021 Contributors to the Eclipse Foundation
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
# SubComponent: server_ismc
#

PROJECT = server_ismc

# Need to have PRJDIR before include so SRCDIR, INCDIR can be set correctly
PRJDIR=$(CURDIR)
# Specify the relative path from this project directory to the root of the directory structure
ROOTREL=..

include $(ROOTREL)/server_build/common.mk

#CLIENT_COMMON_LIBDIR=COMMON_LIBDIR

# Additional source directories
PROTSRC=$(realpath $(ROOTREL)/server_protocol/src)
$(eval $(call add-search-directory,.c,$(PROTSRC)))
TRANSSRC=$(realpath $(ROOTREL)/server_transport/src)
$(eval $(call add-search-directory,.c,$(TRANSSRC)))
# UTILINC=$(realpath $(ROOTREL)/server_utils/include)
# $(eval $(call add-search-directory,.c,$(UTILINC)))

# Add any required flags not specified in $(ROOTREL)/server_build/common.mk
IFLAGS += -I$(PROTSRC)
CFLAGS += -DISMClient -Wdeclaration-after-statement 
CPPFLAGS +=
LDFLAGS +=
LDLIBS += -lpthread -lismutil 
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

# External protocol files (PROTSRC)
# External utils files (UTILSRC)
libismc-FILES = ismc.c message.c error.c tcpclient.c deliverythread.c \
                xid.c copyright.c

libfakeismc-FILES = ismc.c loopback_client.c message.c error.c loopback_client.c deliverythread.c xid.c \
                    copyright.c

LIB-TARGETS += $(LIBDIR)/libismc$(SO)
$(LIBDIR)/libismc$(SO): $(call objects, $(libismc-FILES))
	$(call make-c-library)

DEBUG-LIB-TARGETS += $(DEBUG_LIBDIR)/libismc$(SO)
$(DEBUG_LIBDIR)/libismc$(SO): $(call debug-objects, $(libismc-FILES))
	$(call debug-make-c-library)

COVERAGE-LIB-TARGETS += $(COVERAGE_LIBDIR)/libismc$(SO)
$(COVERAGE_LIBDIR)/libismc$(SO): $(call coverage-objects, $(libismc-FILES))
	$(call coverage-make-c-library)

FAKELIB-TARGETS += $(LIBDIR)/libfakeismc$(SO)
$(LIBDIR)/libfakeismc$(SO): $(call objects, $(libfakeismc-FILES))
	$(call make-c-library)

DEBUG-FAKELIB-TARGETS += $(DEBUG_LIBDIR)/libfakeismc$(SO)
$(DEBUG_LIBDIR)/libfakeismc$(SO): $(call debug-objects, $(libfakeismc-FILES))
	$(call debug-make-c-library)

COVERAGE-FAKELIB-TARGETS += $(COVERAGE_LIBDIR)/libfakeismc$(SO)
$(COVERAGE_LIBDIR)/libfakeismc$(SO): $(call coverage-objects, $(libfakeismc-FILES))
	$(call coverage-make-c-library)

test-LIBS = libismc$(SO)
	
cunittest-LIBS = libfakeismc$(SO) libismc$(SO)
	
CUNIT-TARGETS += $(CUNITTESTDIR)/client_test$(EXE)
$(CUNITTESTDIR)/client_test$(EXE): $(call objects, client_test.c \
                                                   select_unit_test.c \
                                                   message_unit_test.c \
                                                   api_unit_test.c \
                                                   tran_unit_test.c \
                                                   noack_unit_test.c) | \
                                   $(call libs, $(cunittest-LIBS))
	$(call build-cunit-test)

DEBUG-CUNIT-TARGETS += $(DEBUG_CUNITTESTDIR)/client_test$(EXE)
$(DEBUG_CUNITTESTDIR)/client_test$(EXE): $(call debug-objects, client_test.c \
                                                   select_unit_test.c \
                                                   message_unit_test.c \
                                                   api_unit_test.c \
                                                   tran_unit_test.c \
                                                   noack_unit_test.c) | \
                                         $(call debug-libs, $(cunittest-LIBS))

	$(call debug-build-cunit-test)

COVERAGE-CUNIT-TARGETS += $(COVERAGE_CUNITTESTDIR)/client_test$(EXE)
$(COVERAGE_CUNITTESTDIR)/client_test$(EXE): $(call coverage-objects, client_test.c \
                                                   select_unit_test.c \
                                                   message_unit_test.c \
                                                   api_unit_test.c \
                                                   tran_unit_test.c \
                                                   noack_unit_test.c) | \
                                            $(call coverage-libs, $(cunittest-LIBS))

	$(call coverage-build-cunit-test)

CUNIT_BASIC += client_test
CUNIT_FULL += client_test
client_test: $(CUNITTESTDIR)/client_test$(EXE)
	$(call run-cunit-test,$<)
.PHONY:: client_test

DEBUG_CUNIT_BASIC += debug_client_test
DEBUG_CUNIT_FULL += debug_client_test
debug_client_test: $(DEBUG_CUNITTESTDIR)/client_test$(EXE)
	$(call debug-run-cunit-test,$<)
.PHONY:: debug_client_test

COVERAGE_CUNIT_BASIC += coverage_client_test
COVERAGE_CUNIT_FULL += coverage_client_test
coverage_client_test: $(COVERAGE_CUNITTESTDIR)/client_test$(EXE)
	$(call coverage-run-cunit-test,$<)
.PHONY:: coverage_client_test

TEST-TARGETS += $(BINDIR)/commitTest$(EXE)
$(BINDIR)/commitTest$(EXE): $(call objects, commit_test.c) | \
                            $(call libs, $(test-LIBS))
	$(call build-c-test)

DEBUG-TEST-TARGETS += $(DEBUG_BINDIR)/commitTest$(EXE)
$(DEBUG_BINDIR)/commitTest$(EXE): $(call debug-objects, commit_test.c) | \
                                  $(call debug-libs, $(test-LIBS))
	$(call debug-build-c-test)

COVERAGE-TEST-TARGETS += $(COVERAGE_BINDIR)/commitTest$(EXE)
$(COVERAGE_BINDIR)/commitTest$(EXE): $(call coverage-objects, commit_test.c) | \
                                     $(call coverage-libs, $(test-LIBS))
	$(call coverage-build-c-test)

TEST-TARGETS += $(BINDIR)/topicTimerTest$(EXE)
$(BINDIR)/topicTimerTest$(EXE): $(call objects, topictimer_test.c) | \
                                $(call libs, $(test-LIBS))
	$(call build-c-test)

DEBUG-TEST-TARGETS += $(DEBUG_BINDIR)/topicTimerTest$(EXE)
$(DEBUG_BINDIR)/topicTimerTest$(EXE): $(call debug-objects, topictimer_test.c) | \
                                      $(call debug-libs, $(test-LIBS))
	$(call debug-build-c-test)

COVERAGE-TEST-TARGETS += $(COVERAGE_BINDIR)/topicTimerTest$(EXE)
$(COVERAGE_BINDIR)/topicTimerTest$(EXE): $(call coverage-objects, topictimer_test.c) | \
                                         $(call coverage-libs, $(test-LIBS))
	$(call coverage-build-c-test)

TEST-TARGETS += $(BINDIR)/topicAsyncTimerTest$(EXE)
$(BINDIR)/topicAsyncTimerTest$(EXE): $(call objects, topicasynctimer_test.c) | \
                                     $(call libs, $(test-LIBS))
	$(call build-c-test)

DEBUG-TEST-TARGETS += $(DEBUG_BINDIR)/topicAsyncTimerTest$(EXE)
$(DEBUG_BINDIR)/topicAsyncTimerTest$(EXE): $(call debug-objects, topicasynctimer_test.c) | \
                                           $(call debug-libs, $(test-LIBS))
	$(call debug-build-c-test)

COVERAGE-TEST-TARGETS += $(COVERAGE_BINDIR)/topicAsyncTimerTest$(EXE)
$(COVERAGE_BINDIR)/topicAsyncTimerTest$(EXE): $(call coverage-objects, topicasynctimer_test.c) | \
                                              $(call coverage-libs, $(test-LIBS))
	$(call coverage-build-c-test)

TEST-TARGETS += $(BINDIR)/topicListenerTest$(EXE)
$(BINDIR)/topicListenerTest$(EXE): $(call objects, topiclistener_test.c) | \
                                   $(call libs, $(test-LIBS))
	$(call build-c-test)

DEBUG-TEST-TARGETS += $(DEBUG_BINDIR)/topicListenerTest$(EXE)
$(DEBUG_BINDIR)/topicListenerTest$(EXE): $(call debug-objects, topiclistener_test.c) | \
                                         $(call debug-libs, $(test-LIBS))
	$(call debug-build-c-test)

COVERAGE-TEST-TARGETS += $(COVERAGE_BINDIR)/topicListenerTest$(EXE)
$(COVERAGE_BINDIR)/topicListenerTest$(EXE): $(call coverage-objects, topiclistener_test.c) | \
                                            $(call coverage-libs, $(test-LIBS))
	$(call coverage-build-c-test)

TEST-TARGETS += $(BINDIR)/tranTest$(EXE)
$(BINDIR)/tranTest$(EXE): $(call objects, trantest.c) | \
                          $(call libs, $(test-LIBS))
	$(call build-c-test)

DEBUG-TEST-TARGETS += $(DEBUG_BINDIR)/tranTest$(EXE)
$(DEBUG_BINDIR)/tranTest$(EXE): $(call debug-objects, trantest.c) | \
                                $(call debug-libs, $(test-LIBS))
	$(call debug-build-c-test)


TEST-TARGETS += $(BINDIR)/runserver$(EXE)
$(BINDIR)/runserver$(EXE): $(call objects, runserver.c) | \
                           $(call libs, $(test-LIBS))
	$(call build-c-test)

DEBUG-TEST-TARGETS += $(DEBUG_BINDIR)/runserver$(EXE)
$(DEBUG_BINDIR)/runserver$(EXE): $(call debug-objects, runserver.c) | \
                                 $(call debug-libs, $(test-LIBS))
	$(call debug-build-c-test)

COVERAGE-TEST-TARGETS += $(COVERAGE_BINDIR)/runserver$(EXE)
$(COVERAGE_BINDIR)/runserver$(EXE): $(call coverage-objects, runserver.c) | \
                                    $(call coverage-libs, $(test-LIBS))
	$(call coverage-build-c-test)

# ------------------------------------------------
# Define order of targets (after targets defined)
# ------------------------------------------------

# Targets, Typical order: Libs, Exes, Cfgs, Publish, Experiments, Tests
TARGETS = $(LIB-TARGETS)     \
          $(FAKELIB-TARGETS) \
          $(LIB_S-TARGETS)   \
          $(EXE-TARGETS)     \
          $(CFG-TARGETS)     \
          $(PUBLISH-TARGETS) \
          $(EXP-TARGETS)     \
          $(TEST-TARGETS)    \
          $(CUNIT-TARGETS)

# Debug-Targets, Typical order: Libs, Exes, Cfgs, Publish, Experiments, Tests
DEBUG-TARGETS = $(DEBUG-LIB-TARGETS)     \
                $(DEBUG-FAKELIB-TARGETS) \
                $(DEBUG-LIB_S-TARGETS)   \
                $(DEBUG-EXE-TARGETS)     \
                $(DEBUG-CFG-TARGETS)     \
                $(DEBUG-PUBLISH-TARGETS) \
                $(DEBUG-EXP-TARGETS)     \
                $(DEBUG-TEST-TARGETS)    \
                $(DEBUG-CUNIT-TARGETS)

# Coverage-Targets, Typical order: Libs, Exes, Cfgs, Publish, Experiments, Tests
COVERAGE-TARGETS = $(COVERAGE-LIB-TARGETS)     \
                   $(COVERAGE-FAKELIB-TARGETS) \
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