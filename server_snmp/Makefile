# Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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
# SubComponent: server_snmp
#
# Created on:
#     Author:
#

PROJECT = server_snmp

# Need to have PRJDIR before include so SRCDIR, INCDIR can be set correctly
PRJDIR=$(CURDIR)
# Specify the relative path from this project directory to the root of the directory structure
ROOTREL=..

include $(ROOTREL)/server_build/common.mk

#Files in this component need to know paths for server
#(uses the path_parser.py/paths.properties mechanism)
PATH_VAR_CFLAGS=$(IMA_SVR_PATH_DEFINES)

# Add any required flags not specified in $(ROOTREL)/server_build/common.mk
IFLAGS += -I$(ROOTREL)/server_admin/src -I$(ROOTREL)/server_admin/deppkgs \
		  -I$(ROOTREL)/server_ship/include \
		  -I$(ROOTREL)/server_monitoring/src \
		  -I$(ROOTREL)/server_engine/src
CFLAGS = $(WARNFLAGS) $(PERFFLAGS) -fPIC -rdynamic -D_GNU_SOURCE -DLDAP_LIBS_DYNAMIC -std=c99
CPPFLAGS +=
LDFLAGS += -L$(ROOTREL)/server_snmp/mqtt/lib -L$(ROOTREL)/server_admin/deppkgs
LDLIBS += -ldl -lpthread -lssl -lnetsnmp -lnetsnmpagent -lnetsnmphelpers
XFLAGS +=
SHARED_FLAGS +=
WARNFLAGS = -Wall

#Tests that link against other libs
TESTLDFLAGS =
TESTLDLIBS += $(LDLIBS)

# ------------------------------------------------
# Add project specific make rules
# ------------------------------------------------

# Standalone SNMP Agent 
libismsnmp-FILES = imaSnmpMemMib.c imaSnmpMib.c imaSnmpMemStat.c \
				   imaSnmpStoreMib.c imaSnmpStoreStat.c \
				   imaSnmpEndpointMib.c imaSnmpEndpointStat.c \
				   imaSnmpCVMib.c imaSnmpCVStat.c imaSnmpJson.c \
				   imaSnmpEventMib.c imaSnmpEventCommonMib.c\
                   imaSnmpMemEventTrap.c imaSnmpMemEventCtrlMib.c imaSnmpMemEventMib.c\
                   imaSnmpStoreEventTrap.c imaSnmpStoreEventCtrlMib.c imaSnmpStoreEventMib.c\
				   imaSnmpServerMib.c imaSnmpServerStat.c  \
				   imaSnmpSubscriptionMib.c imaSnmpSubscriptionStat.c \
				   imaSnmpConnectionMib.c imaSnmpConnectionStat.c  \
				   imaSnmpTopicMib.c imaSnmpTopicStat.c \
				   imaSnmpUtils.c imaSnmpAgentInit.c

libismsnmp-LIBS = libismutil$(SO) libismadmin$(SO) libismstore$(SO) \
				  libismmonitoring$(SO) libismengine$(SO)

LIB-TARGETS += $(LIBDIR)/libismsnmp$(SO)
$(LIBDIR)/libismsnmp$(SO):  $(call objects, $(libismsnmp-FILES)) | \
							$(call libs, $(libismsnmp-LIBS))
	@echo "Make libismsnmp"
	$(call make-c-library)
	
DEBUG-LIB-TARGETS += $(DEBUG_LIBDIR)/libismsnmp$(SO)
$(DEBUG_LIBDIR)/libismsnmp$(SO):	$(call debug-objects, $(libismsnmp-FILES)) | \
									$(call debug-libs, $(libismsnmp-LIBS))
	@echo "Make debug libismsnmp"
	$(call debug-make-c-library)
	
COVERAGE-LIB-TARGETS += $(COVERAGE_LIBDIR)/libismsnmp$(SO)
$(COVERAGE_LIBDIR)/libismsnmp$(SO):	$(call coverage-objects, $(libismsnmp-FILES)) | \
									$(call coverage-libs, $(libismsnmp-LIBS))
	@echo "Make coverage libismsnmp"
	$(call coverage-make-c-library)	
	

# ------------------------------------------------
# CUNIT Targets
# ------------------------------------------------
test-LIBS =
CUNIT-TARGETS +=
DEBUG-CUNIT-TARGETS +=
COVERAGE-CUNIT-TARGETS +=
CUNIT_BASIC +=
CUNIT_FULL +=
DEBUG_CUNIT_BASIC +=
DEBUG_CUNIT_FULL +=
COVERAGE_CUNIT_BASIC +=
COVERAGE_CUNIT_FULL +=

# ------------------------------------------------
# Define order of targets (after targets defined)
# ------------------------------------------------

# Targets, Typical order: Libs, Exes, Cfgs, Publish, Experiments, Tests
TARGETS = $(EMBED-SOURCE-TARGETS) \
          $(LIB-TARGETS)     \
          $(LIB_S-TARGETS)   \
          $(CFG-TARGETS)     \
          $(PUBLISH-TARGETS) \
          $(EXP-TARGETS)


# Debug-Targets, Typical order: Libs, Exes, Cfgs, Publish, Experiments, Tests
DEBUG-TARGETS = $(EMBED-SOURCE-TARGETS)  \
                $(DEBUG-LIB-TARGETS)     \
                $(DEBUG-LIB_S-TARGETS)   \
                $(DEBUG-CFG-TARGETS)     \
                $(DEBUG-PUBLISH-TARGETS) \
                $(DEBUG-EXP-TARGETS)


# Coverage-Targets, Typical order: Libs, Exes, Cfgs, Publish, Experiments, Tests
COVERAGE-TARGETS = $(EMBED-SOURCE-TARGETS)     \
                   $(COVERAGE-LIB-TARGETS)     \
                   $(COVERAGE-LIB_S-TARGETS)   \
                   $(COVERAGE-EXE-TARGETS)     \
                   $(COVERAGE-CFG-TARGETS)     \
                   $(COVERAGE-PUBLISH-TARGETS) \
                   $(COVERAGE-EXP-TARGETS)

# Typical order: Directories, Targets, Debug-Targets
production: $(COMMON-DIR-TARGETS) $(DIR-TARGETS) $(TARGETS)
debug: $(COMMON-DIR-TARGETS) $(DEBUG-DIR-TARGETS) $(DEBUG-TARGETS)
coverage: $(COMMON-DIR-TARGETS) $(COVERAGE-DIR-TARGETS) $(COVERAGE-TARGETS)

all: $(DIR-TARGETS) production debug

# Since all objects being built depend on the extraction of the tar file, we need
# to ensure that the extraction does not happen in parallel with anything else.
.NOTPARALLEL:

test: production $(CUNIT_BASIC)
fulltest: production $(CUNIT_BASIC) $(CUNIT_FULL) 
debugtest: debug $(DEBUG_CUNIT_BASIC)
debugfulltest: debug $(DEBUG_CUNIT_BASIC) $(DEBUG_CUNIT_FULL)
coveragetest: coverage $(COVERAGE_CUNIT_BASIC)
coveragefulltest: coverage $(COVERAGE_CUNIT_BASIC) $(COVERAGE_CUNIT_FULL)

.PHONY:: production debug coverage all test fulltest coveragetest coveragefulltest debugtest debugfulltest

