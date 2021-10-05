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


PROJECT = svtCSamples

# Need to have PRJDIR before  include so SRCDIR, INCDIR can be set correctly
PRJDIR=$(CURDIR)

ifneq ($(SSL_HOME),)
SSL_INCLUDE=-I$(SSL_HOME)/include
SSL_LIB= -L$(SSL_HOME)/lib
else
SSL_INCLUDE=
endif

include ../server_build/common.mk

# Add any required flags not specified in ../server_build/common.mk
IFLAGS += -I../client_mqttv3sample_C/include/
IFLAGS += $(SSL_INCLUDE)
CFLAGS +=  -pg  -g -O0 -fPIC
CPPFLAGS +=
LDFLAGS += -pg -L/usr/local/ssl/lib
LDLIBS += $(SSL_LIB) -lpthread -lssl -lcrypto
LDLIBS2 = $(LDLIBS)
LDLIBS3 = $(LDLIBS)
LDLIBS += -lmqttv3cs
LDLIBS3 += -lmqttv3as
XFLAGS +=
SHARED_FLAGS +=

COMMON_LIBDIR := ../client_mqttv3sample_C/linux_ia64
RUNPATH_FLAGS = -Wl,-rpath-link,../client_mqttv3sample_C/linux_ia64/ -Wl,-rpath,'$${ORIGIN}/../../../application/client_ship/lib64' -Wl,-rpath,'$${ORIGIN}/../../client_ship/lib64/' -Wl,-rpath,'$${ORIGIN}/../../../client_ship/lib64/'  -Wl,-rpath,'$${ORIGIN}/../../client_mqttv3sample_C/linux_ia64/'


PUBLISH-TARGETS =
CLIENT-PUBLISH-TARGETS =

# ------------------------------------------------
# Add project specific make rules
# ------------------------------------------------

# Function: Link objects into a program
define link-c-program2
	$(call eyecatcher, Link Program 2 Target:${notdir $@} Project:$(PROJECT))
	$(CC) $(LDFLAGS) -o $@ \
	$(RUNPATH_FLAGS) \
	$(filter %$(OBJ),$^) \
	$(addprefix -l,$(subst lib,,$(basename $(notdir $(filter %$(SO),$^))))) \
	-L$(COMMON_LIBDIR) $(LDLIBS2)
endef

# Function: Link objects into a program
define link-c-program3
	$(call eyecatcher, Link Program 3 Target:${notdir $@} Project:$(PROJECT))
	$(CC) $(LDFLAGS) -o $@ \
	$(RUNPATH_FLAGS) \
	$(filter %$(OBJ),$^) \
	$(addprefix -l,$(subst lib,,$(basename $(notdir $(filter %$(SO),$^))))) \
	-L$(COMMON_LIBDIR) $(LDLIBS3)
endef

udptool-FILES = udptool.c
tcptool-FILES = tcptool.c
mqtttool-FILES = mqtttool.c
MQTTV3SAMPLE-FILES = MQTTV3Sample.c
MQTTV3SSAMPLE-FILES = MQTTV3SSample.c
MQTTV3AsyncSSAMPLE-FILES = MQTTV3AsyncSSample.c
MQTTV3ASAMPLE-FILES = MQTTV3ASample.c
mqttsample_svt-FILES = mqttsample.c utils.c connect.c publish.c subscribe.c
mqttsample_array-FILES = mqttsample_array.c utils_array.c connect_array.c publish_array.c subscribe_array.c

EXE-TARGETS += $(BINDIR)/udptool$(EXE) $(BINDIR)/tcptool$(EXE)
EXE-TARGETS += $(BINDIR)/mqtttool$(EXE) $(BINDIR)/mqttsample_svt$(EXE)
EXE-TARGETS += $(BINDIR)/mqttsample_array$(EXE)
EXE-TARGETS += $(BINDIR)/MQTTV3SAMPLE$(EXE)
EXE-TARGETS += $(BINDIR)/MQTTV3SSAMPLE$(EXE)
EXE-TARGETS += $(BINDIR)/MQTTV3ASAMPLE$(EXE)
EXE-TARGETS += $(BINDIR)/MQTTV3AsyncSSAMPLE$(EXE)

$(BINDIR)/udptool$(EXE): $(call objects, $(udptool-FILES)) \
	$(call libs, $(udptool-LIBS))
	$(call link-c-program2)

$(BINDIR)/tcptool$(EXE): $(call objects, $(tcptool-FILES)) \
	$(call libs, $(tcptool-LIBS))
	$(call link-c-program2)

$(BINDIR)/mqtttool$(EXE): $(call objects, $(mqtttool-FILES)) \
	$(call libs, $(mqtttool-LIBS))
	$(call link-c-program2)

$(BINDIR)/mqttsample_svt$(EXE): $(call objects, $(mqttsample_svt-FILES)) \
	$(call libs, $(mqttsample_svt-LIBS))
	$(call link-c-program2)

$(BINDIR)/MQTTV3SAMPLE$(EXE): $(call objects, $(MQTTV3SAMPLE-FILES)) \
	$(call libs, $(MQTTV3SAMPLE-LIBS))
	$(call link-c-program2)

$(BINDIR)/MQTTV3SSAMPLE$(EXE): $(call objects, $(MQTTV3SSAMPLE-FILES)) \
	$(call libs, $(MQTTV3SSAMPLE-LIBS))
	$(call link-c-program2)

$(BINDIR)/MQTTV3AsyncSSAMPLE$(EXE): $(call objects, $(MQTTV3AsyncSSAMPLE-FILES)) \
	$(call libs, $(MQTTV3AsyncSSAMPLE-LIBS))
	$(call link-c-program3)

$(BINDIR)/MQTTV3ASAMPLE$(EXE): $(call objects, $(MQTTV3ASAMPLE-FILES)) \
	$(call libs, $(MQTTV3ASAMPLE-LIBS))
	$(call link-c-program3)

$(BINDIR)/mqttsample_array$(EXE): $(call objects, $(mqttsample_array-FILES)) \
	$(call libs, $(mqttsample_array-LIBS))
	$(call link-c-program2)

strip_files: $(BINDIR)/mqttsample_svt$(EXE)
	strip  $(BINDIR)/mqttsample_svt$(EXE)
	strip  $(BINDIR)/mqttsample_array$(EXE)

# ------------------------------------------------
# Define order of targets (after targets defined)
# ------------------------------------------------

# Targets, Typical order: Libs, Exes, Cfgs, Publish, Experiments, Tests

TARGETS = $(EXE-TARGETS)

# Typical order: Directories, Targets, Debug-Targets
all: $(DIR-TARGETS) $(TARGETS) strip_files

.PHONY:: all strip_files
