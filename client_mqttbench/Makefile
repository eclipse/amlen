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
# Component:    client
# SubComponent: client_mqttbench
#

PROJECT = client_mqttbench

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
include $(ROOTREL)/server_build/common.mk

# Add any required flags not specified in $(ROOTREL)/server_build/common.mk
#Wait for ismutil makefile and build machine to be updated
IFLAGS += $(SSL_INCLUDE)
CFLAGS +=
CPPFLAGS +=
#Wait for ismutil makefile and build machine to be updated
LDFLAGS +=
LDLIBS += $(SSL_LIB) -lssl -lcrypto -ldl
XFLAGS +=
SHARED_FLAGS +=

TESTLDFLAGS =
TESTLDLIBS =

# ------------------------------------------------
# Add project specific make rules
# ------------------------------------------------
mqttbench-FILES = commandline.c      \
                  env.c              \
                  main.c             \
                  mbconfig.c         \
                  mqttbenchlatency.c \
                  mqttbenchrate.c    \
                  mqttbenchutil.c    \
                  mqttbench.c        \
                  mqttclient.c       \
                  mqttpacket.c       \
                  tcpclient.c        \
                  terminate.c        \
                  timertasks.c

srvutils-LIBS =  libismutil$(SO)

EXE-TARGETS += $(BINDIR)/mqttbench$(EXE)
$(BINDIR)/mqttbench$(EXE): $(call objects, $(mqttbench-FILES)) \
                           $(call libs, $(srvutils-LIBS))
	$(call link-c-program)

DEBUG-EXE-TARGETS += $(DEBUG_BINDIR)/mqttbench$(EXE)
$(DEBUG_BINDIR)/mqttbench$(EXE): $(call debug-objects, $(mqttbench-FILES)) \
                                 $(call debug-libs, $(srvutils-LIBS))
	$(call debug-link-c-program)

COVERAGE-EXE-TARGETS += $(COVERAGE_BINDIR)/mqttbench$(EXE)
$(COVERAGE_BINDIR)/mqttbench$(EXE): $(call coverage-objects, $(mqttbench-FILES)) \
                                    $(call coverage-libs, $(srvutils-LIBS))
	$(call coverage-link-c-program)

test-LIBS =

CUNIT-TARGETS +=
CUNIT_BASIC +=
CUNIT_FULL +=

#CUNIT-TARGETS += $(CUNITTESTDIR)/client_test$(EXE)
#$(CUNITTESTDIR)/client_test$(EXE): $(call objects, client_test.c) \
#                                   $(call client-libs, $(test-LIBS))
#       $(call build-cunit-test)
#
#CUNIT_BASIC += client_test
#CUNIT_FULL += client_test
#client_test: $(CUNITTESTDIR)/client_test$(EXE)
#       $(call run-cunit-test,$<)
#.PHONY:: client_test

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
