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

PROJECT = proxy_br

# Need to have PRJDIR before include so SRCDIR, INCDIR can be set correctly
PRJDIR=$(CURDIR)
# Specify the relative path from this project directory to the root of the directory structure
ROOTREL=..
IMA_NO_BOOST=TRUE
include $(ROOTREL)/server_build/common.mk

# Include the server_transport source directory for headers and C files (wstcp.c)
IFLAGS += -I$(ROOTREL)/server_transport/src
$(eval $(call add-search-directory,.c,$(ROOTREL)/server_transport/src))
IFLAGS += -I$(ROOTREL)/server_utils/src
$(eval $(call add-search-directory,.c,$(ROOTREL)/server_utils/src))
$(eval $(call add-search-directory,.cpp,$(ROOTREL)/server_utils/src))
IFLAGS += -I$(ROOTREL)/server_proxy/src
$(eval $(call add-search-directory,.c,$(ROOTREL)/server_proxy/src))

#Include the header file paths needed by utils:
IFLAGS += -I$(ROOTREL)/server_admin/include
IFLAGS += -I$(ROOTREL)/server_monitoring/include 
IFLAGS += -I$(PRJDIR)/../server_transport/include
IFLAGS += -I$(PRJDIR)/../server_snmp/src


#Find out the RPATH directory
IMABRIDGE_RUNPATH_DIR := $(shell python3 $(PRJDIR)/$(ROOTREL)/server_build/path_parser.py -p IMA_RPATH_BRIDGE -m=value)

# Add any required flags not specified in $(ROOTREL)/server_build/common.mk
IDFLAGS +=
# Try it either in the JVM or in build_tools
IFLAGS += -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux
CFLAGS += -DIMAPROXY -DHAS_BRIDGE -DNO_PXACT -DNO_SQS -DNO_CURL -DNO_PROXY -DGAI_SIG
LDFLAGS += -Wl,-rpath,$(IMABRIDGE_RUNPATH_DIR)
LDLIBS += $(IMA_ICU_LIBS) -licuuc -licui18n -licudata -lanl $(SSL_LIB) -lssl -lcrypto -ldl
XFLAGS +=
SHARED_FLAGS +=
TESTLDFLAGS += $(LDFLAGS)
TESTLDLIBS += $(LDLIBS)
test-LIBS = libimabridge$(SO) libismutil${SO}


#Files in this component need to know paths for server as it compiles some
#server_utils code that references those paths even though they aren't used
#here (and the bridge ones are used here)
#(uses the path_parser.py/paths.properties mechanism)
PATH_VAR_CFLAGS=$(IMA_SVR_AND_BRIDGE_PATH_DEFINES)

# ------------------------------------------------
# Add project specific make rules
# ------------------------------------------------




imabridge-FILES = imabridge.c

libimabridge-FILES = bridge.c pxtransport.c pxtcp.c wstcp.c pxmqtt.c tenant.c pxconfig.c \
    javaconfig.c restconfig.c pxStatsdClient.c	monitor.c pxmux.c pxmhub.c pxkafka.c rehash.c \
    execinfo.c  ismutil.c  properties.c map.c server.c array.c  perfstat.c time.c \
    timer.c ismlist.c logformat.c logwriter.c  bufferpool.c  global.c ssl.c tls.c timezone.cpp \
    json.c selector.c filter.c content.c trace.c copyright.c xmlparser.c throttle.c

imabridge-LIBS = libimabridge$(SO) libismutil${SO}

libimabridge-LIBS = libismutil${SO}



EXE-TARGETS += $(COMMON_BINDIR)/imabridge$(EXE)
$(COMMON_BINDIR)/imabridge$(EXE): $(call objects, $(imabridge-FILES)) | \
                                 $(call libs, $(imabridge-LIBS))
	$(call link-c-program)

DEBUG-EXE-TARGETS += $(COMMON_DEBUG_BINDIR)/imabridge$(EXE)
$(COMMON_DEBUG_BINDIR)/imabridge$(EXE): $(call debug-objects, $(imabridge-FILES)) | \
                                       $(call debug-libs, $(imabridge-LIBS))
	$(call debug-link-c-program)


LIB-TARGETS += $(LIBDIR)/libimabridge$(SO)
$(LIBDIR)/libimabridge$(SO): $(call objects, $(libimabridge-FILES)) | \
                            $(call libs, $(libimabridge-LIBS))
	$(call make-c-library)

DEBUG-LIB-TARGETS += $(DEBUG_LIBDIR)/libimabridge$(SO)
$(DEBUG_LIBDIR)/libimabridge$(SO): $(call debug-objects, $(libimabridge-FILES)) | \
                                  $(call debug-libs, $(libimabridge-LIBS))
	$(call debug-make-c-library)

CUNIT-TARGETS += $(CUNITTESTDIR)/bridge_test$(EXE)
$(CUNITTESTDIR)/bridge_test$(EXE): $(call objects, bridge_test.c) | \
                                  $(call libs, $(test-LIBS))
	$(call build-cunit-test)

DEBUG-CUNIT-TARGETS += $(DEBUG_CUNITTESTDIR)/proxy_test$(EXE)
$(DEBUG_CUNITTESTDIR)/bridge_test$(EXE): $(call debug-objects, bridge_test.c) | \
                                  $(call debug-libs, $(test-LIBS))
	$(call debug-build-cunit-test)

CUNIT_BASIC += bridge_test
bridge_test: $(CUNITTESTDIR)/bridge_test$(EXE)
	$(call run-cunit-test,$<)

.PHONY:: bridge_test

DEBUG_CUNIT_BASIC += debug_bridge_test
debug_bridge_test: $(DEBUG_CUNITTESTDIR)/bridge_test$(EXE)
	$(call debug-run-cunit-test,$<)

.PHONY:: debug_bridge_test

# Typical order: Directories, Targets, Debug-Targets
production: $(COMMON-DIR-TARGETS) $(DIR-TARGETS) $(LIB-TARGETS) $(EXE-TARGETS) $(CUNITTESTDIR)/bridge_test$(EXE)
debug: $(COMMON-DIR-TARGETS) $(DEBUG-DIR-TARGETS) $(DEBUG-LIB-TARGETS)  $(DEBUG-EXE-TARGETS) $(DEBUG_CUNITTESTDIR)/bridge_test$(EXE)

all: production debug

test: production $(CUNIT_BASIC)
fulltest: production $(CUNIT_BASIC)
debugtest: debug $(DEBUG_CUNIT_BASIC)
debugfulltest: debug debug $(DEBUG_CUNIT_BASIC)
coveragetest: production $(CUNIT_BASIC)
coveragefulltest: production $(CUNIT_BASIC)

.PHONY:: production debug coverage all test fulltest coveragetest coveragefulltest debugtest debugfulltest
