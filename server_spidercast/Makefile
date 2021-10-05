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
# SubComponent: server_spidercast
#
# Created on:
#     Author:
#

PROJECT = server_spidercast

# Need to have PRJDIR before include so SRCDIR, INCDIR can be set correctly
PRJDIR=$(CURDIR)
# Specify the relative path from this project directory to the root of the directory structure
ROOTREL=..

include $(ROOTREL)/server_build/common.mk


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

ifneq (,$(wildcard $(ROOTREL)/server_certificates_ext/include))
     IFLAGS += -I$(ROOTREL)/server_certificates_ext/include
else
     IFLAGS += -I$(ROOTREL)/server_certificates/include
endif

# Add any required flags not specified in $(ROOTREL)/server_build/common.mk
CFLAGS += -Wno-enum-compare
CPPFLAGS += -DSPDR_FD_CLOEXEC -DSPDR_LINUX -DRUM_UNIX -DBOOST_THREAD_DYN_LINK -DBOOST_DATE_TIME_DYN_LINK -Wno-unused-local-typedefs
LDFLAGS += -L$(BOOST_LIB_DIR) -lboost_system -lboost_thread -lboost_date_time
LDLIBS +=
XFLAGS +=
SHARED_FLAGS +=

# ------------------------------------------------
# Prebuilt libraries
# ------------------------------------------------
#LIB-TARGETS += $(COMMON_LIBDIR)/libboost_system$(SO) $(COMMON_LIBDIR)/libboost_system$(SO).1.66.0 \
#               $(COMMON_LIBDIR)/libboost_thread$(SO) $(COMMON_LIBDIR)/libboost_thread$(SO).1.66.0 \
#               $(COMMON_LIBDIR)/libboost_date_time$(SO) $(COMMON_LIBDIR)/libboost_date_time$(SO).1.66.0 
#
#$(COMMON_LIBDIR)/libboost%: $(BOOST_PRJ_LIBDIR)/libboost%
#	$(call eyecatcher, Copying prebuilt BOOST library:$(notdir $@) from:$(notdir $<) Project:$(PROJECT))
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
# Build the RUM library
# ------------------------------------------------
RUMDIR = $(SRCDIR)/MCC_RUM
IFLAGS += -I$(RUMDIR)/include
$(eval $(call add-search-directory,.c,$(RUMDIR)/src))

OS = Linux
OS_ST=64
VERSION_ID=2.6.0.4
OSDIR = $(OS)/$(OS_ST)
#LIBDIR= ../lib/$(OSDIR)
#OBJDIR= ./$(OSDIR)
OPT 	=

M64_32 = -m32 -fPIC 
X64_32 =

M64_64 = -m64 -fPIC 
X64_64 =

M64 = $(M64_$(OS_ST))
X64 = $(X64_$(OS_ST))

CFLAGS_MD = -DENABLE_SSL -DENABLE_POLL -D_GNU_SOURCE -std=gnu99 -DLINUX -Wall -DENABLE_THREAD_AFFINITY -Wno-format-zero-length -O3
MD_OS_ST  = os_64


#INCDIR = -I. -I../include
CFLAGS += -DISM_BUILD -DRUM_CODE -DRMM_UNIX -DOS_$(OS) -DUNIX -D_THREAD_SAFE -D_REENTRANT -DBUILD_ID=$(BUILD_ID)
CFLAGS += $(CFLAGS_MD) $(USE_NOMA) $(DROP) -DOLD_OS_INFO -DVERSION_ID=$(VERSION_ID)

LIB-TARGETS += $(COMMON_LIBDIR)/librum$(SO)
librum-FILES = RumRxTxApi.c
librum-LIBS = libismutil$(SO) libismadmin$(SO)

$(COMMON_LIBDIR)/librum$(SO): $(call objects, $(librum-FILES)) | $(call libs, $(librum-LIBS))
	$(call make-c-library)

DEBUG-LIB-TARGETS += $(COMMON_DEBUG_LIBDIR)/librum$(SO)
$(COMMON_DEBUG_LIBDIR)/librum$(SO): $(call debug-objects, $(librum-FILES))
	$(call debug-make-c-library)

COVERAGE-LIB-TARGETS += $(COMMON_COVERAGE_LIBDIR)/librum$(SO)
$(COMMON_COVERAGE_LIBDIR)/librum$(SO): $(call coverage-objects, $(librum-FILES))
	$(call coverage-make-c-library)

	
	
# ------------------------------------------------
# Build the SpiderCast library
# ------------------------------------------------
SPDRDIR = $(SRCDIR)/SpiderCastCpp

$(eval $(call add-search-directory,.cpp,$(SPDRDIR)/API/src))
$(eval $(call add-search-directory,.cpp,$(SPDRDIR)/Trace/src))
$(eval $(call add-search-directory,.cpp,$(SPDRDIR)/Util/src))
$(eval $(call add-search-directory,.cpp,$(SPDRDIR)/CommAdapter/src))
$(eval $(call add-search-directory,.cpp,$(SPDRDIR)/Routing/src))
$(eval $(call add-search-directory,.cpp,$(SPDRDIR)/Election/src))
$(eval $(call add-search-directory,.cpp,$(SPDRDIR)/Membership/src))
$(eval $(call add-search-directory,.cpp,$(SPDRDIR)/Messaging/src))
$(eval $(call add-search-directory,.cpp,$(SPDRDIR)/Topology/src))
$(eval $(call add-search-directory,.cpp,$(SPDRDIR)/Core/src))

IFLAGS += -I$(SPDRDIR)/API/include
IFLAGS += -I$(SPDRDIR)/Trace/include
IFLAGS += -I$(SPDRDIR)/Util/include
IFLAGS += -I$(SPDRDIR)/CommAdapter/include
IFLAGS += -I$(SPDRDIR)/Routing/include
IFLAGS += -I$(SPDRDIR)/Election/include
IFLAGS += -I$(SPDRDIR)/Membership/include
IFLAGS += -I$(SPDRDIR)/Messaging/include
IFLAGS += -I$(SPDRDIR)/Topology/include
IFLAGS += -I$(SPDRDIR)/Core/include


SpiderCastCpp_API-FILES = \
                          P2PStreamRcv.cpp                \
                          StreamID.cpp                    \
                          LeaderElectionListener.cpp      \
                          MembershipService.cpp           \
                          P2PStreamTx.cpp                 \
                          P2PStreamEventListener.cpp      \
                          SpiderCast.cpp                  \
                          SpiderCastConfig.cpp            \
                          ConnectivityEvent.cpp           \
                          StackBackTrace.cpp              \
                          PropertyMap.cpp                 \
                          LeaderElectionService.cpp       \
                          SpiderCastEventListener.cpp     \
                          NetworkEndpoints.cpp            \
                          MembershipEvent.cpp             \
                          Topic.cpp                       \
                          FatalErrorEvent.cpp             \
                          NodeID.cpp                      \
                          PubSubEvent.cpp                 \
                          MetaData.cpp                    \
                          TopicSubscriber.cpp             \
                          MutableSharedBuffer.cpp         \
                          AttributeMap.cpp                \
                          BasicConfig.cpp                 \
                          RxMessage.cpp                   \
                          P2PStreamEvent.cpp              \
                          SpiderCastLogicError.cpp        \
                          AttributeValue.cpp              \
                          ConstSharedBuffer.cpp           \
                          SpiderCastEvent.cpp             \
                          WarningEvent.cpp                \
                          SpiderCastRuntimeError.cpp      \
                          TopicPublisher.cpp              \
                          PubSubEventListener.cpp         \
                          MessageListener.cpp             \
                          SpiderCastFactory.cpp           \
                          TxMessage.cpp                   \
                          LogListener.cpp                 \
                          MembershipListener.cpp

SpiderCastCpp_Trace-FILES = \
                            CyclicFileLogListener.cpp     \
                            ScTrConstants.cpp             \
                            ScTr.cpp                      \
                            ScTrWriter.cpp                \
                            ScTraceable.cpp               \
                            TraceUtils.cpp                \
                            ScTraceBuffer.cpp             \
                            SimpleLogListener.cpp

SpiderCastCpp_Util-FILES = \
                           TaskSchedule.cpp              \
                           VirtualID.cpp                 \
                           SHA1.cpp                      \
                           printStackTrace.cpp           \
                           EnumCounter.cpp               \
                           AbstractTask.cpp              \
                           BusName.cpp                   \
                           Thread.cpp                    \
                           ByteBuffer.cpp                \
                           HierarchyUtils.cpp            \
                           ThreadControl.cpp

SpiderCastCpp_CommAdapter-FILES = \
                                  CommUtils.cpp                             \
                                  CommRumAdapter.cpp                        \
                                  LocalNeighbor.cpp                         \
                                  RumConnectionsMgr.cpp                     \
                                  CommRumTxMgr.cpp                          \
                                  CommUDPMulticast.cpp                      \
                                  CommUDP.cpp                               \
                                  RumNeighbor.cpp                           \
                                  CommRumReceiver.cpp                       \
                                  Neighbor.cpp                              \
                                  CommAdapter.cpp                           \
                                  CommAdapterDefinitions.cpp                \
                                  CommEventInfo.cpp                         \
                                  CommUDPThread.cpp                         \
                                  IncomingMsgQ.cpp

SpiderCastCpp_Routing-FILES = \
                              BroadcastRouter.cpp            \
                              RoutingThread.cpp              \
                              Next2HopsBroadcast.cpp         \
                              PubSubViewListener.cpp         \
                              RoutingTableLookup.cpp         \
                              RoutingManager.cpp             \
                              PubSubRouter.cpp               \
                              P2PRouter.cpp                  \
                              SupervisorPubSubBridge.cpp     \
                              PubSubViewKeeper.cpp           \
                              RoutingManagerImpl.cpp         \
                              DBridgePubSubInterestTask.cpp  \
                              RoutingTable.cpp               \
                              PubSubBridge.cpp               \
                              DelegatePubSubBridge.cpp

SpiderCastCpp_Election-FILES = \
                               LEViewKeeper.cpp      \
                               LEViewListener.cpp    \
                               LEWarmupTask.cpp      \
                               LECandidate.cpp

SpiderCastCpp_Membership-FILES = \
                                 MembershipServiceConfig.cpp        \
                                 AttributeTable.cpp                 \
                                 HighPriorityMonitor.cpp            \
                                 MembershipManagerImpl.cpp          \
                                 MembershipPeriodicTask.cpp         \
                                 MembershipManager.cpp              \
                                 NodeHistorySet.cpp                 \
                                 AttributeManager.cpp               \
                                 FirstViewDeliveryTask.cpp          \
                                 NeighborChangeTask.cpp             \
                                 MembershipTermination2Task.cpp     \
                                 UpdateDatabase.cpp                 \
                                 AttributeControl.cpp               \
                                 BootstrapSet.cpp                   \
                                 SuspicionList.cpp                  \
                                 RefreshSuccessorListTask.cpp       \
                                 MembershipTerminationTask.cpp      \
                                 ChangeOfMetadataDeliveryTask.cpp   \
                                 SCMembershipListener.cpp           \
                                 NodeVersion.cpp                    \
                                 ClearRetainAttrTask.cpp            \
                                 Suspicion.cpp                      \
                                 SCMembershipEvent.cpp              \
                                 NodeInfo.cpp                       \
                                 NodeHistoryPruneTask.cpp           \
                                 MembershipServiceImpl.cpp

SpiderCastCpp_Messaging-FILES = \
                                StreamIDImpl.cpp                    \
                                P2PStreamTxImpl.cpp                 \
                                TopicPublisherImpl.cpp              \
                                P2PStreamRcvImpl.cpp                \
                                TopicSubscriberImpl.cpp             \
                                MessagingManagerImpl.cpp            \
                                TopicImpl.cpp                       \
                                MessagingManager.cpp                \
                                P2PStreamSyncCreationAdapter.cpp    \
                                RxMessageImpl.cpp                   \
                                TopicRxBestEffort.cpp

SpiderCastCpp_Topology-FILES = \
                               TopologyChangeSuccessorTask.cpp                       \
                               HierarchyDelegateViewUpdateTask.cpp                   \
                               StopInitialDiscoveryPeriodTask.cpp                    \
                               HierarchyPeriodicTask.cpp                             \
                               OutgoingStructuredNeighborTable.cpp                   \
                               HierarchySupervisorTaskInterface.cpp                  \
                               HierarchySupervisor.cpp                               \
                               HierarchySupervisorForeignZoneMembershipTask.cpp      \
                               TopologyManager.cpp                                   \
                               HierarchyDelegateTaskInterface.cpp                    \
                               HierarchyTerminationTask.cpp                          \
                               DiscoveryPeriodicTask.cpp                             \
                               TopologyRandomDisconnectTask.cpp                      \
                               HierarchySupervisorSetActiveDelegatesTask.cpp         \
                               TopologyRandomConnectTask.cpp                         \
                               HierarchyDelegate.cpp                                 \
                               TopologyDiscoveryReplySendTask.cpp                    \
                               SupervisorViewKeeper.cpp                              \
                               HierarchyViewListener.cpp                             \
                               TopologyStructuredRefreshTask.cpp                     \
                               TopologyStructuredConnectTask.cpp                     \
                               HierarchySupervisorZoneCensusTask.cpp                 \
                               HierarchyDelegateConnectTask.cpp                      \
                               TopologyUpdateDegreeTask.cpp                          \
                               HierarchyManagerImpl.cpp                              \
                               SupervisorNeighborTable.cpp                           \
                               HierarchyViewKeeper.cpp                               \
                               HierarchyDelegatePubSubBridgeTask.cpp                 \
                               TopologyTerminationTask.cpp                           \
                               HierarchyDelegateUnquarantineTask.cpp                 \
                               HierarchySupervisorForeignZoneMembershipTOTask.cpp    \
                               NeighborTable.cpp                                     \
                               TopologyManagerImpl.cpp                               \
                               HierarchyManager.cpp

SpiderCastCpp_Core-FILES = \
                           CoreInterface.cpp           \
                           SpiderCastConfigImpl.cpp    \
                           SpiderCastImpl.cpp          \
                           NodeIDImpl.cpp              \
                           SpiderCastFactoryImpl.cpp   \
                           MemTopoThread.cpp           \
                           SCMessage.cpp               \
                           VirtualIDCache.cpp          \
                           NodeIDCache.cpp             \
                           StatisticsTask.cpp


#libSpiderCastCpp-LIBS += libboost_thread$(SO) libboost_date_time$(SO) libboost_system$(SO)

LIB-TARGETS += $(COMMON_LIBDIR)/libSpiderCast$(SO)
$(COMMON_LIBDIR)/libSpiderCast$(SO): \
                                     $(call objects, $(SpiderCastCpp_Util-FILES)) \
                                     $(call objects, $(SpiderCastCpp_Trace-FILES)) \
                                     $(call objects, $(SpiderCastCpp_Topology-FILES)) \
                                     $(call objects, $(SpiderCastCpp_Routing-FILES)) \
                                     $(call objects, $(SpiderCastCpp_Messaging-FILES)) \
                                     $(call objects, $(SpiderCastCpp_Membership-FILES)) \
                                     $(call objects, $(SpiderCastCpp_Election-FILES)) \
                                     $(call objects, $(SpiderCastCpp_CommAdapter-FILES)) \
                                     $(call objects, $(SpiderCastCpp_API-FILES)) \
                                     $(call objects, $(SpiderCastCpp_Core-FILES)) | \
                                     $(call libs, $(libSpiderCastCpp-LIBS))
	$(call make-c-library)

DEBUG-LIB-TARGETS += $(COMMON_DEBUG_LIBDIR)/libSpiderCast$(SO)
$(COMMON_DEBUG_LIBDIR)/libSpiderCast$(SO): \
                                     $(call debug-objects, $(SpiderCastCpp_Util-FILES)) \
                                     $(call debug-objects, $(SpiderCastCpp_Trace-FILES)) \
                                     $(call debug-objects, $(SpiderCastCpp_Topology-FILES)) \
                                     $(call debug-objects, $(SpiderCastCpp_Routing-FILES)) \
                                     $(call debug-objects, $(SpiderCastCpp_Messaging-FILES)) \
                                     $(call debug-objects, $(SpiderCastCpp_Membership-FILES)) \
                                     $(call debug-objects, $(SpiderCastCpp_Election-FILES)) \
                                     $(call debug-objects, $(SpiderCastCpp_CommAdapter-FILES)) \
                                     $(call debug-objects, $(SpiderCastCpp_API-FILES)) \
                                     $(call debug-objects, $(SpiderCastCpp_Core-FILES)) | \
                                     $(call debug-libs, $(libSpiderCastCpp-LIBS))
	$(call debug-make-c-library)

COVERAGE-LIB-TARGETS += $(COMMON_COVERAGE_LIBDIR)/libSpiderCast$(SO)
$(COMMON_COVERAGE_LIBDIR)/libSpiderCast$(SO): \
                                     $(call coverage-objects, $(SpiderCastCpp_Util-FILES)) \
                                     $(call coverage-objects, $(SpiderCastCpp_Trace-FILES)) \
                                     $(call coverage-objects, $(SpiderCastCpp_Topology-FILES)) \
                                     $(call coverage-objects, $(SpiderCastCpp_Routing-FILES)) \
                                     $(call coverage-objects, $(SpiderCastCpp_Messaging-FILES)) \
                                     $(call coverage-objects, $(SpiderCastCpp_Membership-FILES)) \
                                     $(call coverage-objects, $(SpiderCastCpp_Election-FILES)) \
                                     $(call coverage-objects, $(SpiderCastCpp_CommAdapter-FILES)) \
                                     $(call coverage-objects, $(SpiderCastCpp_API-FILES)) \
                                     $(call coverage-objects, $(SpiderCastCpp_Core-FILES)) | \
                                     $(call coverage-libs, $(libSpiderCastCpp-LIBS))
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
