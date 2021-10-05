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

PROJECT = server_mqcbridge

# Need to have PRJDIR before include so SRCDIR, INCDIR can be set correctly
PRJDIR=$(CURDIR)
# Specify the relative path from this project directory to the root of the directory structure
ROOTREL=..

include $(ROOTREL)/server_build/common.mk

CFLAGS += -DADD_USERNAME_AND_PASSWORD_SUPPORT

#This build requires an MQ client
#
# The client files can either be pre-extracted (so set the MQC_HOME var to e.g. /opt/mqm) or
# this Makefile can can get the bits it needs directly from the tarred redistributable client file
# (set $MQ_CLIENT_TGZ to the file location -or it assumed to be in in $DEPS_HOME if that is set
#  otherwise it defaults to $BROOT/mqfiles)
# If they are prextracted, $MQC_HOME/samp/xatm/xa.h needs to be copied to $MQC_HOME/inc

ifeq ($(MQC_HOME),)
ifeq ($(MQ_CLIENT_TGZ),)
ifeq ($(DEPS_HOME),)
MQC_TGZ_DIR=$(OUTDIR)/$(ROOTREL)/mqfiles
else
MQC_TGZ_DIR=$(DEPS_HOME)/mqc
endif
#Location is the .tar.gz file or the directory that should have held it
MQ_CLIENT_LOCATION=$(MQC_TGZ_DIR)/$(shell ls -1tr $(MQC_TGZ_DIR) | grep MQ | grep 'tar.gz'| tail -n 1)
MQ_CLIENT_TGZ=$(shell echo $(MQ_CLIENT_LOCATION)| grep 'tar.gz')
#MQ_CLIENT_TGZ=$(OUTDIR)/$(ROOTREL)/mqfiles/9.0.0.8-IBM-MQC-LinuxX64.tar.gz
endif
ifeq (,$(wildcard $(MQ_CLIENT_TGZ)))
$(error Cannot find MQ Redistributable client $(MQ_CLIENT_TGZ) - Looking in $$DEPS_HOME/mqc or $(OUTDIR)/$(ROOTREL)/mqfiles)
endif
#We have a client tar.gz that we need to extract...make everything depend
#on a header file that we extract from the client
MQC=$(OUTDIR)/mqc_extract
$(BINDIR)/mqcbridge$(EXE)::$(MQC)/inc/cmqc.h
$(DEBUG_BINDIR)/mqcbridge$(EXE)::$(MQC)/inc/cmqc.h
$(COVERAGE_BINDIR)/mqcbridge$(EXE)::$(MQC)/inc/cmqc.h
$(LIBDIR)/libtest_mqcbridge$(SO)::$(MQC)/inc/cmqc.h
$(DEBUG_LIBDIR)/libtest_mqcbridge$(SO)::$(MQC)/inc/cmqc.h
$(COVERAGE_LIBDIR)/libtest_mqcbridge$(SO)::$(MQC)/inc/cmqc.h
else
MQC=$(MQC_HOME)
endif

# Include the mqfiles include directory
IFLAGS += -I$(PRJDIR)/mqfiles/include -I$(MQC)/inc
$(eval $(call add-search-directory,.c,$(PRJDIR)/mqfiles/include))

# Include the MQ test program source directory
IFLAGS += -I$(PRJDIR)/test/testMQ -I$(PRJDIR)/test/testIMSC -I$(PRJDIR)/test/testISM
$(eval $(call add-search-directory,.c,$(PRJDIR)/testMQ))

# Add any required flags not specified in $(ROOTREL)/server_build/common.mk
IFLAGS += -I$(CLIENT_COMMON_INCDIR) -I$(PRJDIR)/../server_ismc/src -I$(PRJDIR)/../server_protocol/src -I$(PRJDIR)/../server_utils/src
CFLAGS +=
CPPFLAGS +=
LDFLAGS +=
LDLIBS += -L$(MQC)/lib64 -lmqdc_r -lmqiz_r -lmqmcs_r -lmqxzu_r -ldl -lssl -lcrypto -Wl,-rpath,\$$ORIGIN/../mqclient/lib64
XFLAGS +=
SHARED_FLAGS +=
TESTLDFLAGS += -Wl,-rpath=$(MQ_INSTALLATION_PATH)/lib64 -Wl,-rpath=/usr/lib64 -Wl,-rpath=$(MQC)/lib64
TESTLDLIBS += -L$(MQ_INSTALLATION_PATH)/lib64 -L$(MQC)/lib64 -lmqdc_r -lmqiz_r -lmqmcs_r -lmqxzu_r -lstdc++


#Files in this component need to know paths for server
#(uses the path_parser.py/paths.properties mechanism)
PATH_VAR_CFLAGS=$(IMA_SVR_PATH_DEFINES)

# ------------------------------------------------
# Add project specific make rules
# ------------------------------------------------

mqcbridge-FILES = main.c          \
                  mqcAdmin.c      \
                  control.c       \
                  mqapi.c         \
                  mqfix.c         \
                  ismcapi.c       \
                  xathread.c      \
                  reconnect.c     \
                  error.c         \
                  properties.c    \
                  uid.c           \
                  monitor.c       \
                  copyright.c

mqcbridge-LIBS = libismutil$(SO) libismadmin$(SO) libismc$(SO)

ifneq (,$(wildcard $(ROOTREL)/server_mqcbridge_ext/src))
     CFLAGS += -DMQBRIDGE_EXTRA_SRC
     IFLAGS += -I$(ROOTREL)/server_mqcbridge_ext/src
     $(eval $(call add-search-directory,.c,$(ROOTREL)/server_mqcbridge_ext/src))
     mqcbridge-FILES += mqextra.c
endif

EXE-TARGETS += $(BINDIR)/mqcbridge$(EXE)
$(BINDIR)/mqcbridge$(EXE):: $(call objects, $(mqcbridge-FILES)) | \
                           $(call libs, $(mqcbridge-LIBS))
	$(call link-c-program)

CFG-TARGETS += $(BINDIR)/mqcbridge.cfg

DEBUG-EXE-TARGETS += $(DEBUG_BINDIR)/mqcbridge$(EXE)
$(DEBUG_BINDIR)/mqcbridge$(EXE):: $(call debug-objects, $(mqcbridge-FILES)) | \
                                 $(call debug-libs, $(mqcbridge-LIBS))
	$(call debug-link-c-program)

DEBUG-CFG-TARGETS += $(DEBUG_BINDIR)/mqcbridge.cfg

COVERAGE-EXE-TARGETS += $(COVERAGE_BINDIR)/mqcbridge$(EXE)
$(COVERAGE_BINDIR)/mqcbridge$(EXE):: $(call coverage-objects, $(mqcbridge-FILES)) | \
                                    $(call coverage-libs, $(mqcbridge-LIBS))
	$(call coverage-link-c-program)

# ----------------------------------------------------------------------
# Build library version of the mqcbridge code for unit tests to use
# ----------------------------------------------------------------------
LIB-TARGETS += $(LIBDIR)/libtest_mqcbridge$(SO)
$(LIBDIR)/libtest_mqcbridge$(SO):: $(call objects, $(mqcbridge-FILES)) | \
                                  $(call libs, $(mqcbridge-LIBS))
	$(call make-c-library)

DEBUG-LIB-TARGETS += $(DEBUG_LIBDIR)/libtest_mqcbridge$(SO)
$(DEBUG_LIBDIR)/libtest_mqcbridge$(SO):: $(call debug-objects, $(mqcbridge-FILES)) | \
                                        $(call debug-libs, $(mqcbridge-LIBS))
	$(call debug-make-c-library)

COVERAGE-LIB-TARGETS += $(COVERAGE_LIBDIR)/libtest_mqcbridge$(SO)
$(COVERAGE_LIBDIR)/libtest_mqcbridge$(SO):: $(call coverage-objects, $(mqcbridge-FILES)) | \
                                           $(call coverage-libs, $(mqcbridge-LIBS))
	$(call coverage-make-c-library)

COVERAGE-CFG-TARGETS += $(COVERAGE_BINDIR)/mqcbridge.cfg

# ------------------------------------------------
# Build the unit tests
# ------------------------------------------------
test-LIBS = libtest_mqcbridge$(SO) libismc$(SO)

# ------------------------------------------------
# BASETEST included in base automated test (make test)
# FULLTEST included in full automated test (make fulltest)
# PRIVTEST private, not automated test (run manually)
# DISABLED temporarily removed from automated test (run manually)
# ------------------------------------------------
BASETEST = prod debug coverage
FULLTEST = prod debug coverage full
PRIVTEST = prod debug coverage manual
DISABLED = prod debug coverage manual

#$(eval $(call build-c-tests, $(PRIVTEST), sendx, sendx.c, $(test-LIBS)))
#$(eval $(call build-c-tests, $(PRIVTEST), queue, queue.c, $(test-LIBS)))
#$(eval $(call build-c-tests, $(PRIVTEST), qmxid, qmxid.c, $(test-LIBS)))
#$(eval $(call build-c-tests, $(PRIVTEST), xidRestart, xidRestart.c, $(test-LIBS)))
#$(eval $(call build-c-tests, $(PRIVTEST), xidStore, xidStore.c, $(test-LIBS)))
#$(eval $(call build-c-tests, $(PRIVTEST), xa, xa.c, $(test-LIBS)))
#$(eval $(call build-c-tests, $(PRIVTEST), xa2, xa2.c, $(test-LIBS)))
#$(eval $(call build-c-tests, $(PRIVTEST), rollback, rollback.c, $(test-LIBS)))
#$(eval $(call build-c-tests, $(PRIVTEST), nprollback, nprollback.c, $(test-LIBS)))

# testMQ tests
#$(eval $(call build-c-tests, $(PRIVTEST), testMQConn, testMQConn.c, $(test-LIBS)))
#$(eval $(call build-c-tests, $(PRIVTEST), mqSubTest, mqSubTest.c, $(test-LIBS)))
#$(eval $(call build-c-tests, $(PRIVTEST), mqPubTest, mqPubTest.c, $(test-LIBS)))
#$(eval $(call build-c-tests, $(PRIVTEST), mqresponse, mqresponse.c, $(test-LIBS)))
#$(eval $(call build-c-tests, $(PRIVTEST), mqreply, mqreply.c, $(test-LIBS)))

# ------------------------------------------------
# Define order of targets (after targets defined)
# ------------------------------------------------

# Targets, Typical order: Libs, Exes, Cfgs, Publish, Experiments, Tests
TARGETS = $(LIB-TARGETS)        \
          $(LIB_S-TARGETS)      \
          $(EXE-TARGETS)        \
          $(CFG-TARGETS)        \
          $(PUBLISH-TARGETS)    \
          $(EXP-TARGETS)        \
          $(TEST-TARGETS)       \
          $(TEST-TARGETS-FULL)  \
          $(CUNIT-TARGETS)      \
          $(CUNIT-TARGETS-FULL)

# Debug-Targets, Typical order: Libs, Exes, Cfgs, Publish, Experiments, Tests
DEBUG-TARGETS = $(DEBUG-LIB-TARGETS)        \
                $(DEBUG-LIB_S-TARGETS)      \
                $(DEBUG-EXE-TARGETS)        \
                $(DEBUG-CFG-TARGETS)        \
                $(DEBUG-PUBLISH-TARGETS)    \
                $(DEBUG-EXP-TARGETS)        \
                $(DEBUG-TEST-TARGETS)       \
                $(DEBUG-TEST-TARGETS-FULL)  \
                $(DEBUG-CUNIT-TARGETS)      \
                $(DEBUG-CUNIT-TARGETS-FULL)

# Coverage-Targets, Typical order: Libs, Exes, Cfgs, Publish, Experiments, Tests
COVERAGE-TARGETS = $(COVERAGE-LIB-TARGETS)        \
                   $(COVERAGE-LIB_S-TARGETS)      \
                   $(COVERAGE-EXE-TARGETS)        \
                   $(COVERAGE-CFG-TARGETS)        \
                   $(COVERAGE-PUBLISH-TARGETS)    \
                   $(COVERAGE-EXP-TARGETS)        \
                   $(COVERAGE-TEST-TARGETS)       \
                   $(COVERAGE-TEST-TARGETS-FULL)  \
                   $(COVERAGE-CUNIT-TARGETS)      \
                   $(COVERAGE-CUNIT-TARGETS-FULL)

#Code to Extract the MQ Client:
$(MQC)/inc/cmqc.h: $(MQ_CLIENT_TGZ)
	@(echo client is $(MQ_CLIENT_TGZ))
	@(echo MQC_HOME set by user is $(MQC_HOME))
	@(echo MQC based on MQC_HOME or created is $(MQC))
	mkdir -p $(MQC)/tmpextract
	mkdir -p $(MQC)/inc
	mkdir -p $(MQC)/lib64
	tar -xvf $(MQ_CLIENT_TGZ) -C $(MQC)/tmpextract
	mkdir -p $(MQC)/tmpextract/client
	rpm2cpio $(MQC)/tmpextract/MQSeriesClient-*.x86_64.rpm | cpio -id -D $(MQC)/tmpextract/client
	mkdir -p $(MQC)/tmpextract/runtime
	rpm2cpio $(MQC)/tmpextract/MQSeriesRuntime-*.x86_64.rpm | cpio -id -D $(MQC)/tmpextract/runtime
	mkdir -p $(MQC)/tmpextract/sdk
	rpm2cpio $(MQC)/tmpextract/MQSeriesSDK-*.x86_64.rpm | cpio -id -D $(MQC)/tmpextract/sdk
	mkdir -p $(MQC)/tmpextract/samples
	rpm2cpio $(MQC)/tmpextract/MQSeriesSamples-*.x86_64.rpm | cpio -id -D $(MQC)/tmpextract/samples
	rm -f $(MQC)/lib64/*.so
	rm -f $(MQC)/inc/*.h
	cp $(MQC)/tmpextract/client/opt/mqm/lib64/libmqdc_r.so   $(MQC)/lib64/
	cp $(MQC)/tmpextract/client/opt/mqm/lib64/libmqiz_r.so   $(MQC)/lib64/
	cp $(MQC)/tmpextract/runtime/opt/mqm/lib64/libmqmcs_r.so $(MQC)/lib64/
	cp $(MQC)/tmpextract/runtime/opt/mqm/lib64/libmqxzu_r.so $(MQC)/lib64/
	cp $(MQC)/tmpextract/sdk/opt/mqm/inc/cmqcfc.h            $(MQC)/inc/
	cp $(MQC)/tmpextract/sdk/opt/mqm/inc/cmqxc.h             $(MQC)/inc/
	cp $(MQC)/tmpextract/sdk/opt/mqm/inc/cmqc.h              $(MQC)/inc/
	cp $(MQC)/tmpextract/samples/opt/mqm/samp/xatm/xa.h      $(MQC)/inc/
	#The Redistributable client gets included in the rpm:
	cp $(MQC)/tmpextract/*IBM-MQC-Redist-*.tar.gz            $(MQC)/
	chmod -R 770 $(MQC)/tmpextract
	rm -rf $(MQC)/tmpextract

# Typical order: Directories, Targets, Debug-Targets
production: $(DIR-TARGETS) $(TARGETS)
debug: $(DEBUG-DIR-TARGETS) $(DEBUG-TARGETS)
coverage: $(COVERAGE-DIR-TARGETS) $(COVERAGE-TARGETS)

all: production debug

test: production $(LEGACY_BASIC) $(CUNIT_BASIC)
fulltest: production $(LEGACY_BASIC) $(LEGACY_FULL) $(CUNIT_BASIC) $(CUNIT_FULL)

debugtest: debug $(DEBUG_LEGACY_BASIC) $(DEBUG_CUNIT_BASIC)
debugfulltest: debug $(DEBUG_LEGACY_BASIC) $(DEBUG_LEGACY_FULL) $(DEBUG_CUNIT_BASIC) $(DEBUG_CUNIT_FULL)

coveragetest: coverage $(COVERAGE_LEGACY_BASIC) $(COVERAGE_CUNIT_BASIC)
coveragefulltest: coverage $(COVERAGE_LEGACY_BASIC) $(COVERAGE_LEGACY_FULL) $(COVERAGE_CUNIT_BASIC) $(COVERAGE_CUNIT_FULL)

.PHONY:: production debug coverage all \
         test fulltest \
         debugtest debugfulltest \
         coveragetest coveragefulltest
