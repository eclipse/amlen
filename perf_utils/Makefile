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
# Component: client
# SubComponent: perf_utils
#
# Created on:
#     Author:
#

PROJECT = perf_utils

# Need to have PRJDIR before include so SRCDIR, INCDIR can be set correctly
PRJDIR=$(CURDIR)
# Specify the relative path from this project directory to the root of the directory structure
ROOTREL=..

include $(ROOTREL)/server_build/common.mk

# Source files:
SRCFILES = perfjni.c

# Add any required flags not specified in $(ROOTREL)/server_build/common.mk
IFLAGS += -I $(JAVA_HOME)/include -I $(JAVA_HOME)/include/linux
CFLAGS +=
CPPFLAGS +=
LDFLAGS += 
LDLIBS +=
XFLAGS +=
SHARED_FLAGS +=

TESTLDFLAGS = $(LDFLAGS)
TESTLDLIBS = $(LDLIBS)

# ------------------------------------------------
# Add project specific make rules
# ------------------------------------------------

libperfutils-FILES = perfjni.c

libperfutils-LIBS =  libismutil$(SO)

LIB-TARGETS += $(LIBDIR_64)/libperfutil$(SO)
$(LIBDIR_64)/libperfutil$(SO): $(call objects, $(libperfutils-FILES)) \
                               $(call libs, $(libperfutils-LIBS))
	$(call make-c-library)

DEBUG-LIB-TARGETS += $(DEBUG_LIBDIR_64)/libperfutil$(SO)
$(DEBUG_LIBDIR_64)/libperfutil$(SO): $(call debug-objects, $(libperfutils-FILES)) \
                                     $(call debug-libs, $(libperfutils-LIBS))
	$(call debug-make-c-library)

COVERAGE-LIB-TARGETS += $(COVERAGE_LIBDIR_64)/libperfutil$(SO)
$(COVERAGE_LIBDIR_64)/libperfutil$(SO): $(call coverage-objects, $(libperfutils-FILES)) \
                                        $(call coverage-libs, $(libperfutils-LIBS))
	$(call coverage-make-c-library)

# ------------------------------------------------
# Define order of targets (after targets defined)
# ------------------------------------------------

# Targets, Typical order: Libs, Exes, Cfgs, Publish, Experiments, Tests
TARGETS = $(LIB-TARGETS)               \
          $(LIB_S-TARGETS)             \
          $(EXE-TARGETS)               \
          $(CFG-TARGETS)               \
          $(CLIENT-PUBLISH-TARGETS-64) \
          $(EXP-TARGETS)               \
          $(TEST-TARGETS)              \
          $(CUNIT-TARGETS)

# Debug-Targets, Typical order: Libs, Exes, Cfgs, Publish, Experiments, Tests
DEBUG-TARGETS = $(DEBUG-LIB-TARGETS)               \
                $(DEBUG-LIB_S-TARGETS)             \
                $(DEBUG-EXE-TARGETS)               \
                $(DEBUG-CFG-TARGETS)               \
                $(DEBUG-CLIENT-PUBLISH-TARGETS-64) \
                $(DEBUG-EXP-TARGETS)               \
                $(DEBUG-TEST-TARGETS)              \
                $(DEBUG-CUNIT-TARGETS)

# Coverage-Targets, Typical order: Libs, Exes, Cfgs, Publish, Experiments, Tests
COVERAGE-TARGETS = $(COVERAGE-LIB-TARGETS)               \
                   $(COVERAGE-LIB_S-TARGETS)             \
                   $(COVERAGE-EXE-TARGETS)               \
                   $(COVERAGE-CFG-TARGETS)               \
                   $(COVERAGE-CLIENT-PUBLISH-TARGETS-64) \
                   $(COVERAGE-EXP-TARGETS)               \
                   $(COVERAGE-TEST-TARGETS)              \
                   $(COVERAGE-CUNIT-TARGETS)

# Typical order: Directories, Targets, Debug-Targets
production: $(COMMON-DIR-TARGETS) $(DIR-TARGETS) $(DIR-TARGETS-64) $(TARGETS)
debug: $(COMMON-DIR-TARGETS) $(DEBUG-DIR-TARGETS) $(DEBUG-DIR-TARGETS-64) $(DEBUG-TARGETS)
coverage: $(COMMON-DIR-TARGETS) $(COVERAGE-DIR-TARGETS) $(COVERAGE-DIR-TARGETS-64) $(COVERAGE-TARGETS)

all: production debug

test: production $(CUNIT_BASIC)
fulltest: production $(CUNIT_BASIC) $(CUNIT_FULL) 
debugtest: debug $(DEBUG_CUNIT_BASIC)
debugfulltest: debug $(DEBUG_CUNIT_BASIC) $(DEBUG_CUNIT_FULL)
coveragetest: coverage $(COVERAGE_CUNIT_BASIC)
coveragefulltest: coverage $(COVERAGE_CUNIT_BASIC) $(COVERAGE_CUNIT_FULL)

.PHONY:: production debug coverage all test fulltest coveragetest coveragefulltest debugtest debugfulltest
