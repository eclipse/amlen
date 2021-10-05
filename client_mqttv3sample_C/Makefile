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

# Component:    client
# SubComponent: client_mqttv3sample_C
#

PROJECT = client_mqttv3sample_C

# Need to have PRJDIR before include so SRCDIR, INCDIR can be set correctly
PRJDIR=$(CURDIR)
# Specify the relative path from this project directory to the root of the directory structure
ROOTREL=..

include $(ROOTREL)/server_build/common.mk

# Add any required flags not specified in $(ROOTREL)/server_build/common.mk
IFLAGS += -I/usr/local/ssl/include
CFLAGS += -fPIC
CPPFLAGS +=
LDFLAGS += -L/usr/local/ssl/lib
#LDFLAGS += -Wl,--verbose
LDLIBS += -lpthread -lmqttv3c -lssl -lcrypto
XFLAGS +=
SHARED_FLAGS +=

COMMON_LIBDIR := linux_ia64
RUNPATH_FLAGS = -Wl,-rpath-link,linux_ia64/ -Wl,-rpath,'$${ORIGIN}/../lib64/' -Wl,-rpath,'$${ORIGIN}/../linux_ia64/'


TESTLDFLAGS = $(LDFLAGS)
TESTLDLIBS = $(LDLIBS) -L$(CLIENT_COMMON_LIBDIR)

# ------------------------------------------------
# Add project specific make rules
# ------------------------------------------------

MQTTV3Sample-FILES = MQTTV3Sample.c

EXE-TARGETS += $(BINDIR)/MQTTV3Sample$(EXE)

$(BINDIR)/MQTTV3Sample$(EXE): $(call objects, $(MQTTV3Sample-FILES)) \
                              $(call libs, $(MQTTV3Sample-LIBS))
	$(call link-c-program)

DEBUG-EXE-TARGETS += $(DEBUG_BINDIR)/MQTTV3Sample$(EXE)

$(DEBUG_BINDIR)/MQTTV3Sample$(EXE): $(call debug-objects, $(MQTTV3Sample-FILES)) \
                                    $(call debug-libs, $(MQTTV3Sample-LIBS))
	$(call debug-link-c-program)

COVERAGE-EXE-TARGETS += $(COVERAGE_BINDIR)/MQTTV3Sample$(EXE)

$(COVERAGE_BINDIR)/MQTTV3Sample$(EXE): $(call coverage-objects, $(MQTTV3Sample-FILES)) \
                                       $(call coverage-libs, $(MQTTV3Sample-LIBS))
	$(call coverage-link-c-program)

mqttsample-FILES = mqttsample.c publish.c subscribe.c connect.c utils.c

EXE-TARGETS += $(BINDIR)/mqttsample$(EXE)

$(BINDIR)/mqttsample$(EXE): $(call objects, $(mqttsample-FILES)) \
                            $(call libs, $(mqttsample-LIBS))
	$(call link-c-program)

DEBUG-EXE-TARGETS += $(DEBUG_BINDIR)/mqttsample$(EXE)

$(DEBUG_BINDIR)/mqttsample$(EXE): $(call debug-objects, $(mqttsample-FILES)) \
                                  $(call debug-libs, $(mqttsample-LIBS))
	$(call debug-link-c-program)

COVERAGE-EXE-TARGETS += $(COVERAGE_BINDIR)/mqttsample$(EXE)

$(COVERAGE_BINDIR)/mqttsample$(EXE): $(call coverage-objects, $(mqttsample-FILES)) \
                                     $(call coverage-libs, $(mqttsample-LIBS))
	$(call coverage-link-c-program)


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

strip_files: production
	strip linux_ia64/libmqttv3c.so
	strip linux_ia64/libmqttv3cs.so
	strip linux_ia64/libmqttv3a.so
	strip linux_ia64/libmqttv3as.so

# NB: all is production only, debug will not build for lack of libmqttv3c.so
all: production strip_files

test: production $(CUNIT_BASIC)
fulltest: production $(CUNIT_BASIC) $(CUNIT_FULL)
debugtest: debug $(DEBUG_CUNIT_BASIC)
debugfulltest: debug $(DEBUG_CUNIT_BASIC) $(DEBUG_CUNIT_FULL)
coveragetest: coverage $(COVERAGE_CUNIT_BASIC)
coveragefulltest: coverage $(COVERAGE_CUNIT_BASIC) $(COVERAGE_CUNIT_FULL)

.PHONY:: production debug coverage all test fulltest coveragetest coveragefulltest debugtest debugfulltest strip_files
