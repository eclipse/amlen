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
# SubComponent: server_cluster
#
# Created on:
#     Author:
#

PROJECT = server_cluster

# Need to have PRJDIR before include so SRCDIR, INCDIR can be set correctly
PRJDIR=$(CURDIR)
# Specify the relative path from this project directory to the root of the directory structure
ROOTREL=..

include $(ROOTREL)/server_build/common.mk

$(eval $(call add-search-directory,.cpp,$(SRCDIR)/MCP_Routing/Routing/src))

#Files in this component need to know paths for server
#(uses the path_parser.py/paths.properties mechanism)
PATH_VAR_CFLAGS=$(IMA_SVR_PATH_DEFINES)

#Build against a specific version of boost if set
ifneq ($(IMA_BOOST_HOME),)
BOOST_INC_DIR=$(IMA_BOOST_HOME)/include
BOOST_LIB_DIR=$(IMA_BOOST_HOME)/lib
#The location we'll copy libraries from
BOOST_PRJ_LIBDIR=$(BOOST_LIB_DIR)
else
BOOST_INC_DIR=/usr/local/boost
BOOST_LIB_DIR=/usr/local/boost/libs
#The location we'll copy libraries from
BOOST_PRJ_LIBDIR=$(PRJDIR)/lib
endif

# Include the boost headers
IFLAGS += -I$(BOOST_INC_DIR)

# Add any required flags not specified in $(ROOTREL)/server_build/common.mk
CFLAGS +=
CPPFLAGS += -DSPDR_LINUX -DRUM_UNIX -DBOOST_THREAD_DYN_LINK -DBOOST_DATE_TIME_DYN_LINK -Wno-unused-local-typedefs
LDFLAGS += -L$(BOOST_LIB_DIR)
LDLIBS += -lboost_system -lboost_thread -lboost_date_time
XFLAGS +=
SHARED_FLAGS +=

# ------------------------------------------------
# Prebuilt libraries
# ------------------------------------------------
#LIB-TARGETS += $(COMMON_LIBDIR)/libboost_system$(SO) $(COMMON_LIBDIR)/libboost_system$(SO).1.66.0 \
#               $(COMMON_LIBDIR)/libboost_thread$(SO) $(COMMON_LIBDIR)/libboost_thread$(SO).1.66.0 \
#               $(COMMON_LIBDIR)/libboost_date_time$(SO) $(COMMON_LIBDIR)/libboost_date_time$(SO).1.66.0 #
#
#$(COMMON_LIBDIR)/libboost%: $(BOOST_PRJ_LIBDIR)/libboost%
#	$(call eyecatcher, Copying prebuilt BOOST library:$(notdir $@) from:$(notdir $<) Project:$(PROJECT))#
#	$(CP) $< $@
#	
#$(COMMON_LIBDIR)/%: $(PRJDIR)/lib/%
#	$(call eyecatcher, Copying prebuilt library:$(notdir $@) from:$(notdir $<) Project:$(PROJECT))
#	$(CP) $< $@
#
#DEBUG-LIB-TARGETS += $(COMMON_DEBUG_LIBDIR)/libboost_system$(SO) $(COMMON_DEBUG_LIBDIR)/libboost_system$(SO).1.66.0 \
#                     $(COMMON_DEBUG_LIBDIR)/libboost_thread$(SO) $(COMMON_DEBUG_LIBDIR)/libboost_thread$(SO).1.66.0 \
#                     $(COMMON_DEBUG_LIBDIR)/libboost_date_time$(SO) $(COMMON_DEBUG_LIBDIR)/libboost_date_time$(SO).1.66.0 
#
#$(COMMON_DEBUG_LIBDIR)/libboost%: $(BOOST_PRJ_LIBDIR)/libboost%
#	$(call eyecatcher, Copying prebuilt BOOST library:$(notdir $@) from:$(notdir $<) Project:$(PROJECT) Debug version)
#	$(CP) $< $@
#
#$(COMMON_DEBUG_LIBDIR)/%: $(PRJDIR)/lib/%
#	$(call eyecatcher, Copying prebuilt library:$(notdir $@) from:$(notdir $<) Project:$(PROJECT) Debug version)
#	$(CP) $< $@
#
#COVERAGE-LIB-TARGETS += $(COMMON_COVERAGE_LIBDIR)/libboost_system$(SO) $(COMMON_COVERAGE_LIBDIR)/libboost_system$(SO).1.66.0 \
#                        $(COMMON_COVERAGE_LIBDIR)/libboost_thread$(SO) $(COMMON_COVERAGE_LIBDIR)/libboost_thread$(SO).1.66.0 \
#                        $(COMMON_COVERAGE_LIBDIR)/libboost_date_time$(SO) $(COMMON_COVERAGE_LIBDIR)/libboost_date_time$(SO).1.66.0 
#
#$(COMMON_COVERAGE_LIBDIR)/libboost%: $(BOOST_PRJ_LIBDIR)/libboost%
#	$(call eyecatcher, Copying prebuilt BOOST library:$(notdir $@) from:$(notdir $<) Project:$(PROJECT) Coverage version)
#	$(CP) $< $@
#
#$(COMMON_COVERAGE_LIBDIR)/%: $(PRJDIR)/lib/%
#	$(call eyecatcher, Copying prebuilt library:$(notdir $@) from:$(notdir $<) Project:$(PROJECT) Coverage version)
#	$(CP) $< $@
	
	
# ------------------------------------------------
# Build the main MCP routing library
# ------------------------------------------------
IFLAGS += -I$(PRJDIR)/../server_transport/include

# Include the SpiderCast header directories
SPDRDIR = $(PRJDIR)/../server_spidercast/src/SpiderCastCpp
IFLAGS += -I$(SPDRDIR)/API/include -I$(SPDRDIR)/Trace/include

# Include the MCP_Routing Util headers, C and  C++ files
IFLAGS += -I$(SRCDIR)/MCP_Routing/Util/include
$(eval $(call add-search-directory,.c,$(SRCDIR)/MCP_Routing/Util/src))
$(eval $(call add-search-directory,.cpp,$(SRCDIR)/MCP_Routing/Util/src))

# Include the MCP_Routing API headers, C and C++ files
IFLAGS += -I$(SRCDIR)/MCP_Routing/API/include
$(eval $(call add-search-directory,.c,$(SRCDIR)/MCP_Routing/API/src))
$(eval $(call add-search-directory,.cpp,$(SRCDIR)/MCP_Routing/API/src))

# Include the MCP_Routing Control headers, C and C++ files
IFLAGS += -I$(SRCDIR)/MCP_Routing/Control/include
$(eval $(call add-search-directory,.c,$(SRCDIR)/MCP_Routing/Control/src))
$(eval $(call add-search-directory,.cpp,$(SRCDIR)/MCP_Routing/Control/src))

# Include the MCP_Routing Routing headers, C and C++ files
IFLAGS += -I$(SRCDIR)/MCP_Routing/Routing/include
$(eval $(call add-search-directory,.c,$(SRCDIR)/MCP_Routing/Routing/src))
libMCP_Routing_Routing-FILES = ASMFilter.cpp                       \
                               BloomFilter.cpp                     \
                               CountingBloomFilter.cpp             \
                               DiscoveryTimeoutTask.cpp            \
                               EngineEventCallbackCAdapter.cpp     \
                               EngineStatisticsTask.cpp            \
                               FatalErrorHandler.cpp               \
                               ForwardingControlCAdapter.cpp       \
                               GlobalSubManager.cpp                \
                               GlobalSubManagerImpl.cpp            \
                               LocalExactSubManager.cpp            \
                               LocalSubManager.cpp                 \
                               LocalSubManagerImpl.cpp             \
                               LocalWildcardSubManager.cpp         \
                               LocalMonitoringManager.cpp          \
                               MCPRoutingImpl.cpp                  \
                               PublishLocalBFTask.cpp              \
                               RemoteSubscriptionStats.cpp         \
                               RemoteSubscriptionStatsListener.cpp \
                               RemovedServers.cpp                  \
                               RoutingTasksHandler.cpp             \
                               GlobalRetainedStatsManager.cpp      \
                               LocalRetainedStatsManager.cpp       \
                               PublishRetainedTask.cpp             \
                               PublishMonitoringTask.cpp           \
                               TraceLevelMonitorTask.cpp           \
                               mccBFSet.c                          \
                               mccLookupSet.c                      \
                               mccWildcardBFSet.c 

libMCP_Routing_API-FILES = EngineEventCallback.cpp               \
                           ForwardingControl.cpp                 \
                           LocalForwardingEvents.cpp             \
                           LocalSubscriptionEvents.cpp           \
                           MCPConfig.cpp                         \
                           MCPRouting.cpp                        \
                           RemoteTopicTreeSubscriptionEvents.cpp \
                           RouteLookup.cpp                       \
                           ServerRegistration.cpp                \
                           StoreNodeData.cpp                     \
                           cluster.cpp

libMCP_Routing_Control-FILES = AdminDeleteRemovedServersTask.cpp  \
                               ControlManager.cpp                 \
                               ControlManagerImpl.cpp             \
                               FilterTags.cpp                     \
                               PublishRemovedServersTask.cpp      \
                               PublishRestoredNotInViewTask.cpp   \
                               RemoteServerRecord.cpp             \
                               RemoteServerStatus.cpp             \
                               RequestAdminMaintenanceModeTask.cpp \
                               SubCoveringFilterEventListener.cpp \
                               SubCoveringFilterPublisher.cpp     \
                               SubCoveringFilterPublisherImpl.cpp \
                               SubCoveringFilterWireFormat.cpp    \
                               ViewNotifyEvent.cpp                \
                               ViewNotifyTask.cpp                 \
                               ViewKeeper.cpp

libMCP_Routing_Util-FILES = AbstractTask.cpp        \
                            ByteBuffer.cpp          \
                            CyclicFileLogger.cpp    \
                            HasErrorCode.cpp        \
                            MCPExceptions.cpp       \
                            SubscriptionPattern.cpp \
                            TaskExecutor.cpp        \
                            Thread.cpp              \
                            MurmurHash3.c           \
                            city_c.c                \
                            hashFunction.c          \
                            ismLogListener.cpp		\
                            copyright.c
 
#libMCP_Routing-LIBS += libboost_thread$(SO) libboost_date_time$(SO) libboost_system$(SO) libSpiderCast$(SO) librum$(SO)

LIB-TARGETS += $(LIBDIR)/libMCP_Routing$(SO)
$(LIBDIR)/libMCP_Routing$(SO): $(call objects, $(libMCP_Routing_API-FILES)) \
                               $(call objects, $(libMCP_Routing_Routing-FILES)) \
                               $(call objects, $(libMCP_Routing_Control-FILES)) \
                               $(call objects, $(libMCP_Routing_Util-FILES)) | \
                               $(call libs, $(libMCP_Routing-LIBS))
	$(call make-c-library)

DEBUG-LIB-TARGETS += $(DEBUG_LIBDIR)/libMCP_Routing$(SO)
$(DEBUG_LIBDIR)/libMCP_Routing$(SO): $(call debug-objects, $(libMCP_Routing_API-FILES)) \
                                     $(call debug-objects, $(libMCP_Routing_Routing-FILES)) \
                                     $(call debug-objects, $(libMCP_Routing_Control-FILES)) \
                                     $(call debug-objects, $(libMCP_Routing_Util-FILES)) | \
                                     $(call debug-libs, $(libMCP_Routing-LIBS))
	$(call debug-make-c-library)

COVERAGE-LIB-TARGETS += $(COVERAGE_LIBDIR)/libMCP_Routing$(SO)
$(COVERAGE_LIBDIR)/libMCP_Routing$(SO): $(call coverage-objects, $(libMCP_Routing_API-FILES)) \
                                        $(call coverage-objects, $(libMCP_Routing_Routing-FILES)) \
                                        $(call coverage-objects, $(libMCP_Routing_Control-FILES)) \
                                        $(call coverage-objects, $(libMCP_Routing_Util-FILES)) | \
                                        $(call coverage-libs, $(libMCP_Routing-LIBS))
	$(call coverage-make-c-library)

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

# Typical order: Directories, Targets, Debug-Targets
production: $(COMMON-DIR-TARGETS) $(DIR-TARGETS) $(TARGETS)
debug: $(COMMON-DIR-TARGETS) $(DEBUG-DIR-TARGETS) $(DEBUG-TARGETS)
coverage: $(COMMON-DIR-TARGETS) $(COVERAGE-DIR-TARGETS) $(COVERAGE-TARGETS)

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
         coveragetest coveragefulltest \
         cleanupTests
