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
# SubComponent: server_admin
#
# Created on:
#     Author:
#

PROJECT = server_admin

# Need to have PRJDIR before include so SRCDIR, INCDIR can be set correctly
PRJDIR=$(CURDIR)
# Specify the relative path from this project directory to the root of the directory structure
ROOTREL=..

#Files in this component need to know paths for server
#(uses the path_parser.py/paths.properties mechanism)
PATH_VAR_CFLAGS=$(IMA_SVR_PATH_DEFINES)

include $(ROOTREL)/server_build/common.mk

# Add any required flags not specified in $(ROOTREL)/server_build/common.mk
IFLAGS += -I$(ROOTREL)/server_transport/include -I$(ROOTREL)/server_store/include
IFLAGS += -I$(ROOTREL)/server_engine/include -I$(ROOTREL)/server_cluster/include
IFLAGS += -I$(ROOTREL)/server_admin/deppkgs
CFLAGS += 
CPPFLAGS +=
LDFLAGS += -L$(ROOTREL)/server_admin/deppkgs -Wl,-rpath,$(ROOTREL)/server_admin/deppkgs
LDLIBS += -l$(OPENLDAP_LIBNAME) -lssl -licuuc -licui18n -licudata -lcurl -ljansson
XFLAGS +=
SHARED_FLAGS +=

#Tests that link against other libs
TESTLDFLAGS += $(LDFLAGS)
TESTLDLIBS += $(LDLIBS)

# ------------------------------------------------
# Add project specific make rules
# ------------------------------------------------

libismadmin-FILES = admin.c adminInternal.c adminHA.c config.c \
                    authorization.c authentication.c validateAndSet.c threadpool.c \
                    ldaputil.c copyright.c ltpautils.c transaction.c oauth.c \
                    adminpluginutils.c validate_internal.c validate_object.c \
                    validate_genericData.c validate_MessageHub.c validate_CRLProfile.c \
                    validate_ConnectionPolicy.c \
                    validate_QueuePolicy.c validate_SubscriptionPolicy.c validate_TopicPolicy.c \
                    clientSet.c validate_AdminEndpoint.c validate_CertificateProfile.c \
                    validate_Endpoint.c validate_LTPAProfile.c validate_OAuthProfile.c \
                    validate_SecurityProfile.c validate_Singleton.c validate_TrustedCertificate.c \
                    validate_ClusterMembership.c validate_TopicMonitor.c config_object.c config_internal.c \
                    config_jsonutils.c config_restapi.c validate_ConfigurationPolicy.c \
                    listTruststoreCert.c janssonConfigUtils.c janssonConfigProcessSchema.c \
                    janssonProcessPost.c janssonProcessGet.c janssonProcessDelete.c adminClient.c \
                    validate_HighAvailability.c validate_LDAP.c validate_Queue.c validate_Plugin.c \
                    security.c policies.c validate_QueueMgrConnection.c validate_DestinationMappingRule.c \
                    validate_LogLocation.c validate_Syslog.c janssonProcessMQCObjects.c validate_ClientCertificate.c \
                    importExport.c validate_MQCertificate.c crlprofile.c asyncRestCall.c resourceSetDescriptor.c \
                    validate_AdminSubs.c validate_ClusterRequestedTopics.c

libismadmin-LIBS = libismutil$(SO)

# Deal with the embeded source files
SERVER_MAIN_CFGDIR = $(PRJDIR)/$(ROOTREL)/server_main/config
$(OUTSRCDIR)/serverConfigSchema.c: $(SERVER_MAIN_CFGDIR)/serverConfigSchema.json
	$(XXD) -i $< | sed "s/[^ ]\+serverConfigSchema_json/serverConfigSchema_json/" > $@

$(OUTSRCDIR)/monitoringSchema.c: $(SERVER_MAIN_CFGDIR)/monitoringSchema.json
	$(XXD) -i $< | sed "s/[^ ]\+monitoringSchema_json/monitoringSchema_json/"> $@

# Make sure there is a dependency on the embedded source files for the source file that embeds them
$(BINDIR)/adminInternal$(OBJ): $(OUTSRCDIR)/serverConfigSchema.c $(OUTSRCDIR)/monitoringSchema.c
$(DEBUG_BINDIR)/adminInternal$(OBJ): $(OUTSRCDIR)/serverConfigSchema.c $(OUTSRCDIR)/monitoringSchema.c
$(COVERAGE_BINDIR)/adminInternal$(OBJ): $(OUTSRCDIR)/serverConfigSchema.c $(OUTSRCDIR)/monitoringSchema.c

LIB-TARGETS += $(LIBDIR)/libismadmin$(SO)
    
$(LIBDIR)/libismadmin$(SO): $(call objects, $(libismadmin-FILES)) | \
                            $(call libs, $(libismadmin-LIBS))
	$(call make-c-library)
	
LIB_S-TARGETS += $(LIBDIR_S)/libismadmin$(A)
$(LIBDIR_S)/libismadmin$(A): $(call objects, $(libismadmin-FILES)) 
	$(call make-c-static-library)

DEBUG-LIB-TARGETS += $(DEBUG_LIBDIR)/libismadmin$(SO)
$(DEBUG_LIBDIR)/libismadmin$(SO):  $(call debug-objects, $(libismadmin-FILES)) | \
                                   $(call debug-libs, $(libismadmin-LIBS))
	$(call debug-make-c-library)

DEBUG-LIB_S-TARGETS += $(DEBUG_LIBDIR_S)/libismadmin$(A)
$(DEBUG_LIBDIR_S)/libismadmin$(A):  $(call debug-objects, $(libismadmin-FILES)) 
	$(call debug-make-c-static-library)

COVERAGE-LIB-TARGETS += $(COVERAGE_LIBDIR)/libismadmin$(SO)
$(COVERAGE_LIBDIR)/libismadmin$(SO):  $(call coverage-objects, $(libismadmin-FILES)) | \
                                      $(call coverage-libs, $(libismadmin-LIBS))
	$(call coverage-make-c-library)

COVERAGE-LIB_S-TARGETS += $(COVERAGE_LIBDIR_S)/libismadmin$(A)
$(COVERAGE_LIBDIR_S)/libismadmin$(A):  $(call coverage-objects, $(libismadmin-FILES)) 
	$(call coverage-make-c-static-library)

test-LIBS = $(libismadmin-LIBS) \
            libismadmin$(SO)

CUNIT-TARGETS += $(CUNITTESTDIR)/admin_test$(EXE)
$(CUNITTESTDIR)/admin_test$(EXE): $(call objects, admin_test.c) | \
                                  $(call libs, $(test-LIBS))
	$(call build-cunit-test)

DEBUG-CUNIT-TARGETS += $(DEBUG_CUNITTESTDIR)/admin_test$(EXE)
$(DEBUG_CUNITTESTDIR)/admin_test$(EXE): $(call debug-objects, admin_test.c) | \
                                        $(call debug-libs, $(test-LIBS))
	$(call debug-build-cunit-test)

COVERAGE-CUNIT-TARGETS += $(COVERAGE_CUNITTESTDIR)/admin_test$(EXE)
$(COVERAGE_CUNITTESTDIR)/admin_test$(EXE): $(call coverage-objects, admin_test.c) | \
                                           $(call coverage-libs, $(test-LIBS))
	$(call coverage-build-cunit-test)

CUNIT_BASIC += admin_testBasic
admin_testBasic: $(CUNITTESTDIR)/admin_test$(EXE)
	$(call run-cunit-test,$<)

DEBUG_CUNIT_BASIC += debug_admin_testBasic
debug_admin_testBasic: $(DEBUG_CUNITTESTDIR)/admin_test$(EXE)
	$(call debug-run-cunit-test,$<)

COVERAGE_CUNIT_BASIC += coverage_admin_testBasic
coverage_admin_testBasic: $(COVERAGE_CUNITTESTDIR)/admin_test$(EXE)
	$(call coverage-run-cunit-test,$<)

CUNIT_FULL += admin_testFull
admin_testFull: $(CUNITTESTDIR)/admin_test$(EXE)
	$(call run-cunit-test,$<,FULL)

DEBUG_CUNIT_FULL += debug_admin_testFull
debug_admin_testFull: $(DEBUG_CUNITTESTDIR)/admin_test$(EXE)
	$(call debug-run-cunit-test,$<,FULL)

COVERAGE_CUNIT_FULL += coverage_admin_testFull
coverage_admin_testFull: $(COVERAGE_CUNITTESTDIR)/admin_test$(EXE)
	$(call coverage-run-cunit-test,$<,FULL)

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
COVERAGE-TARGETS = $(COVERAGE-LIB-TARGETS)     \
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
