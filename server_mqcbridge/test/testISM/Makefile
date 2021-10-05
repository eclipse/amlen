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
# SubComponent: mqcbridge
#
# Created on:
#     Author:
#

PROJECT = server_mqcbridge_testISM

# Need to have PRJDIR before include so SRCDIR, INCDIR can be set correctly
PRJDIR=$(CURDIR)
# Specify the relative path from this project directory to the root of the directory structure
ROOTREL=../..

include $(ROOTREL)/server_build/common.mk

# Add any required flags not specified in $(ROOTREL)/server_build/common.mk
MQ_INSTALLATION_PATH=/opt/mqm
IFLAGS += -I$(CLIENT_COMMON_INCDIR) -I$(PRJDIR)/../client_ismc/src -I$(PRJDIR)/../server_protocol/src -I$(MQ_INSTALLATION_PATH)/mqxr/SDK/clients/c/include
CFLAGS +=
CPPFLAGS +=
TESTLDFLAGS += -Wl,-rpath=$(MQ_INSTALLATION_PATH)/mqxr/SDK/clients/c/linux_ia64 -Wl,-rpath=/usr/lib64
TESTLDLIBS += -L$(MQ_INSTALLATION_PATH)/mqxr/SDK/clients/c/linux_ia64 -lmqttv3c -pthread
XFLAGS +=
SHARED_FLAGS +=

# ------------------------------------------------
# Add project specific make rules
# ------------------------------------------------

TEST-TARGETS += $(BINDIR)/ismSubTest$(EXE)
$(BINDIR)/ismSubTest$(EXE): $(call objects, ismSubTest.c)
	$(call build-c-test)
TEST-TARGETS += $(BINDIR)/ismPubTest$(EXE)
$(BINDIR)/ismPubTest$(EXE): $(call objects, ismPubTest.c)
	$(call build-c-test)

# CFG-TARGETS += $(BINDIR)/mqcbridge.cfg

DEBUG-TEST-TARGETS += $(DEBUG_BINDIR)/ismPubTest$(EXE)
$(DEBUG_BINDIR)/ismPubTest$(EXE): $(call debug-objects, ismPubTest.c)
	$(call debug-build-c-test)
DEBUG-TEST-TARGETS += $(DEBUG_BINDIR)/ismSubTest$(EXE)
$(DEBUG_BINDIR)/ismSubTest$(EXE): $(call debug-objects, ismSubTest.c)
	$(call debug-build-c-test)

# DEBUG-CFG-TARGETS += $(DEBUG_BINDIR)/mqcbridge.cfg

COVERAGE-TEST-TARGETS += $(COVERAGE_BINDIR)/ismPubTest$(EXE)
$(COVERAGE_BINDIR)/ismPubTest$(EXE): $(call coverage-objects, ismPubTest.c)
	$(call coverage-build-c-test)
COVERAGE-TEST-TARGETS += $(COVERAGE_BINDIR)/ismSubTest$(EXE)
$(COVERAGE_BINDIR)/ismSubTest$(EXE): $(call coverage-objects, ismSubTest.c)
	$(call coverage-build-c-test)

# COVERAGE-CFG-TARGETS += $(COVERAGE_BINDIR)/mqcbridge.cfg

# mqttrequest
TEST-TARGETS += $(BINDIR)/mqttrequest$(EXE)
$(BINDIR)/mqttrequest$(EXE): $(call objects, mqttrequest.c)
	$(call build-c-test)

DEBUG-TEST-TARGETS += $(DEBUG_BINDIR)/mqttrequest$(EXE)
$(DEBUG_BINDIR)/mqttrequest$(EXE): $(call debug-objects, mqttrequest.c)
	$(call debug-build-c-test)

COVERAGE-TEST-TARGETS += $(COVERAGE_BINDIR)/mqttrequest$(EXE)
$(COVERAGE_BINDIR)/mqttrequest$(EXE): $(call coverage-objects, mqttrequest.c)
	$(call coverage-build-c-test)

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
fulltest: production $(CUNIT_FULL) 
debugtest: debug $(DEBUG_CUNIT_BASIC)
debugfulltest: debug $(DEBUG_CUNIT_FULL)
coveragetest: coverage $(COVERAGE_CUNIT_BASIC)
coveragefulltest: coverage $(COVERAGE_CUNIT_FULL)

.PHONY:: production debug coverage all test fulltest coveragetest coveragefulltest debugtest debugfulltest