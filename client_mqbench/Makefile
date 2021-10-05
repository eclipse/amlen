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
# SubComponent: client_mqbench
#

PROJECT = client_mqbench

# Need to have PRJDIR before include so SRCDIR, INCDIR can be set correctly
PRJDIR=$(CURDIR)
# Specify the relative path from this project directory to the root of the directory structure
ROOTREL=..

include $(ROOTREL)/server_build/common.mk

# Add any required flags not specified in $(ROOTREL)/server_build/common.mk
#Wait for ismutil makefile and build machine to be updated
IFLAGS += -I/usr/local/ssl/include -I/opt/mqm/inc 
CFLAGS += -DAMQ_UNIX -DLINUX -D_XOPENS_SOURCE
CPPFLAGS +=
#Wait for ismutil makefile and build machine to be updated
LDFLAGS += -L/usr/local/ssl/lib -L/opt/mqm/lib64
LDLIBS += -lssl -lcrypto -ldl -lmqm_r 
XFLAGS +=
SHARED_FLAGS +=

TESTLDFLAGS = 
TESTLDLIBS = 

# ------------------------------------------------
# Add project specific make rules
# ------------------------------------------------

mqbench-FILES   = cphArrayList.c           \
                  cphListIterator.c        \
                  cphThread.c              \
                  cphTrace.c               \
                  cphLog.c                 \
                  cphConfig.c              \
                  cphPublisher.c           \
                  cphSubscriber.c          \
                  cphDummy.c               \
                  cphUtil.c                \
                  cphControlThread.c       \
                  cphStringBuffer.c        \
                  cphWorkerThread.c        \
                  cphSpinLock.c            \
                  cphBundle.c              \
                  cphConnection.c          \
                  cphDestinationFactory.c  \
                  cphNameVal.c             \
                  cphMQSplitter.c          \
                  cphSender.c              \
                  cphReceiver.c            \
                  cphPutGet.c              \
                  cphRequester.c           \
                  cphResponder.c           \
                  cph.c

srvutils-LIBS =  libismutil$(SO)

EXE-TARGETS += $(BINDIR)/mqbench$(EXE) 
$(BINDIR)/mqbench$(EXE): $(call objects, $(mqbench-FILES)) \
                         $(call libs, $(srvutils-LIBS))
	$(call link-c-program)

DEBUG-EXE-TARGETS += $(DEBUG_BINDIR)/mqbench$(EXE)
$(DEBUG_BINDIR)/mqbench$(EXE): $(call debug-objects, $(mqbench-FILES)) \
                               $(call debug-libs, $(srvutils-LIBS))
	$(call debug-link-c-program)

COVERAGE-EXE-TARGETS += $(COVERAGE_BINDIR)/mqbench$(EXE)
$(COVERAGE_BINDIR)/mqbench$(EXE): $(call coverage-objects, $(mqbench-FILES)) \
                                  $(call coverage-libs, $(srvutils-LIBS))
	$(call coverage-link-c-program)

test-LIBS = 

CUNIT-TARGETS +=
CUNIT_BASIC +=
CUNIT_FULL +=

#CUNIT-TARGETS += $(CUNITTESTDIR)/client_test$(EXE)
#$(CUNITTESTDIR)/client_test$(EXE): $(call objects, client_test.c) \
#                                   $(call client-libs, $(test-LIBS)) 
#	$(call build-cunit-test)
#
#CUNIT_BASIC += client_test
#CUNIT_FULL += client_test
#client_test: $(CUNITTESTDIR)/client_test$(EXE)
#	$(call run-cunit-test,$<)
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
