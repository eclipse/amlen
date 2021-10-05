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
# SubComponent: server_engine
#
# Created on:
#     Author:
#

PROJECT = server_engine

# Need to have PRJDIR before include so SRCDIR, INCDIR can be set correctly
PRJDIR=$(CURDIR)
# Specify the relative path from this project directory to the root of the directory structure
ROOTREL=..

include $(ROOTREL)/server_build/common.mk

# Include our test utils subdirectory for headers and C files
IFLAGS += -I$(TSTDIR)/utils
$(eval $(call add-search-directory,.c,$(TSTDIR)/utils))

# Include our diagnostic utilities subdirectory for headers and C files
IFLAGS += -I$(SRCDIR)/diagnostics
$(eval $(call add-search-directory,.c,$(SRCDIR)/diagnostics))

# Include the export function subdirectory for headers and C files
IFLAGS += -I$(SRCDIR)/export
$(eval $(call add-search-directory,.c,$(SRCDIR)/export))

# Add any required flags not specified in $(ROOTREL)/server_build/common.mk
# Add -DMEMDEBUG=1 to CFLAGS to cause the engine to do additional checks on memory
# Add -DNO_MALLOC_WRAPPER to CFLAGS to disable memHandler functions.
# Add -DENGINE_FORCE_INTERMEDIATE_TO_MULTI to force all requests for intermediate queue to
#                                          instead use multiConsumer queue.
# Add -DDEFER_ALL_DEFERRABLE_FREES to force all deferrable frees to be deferred
# Add -DIMA_IGNORE_USESPINLOCKS_PROPERTY to CFLAGS to ignore the UseSpinlocks attribute in queues
# Add -DUSEFAKE_ASYNC_COMMIT to use fake async store commits
# Add -DALLOW_POLICY_NAME_TO_NAME_MAPPING to CFLAGS to enable policy name remapping during store recovery
# Add -DENABLE_LOCK_CHECKING to enable checking that locks are owned
# Add -DIEJQ_JOBQUEUE_PUTLOCK to use a putlock in the jobQueue code

ifneq ($(findstring fakeAsync,$(OUTPUT_TYPE)),)
CFLAGS += -DUSEFAKE_ASYNC_COMMIT
endif

#We rely on 16 bytes CAS (thus -mcx16)
CFLAGS += -mcx16
CPPFLAGS +=
LDFLAGS +=
LDLIBS += -lpthread -ldl
XFLAGS +=
SHARED_FLAGS +=

# Allow for the inclusion of a single extra include file in every compilation, for
# example to add explicit break points to the code.
ifneq ($(EXTRA_INCLUDE_FILE),)
CFLAGS += -include $(EXTRA_INCLUDE_FILE)
endif

TESTLDFLAGS += $(LDFLAGS)
TESTLDLIBS += $(LDLIBS)

#Files in this component need to know paths for server
#(uses the path_parser.py/paths.properties mechanism)
PATH_VAR_CFLAGS=$(IMA_SVR_PATH_DEFINES)

# ------------------------------------------------
# Build the main engine library
# ------------------------------------------------

libismengine-FILES = ackList.c                \
                     clientState.c            \
                     clientStateUtils.c       \
                     clientStateRestore.c     \
                     destination.c            \
                     engine.c                 \
                     engineAsync.c            \
                     engineDiag.c             \
                     engineDump.c             \
                     engineGlobal.c           \
                     engineHashTable.c        \
                     engineHashSet.c          \
                     engineSplitList.c        \
                     engineDeferredFree.c     \
                     engineMonitoring.c       \
                     engineNotifications.c    \
                     engineQueue.c            \
                     engineRestore.c          \
                     engineRestoreTable.c     \
                     engineStore.c            \
                     engineTimers.c           \
                     engineTraceDump.c        \
                     engineUtils.c            \
                     expiringGet.c            \
                     exportResources.c        \
                     exportCrypto.c           \
                     exportClientState.c      \
                     exportInflightMsgs.c     \
                     exportIntermediateQ.c    \
                     exportMessage.c          \
                     exportMultiConsumerQ.c   \
                     exportSimpQ.c            \
                     exportSubscription.c     \
                     exportRetained.c         \
                     fakeAsyncStore.c         \
                     intermediateQ.c          \
                     jobQueue.c               \
                     threadJobs.c             \
                     lockManager.c            \
                     memHandler.c             \
                     mempool.c                \
                     messageDelivery.c        \
                     messageExpiry.c          \
                     clientStateExpiry.c      \
                     msgCommon.c              \
                     multiConsumerQ.c         \
                     policyInfo.c             \
                     policyInfoRestore.c      \
                     queueNamespace.c         \
                     resourceSetStats.c       \
                     resourceSetMem.c         \
                     resourceSetReporting.c   \
                     simpQ.c                  \
                     storeMQRecords.c         \
                     topicTree.c              \
                     topicTreeDump.c          \
                     topicTreeRemote.c        \
                     topicTreeRestore.c       \
                     topicTreeRetained.c      \
                     topicTreeStats.c         \
                     topicTreeSubscriptions.c \
                     topicTreeSharedSubs.c    \
                     topicTreeThreads.c       \
                     topicTreeUtils.c         \
                     remoteServers.c          \
                     remoteServersSync.c      \
                     remoteServersRestore.c   \
                     transaction.c            \
                     waiterStatus.c           \
                     copyright.c

UID=$(shell id -u)

ifneq ($(QUALIFY_SHARED),)
cleanupTests:
	$(RM) -rf /var/tmp/unittest_$(UID)_store$(QUALIFY_SHARED)
	$(shell find /var/tmp -name 'unittest_$(UID)_store*' -mtime +4 2>/dev/null -exec rm -rf {} \;)  
	$(RM) -rf /tmp/unittest_$(UID)_store$(QUALIFY_SHARED)
	$(shell find /tmp -name 'unittest_$(UID)_store*' -mtime +4 2>/dev/null -exec rm -rf {} \;)  
	$(RM) -f /dev/shm/com.ibm.ism::$(UID):store$(QUALIFY_SHARED)
	$(shell find /dev/shm -name 'com.ibm.ism::$(UID):store*' -mtime +4 2>/dev/null -exec rm -rf {} \;)  
else
cleanupTests:
	$(RM) -rf /var/tmp/unittest_$(UID)_store
	$(shell find /var/tmp -name 'unittest_$(UID)_store*' -mtime +4 2>/dev/null -exec rm -rf {} \;)  
	$(RM) -rf /tmp/unittest_$(UID)_store
	$(shell find /tmp -name 'unittest_$(UID)_store*' -mtime +4 2>/dev/null -exec rm -rf {} \;)  
	$(RM) -f /dev/shm/com.ibm.ism::$(UID):store
	$(shell find /dev/shm -name 'com.ibm.ism::$(UID):store*' -mtime +4 2>/dev/null -exec rm -rf {} \;)  
endif
	
libismengine-LIBS += libismutil$(SO) libismstore$(SO) libismadmin$(SO) libismtransport$(SO) libSpiderCast$(SO) librum$(SO) libMCP_Routing$(SO)
commontest-FILES = test_utils_initterm.c test_utils_security.c test_utils_file.c test_utils_ismlogs.c \
                   test_utils_log.c test_utils_task.c test_utils_message.c
cunittest-FILES = $(commontest-FILES) test_utils_phases.c                   

LIB-TARGETS += $(LIBDIR)/libismengine$(SO)
$(LIBDIR)/libismengine$(SO): $(call objects, $(libismengine-FILES)) | \
                             $(call libs, $(libismengine-LIBS))
	$(call make-c-library)
	
DEBUG-LIB-TARGETS += $(DEBUG_LIBDIR)/libismengine$(SO)
$(DEBUG_LIBDIR)/libismengine$(SO): $(call debug-objects, $(libismengine-FILES)) | \
                                   $(call debug-libs, $(libismengine-LIBS))
	$(call debug-make-c-library)

COVERAGE-LIB-TARGETS += $(COVERAGE_LIBDIR)/libismengine$(SO)
$(COVERAGE_LIBDIR)/libismengine$(SO): $(call coverage-objects, $(libismengine-FILES)) | \
                                      $(call coverage-libs, $(libismengine-LIBS))
	$(call coverage-make-c-library)

LIB_S-TARGETS += $(LIBDIR_S)/libismengine$(A)
$(LIBDIR_S)/libismengine$(A): $(call objects, $(libismengine-FILES)) 
	$(call make-c-static-library)
	
DEBUG-LIB_S-TARGETS += $(DEBUG_LIBDIR_S)/libismengine$(A)
$(DEBUG_LIBDIR_S)/libismengine$(A): $(call debug-objects, $(libismengine-FILES)) 
	$(call debug-make-c-static-library)

COVERAGE-LIB_S-TARGETS += $(COVERAGE_LIBDIR_S)/libismengine$(A)
$(COVERAGE_LIBDIR_S)/libismengine$(A): $(call coverage-objects, $(libismengine-FILES)) 
	$(call coverage-make-c-static-library)

# ------------------------------------------------
# Build the dump formatting routine library
# ------------------------------------------------

libismdumpfmt-FILES = dumpFormat.c                  \
                      dumpFormatUtils.c             \
                      dumpFormatGenericFormatters.c \
                      dumpFormatCustomFormatters.c
                      

LIB-TARGETS += $(LIBDIR)/libismdumpfmt$(SO)
$(LIBDIR)/libismdumpfmt$(SO): $(call objects, $(libismdumpfmt-FILES)) | \
                              $(call libs, $(libismdumpfmt-LIBS))
	$(call make-c-library)
	
DEBUG-LIB-TARGETS += $(DEBUG_LIBDIR)/libismdumpfmt$(SO)
$(DEBUG_LIBDIR)/libismdumpfmt$(SO): $(call debug-objects, $(libismdumpfmt-FILES)) | \
                                    $(call debug-libs, $(libismdumpfmt-LIBS))
	$(call debug-make-c-library)

COVERAGE-LIB-TARGETS += $(COVERAGE_LIBDIR)/libismdumpfmt$(SO)
$(COVERAGE_LIBDIR)/libismdumpfmt$(SO): $(call coverage-objects, $(libismdumpfmt-FILES)) | \
                                       $(call coverage-libs, $(libismdumpfmt-LIBS))
	$(call coverage-make-c-library)

LIB_S-TARGETS += $(LIBDIR_S)/libismdumpfmt$(A)
$(LIBDIR_S)/libismdumpfmt$(A): $(call objects, $(libismdumpfmt-FILES)) 
	$(call make-c-static-library)
	
DEBUG-LIB_S-TARGETS += $(DEBUG_LIBDIR_S)/libismdumpfmt$(A)
$(DEBUG_LIBDIR_S)/libismdumpfmt$(A): $(call debug-objects, $(libismdumpfmt-FILES)) 
	$(call debug-make-c-static-library)

COVERAGE-LIB_S-TARGETS += $(COVERAGE_LIBDIR_S)/libismdumpfmt$(A)
$(COVERAGE_LIBDIR_S)/libismdumpfmt$(A): $(call coverage-objects, $(libismdumpfmt-FILES)) 
	$(call coverage-make-c-static-library)

# ------------------------------------------------
# Build the dump formatter, note, this can be built
# standalone by compiling in $(libismdumpfmt-FILES)
# rather than linking libismdumpfmt.so.
# ------------------------------------------------

imadumpformatter-FILES = dumpFormatMain.c
imadumpformatter-LIBS = libismdumpfmt.so libismutil.so
imadumpformatter-LDLIBS = $(LDLIBS) -L$(SSL_LIB) -lssl -lcrypto

EXE-TARGETS += $(BINDIR)/imadumpformatter$(EXE)
$(BINDIR)/imadumpformatter$(EXE): $(call objects, $(imadumpformatter-FILES)) | \
                                  $(call libs, $(imadumpformatter-LIBS))
	$(call link-c-program, $(imadumpformatter-LDLIBS))
	
DEBUG-EXE-TARGETS += $(DEBUG_BINDIR)/imadumpformatter$(EXE)
$(DEBUG_BINDIR)/imadumpformatter$(EXE): $(call debug-objects, $(imadumpformatter-FILES)) | \
                                        $(call debug-libs, $(imadumpformatter-LIBS))
	$(call debug-link-c-program, $(imadumpformatter-LDLIBS))
	
COVERAGE-EXE-TARGETS += $(COVERAGE_BINDIR)/imadumpformatter$(EXE)
$(COVERAGE_BINDIR)/imadumpformatter$(EXE): $(call coverage-objects, $(imadumpformatter-FILES)) | \
                                           $(call coverage-libs, $(imadumpformatter-LIBS))
	$(call coverage-link-c-program, $(imadumpformatter-LDLIBS))

# ------------------------------------------------
# Build a utility to show contents of an encrypted file
# (primarily for the displaying of diagnostics)
# ------------------------------------------------

showCrypto-FILES = showCrypto.c \
                   exportCrypto.c
showCrypto-LDLIBS = $(LDLIBS) -L$(SSL_LIB) -lssl -lcrypto
showCrypto-LIBS = libismutil$(SO)

EXE-TARGETS += $(BINDIR)/showCrypto$(EXE) 
$(BINDIR)/showCrypto$(EXE): $(call objects, $(showCrypto-FILES)) \
                            $(call libs, $(showCrypto-LIBS))
	$(call link-c-program, $(showCrypto-LDLIBS))

DEBUG-EXE-TARGETS += $(DEBUG_BINDIR)/showCrypto$(EXE)
$(DEBUG_BINDIR)/showCrypto$(EXE): $(call debug-objects, $(showCrypto-FILES)) | \
                                  $(call debug-libs, $(showCrypto-LIBS))
	$(call debug-link-c-program, $(showCrypto-LDLIBS))
	
COVERAGE-EXE-TARGETS += $(COVERAGE_BINDIR)/showCrypto$(EXE)
$(COVERAGE_BINDIR)/showCrypto$(EXE): $(call coverage-objects, $(showCrypto-FILES)) | \
                                     $(call coverage-libs, $(showCrypto-LIBS))
	$(call coverage-link-c-program, $(showCrypto-LDLIBS))

# ------------------------------------------------
# Build diagnostics (Selective Trace)
# ------------------------------------------------

libseltrace-FILES = selectiveTrace.c

LIB-TARGETS += $(LIBDIR)/libseltrace$(SO)
$(LIBDIR)/libseltrace$(SO): $(call objects, $(libseltrace-FILES)) | \
                            $(call libs, $(libseltrace-LIBS))
	$(call make-c-library)

DEBUG-LIB-TARGETS += $(DEBUG_LIBDIR)/libseltrace$(SO)
$(DEBUG_LIBDIR)/libseltrace$(SO): $(call debug-objects, $(libseltrace-FILES)) | \
                                  $(call debug-libs, $(libseltrace-LIBS))
	$(call debug-make-c-library)

COVERAGE-LIB-TARGETS += $(COVERAGE_LIBDIR)/libseltrace$(SO)
$(COVERAGE_LIBDIR)/libseltrace$(SO): $(call coverage-objects, $(libseltrace-FILES)) | \
                                     $(call coverage-libs, $(libseltrace-LIBS))
	$(call coverage-make-c-library)

# ------------------------------------------------
# Build diagnostics (Fake Async Store Commit)
# ------------------------------------------------

libfakeasyncstore-FILES = fakeAsyncStore.c

LIB-TARGETS += $(LIBDIR)/libfakeasyncstore$(SO)
$(LIBDIR)/libfakeasyncstore$(SO): $(call objects, $(libfakeasyncstore-FILES)) | \
                                  $(call libs, $(libfakeasyncstore-LIBS))
	$(call make-c-library)

DEBUG-LIB-TARGETS += $(DEBUG_LIBDIR)/libfakeasyncstore$(SO)
$(DEBUG_LIBDIR)/libfakeasyncstore$(SO): $(call debug-objects, $(libfakeasyncstore-FILES)) | \
                                        $(call debug-libs, $(libfakeasyncstore-LIBS))
	$(call debug-make-c-library)

COVERAGE-LIB-TARGETS += $(COVERAGE_LIBDIR)/libfakeasyncstore$(SO)
$(COVERAGE_LIBDIR)/libfakeasyncstore$(SO): $(call coverage-objects, $(libfakeasyncstore-FILES)) | \
                                           $(call coverage-libs, $(libfakeasyncstore-LIBS))
	$(call coverage-make-c-library)

# ------------------------------------------------
# Build the unit tests
# ------------------------------------------------

test-LIBS = $(libismengine-LIBS) libismengine$(SO)

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

$(eval $(call build-c-tests, $(FULLTEST), testTopicTree, \
                                          testTopicTree.c $(commontest-FILES), \
                                          $(test-LIBS)))
$(eval $(call build-c-tests, $(BASETEST), testPubSubCfgs, \
                                          testPubSubCfgs.c testPubSubThreads.c $(commontest-FILES), \
                                          $(test-LIBS)))
$(eval $(call build-c-tests, $(BASETEST), testPubSubMemLowCfgs, \
                                          testPubSubMemLowCfgs.c testPubSubThreads.c $(commontest-FILES), \
                                          $(test-LIBS)))
$(eval $(call build-c-tests, $(BASETEST), testPubSubRestart, \
                                          testPubSubRestart.c testPubSubRestartPersistentState.c $(commontest-FILES) \
                                          test_utils_sync.c, \
                                          $(test-LIBS)))
$(eval $(call build-c-tests, $(BASETEST), testClientStateMonitor, \
                                          testClientStateMonitor.c $(commontest-FILES) \
                                          test_utils_sync.c, \
                                          $(test-LIBS)))
$(eval $(call build-c-tests, $(BASETEST), testClientStateRecovery, \
                                          testClientStateRecovery.c $(commontest-FILES) \
                                          test_utils_sync.c, \
                                          $(test-LIBS)))
$(eval $(call build-c-tests, $(BASETEST), testLockManager, \
                                          testLockManager.c $(commontest-FILES), \
                                          $(test-LIBS)))
$(eval $(call build-c-tests, $(BASETEST), testStoreMQRecovery, \
                                          testStoreMQRecovery $(commontest-FILES) \
                                          test_utils_sync.c, \
                                          $(test-LIBS)))
$(eval $(call build-c-tests, $(FULLTEST), testTopicTreeScenarios, \
                                          testTopicTreeScenarios.c testPubSubThreads.c $(commontest-FILES) \
                                          test_utils_sync.c, \
                                          $(test-LIBS)))
$(eval $(call build-c-tests, $(BASETEST), testConsumer, \
                                          testConsumer.c $(commontest-FILES), \
                                          $(test-LIBS)))
$(eval $(call build-c-tests, $(BASETEST), testSubMonitor, \
                                          testSubMonitor.c $(commontest-FILES), \
                                          $(test-LIBS)))
$(eval $(call build-c-tests, $(BASETEST), testQueueMonitor, \
                                          testQueueMonitor.c $(commontest-FILES), \
                                          $(test-LIBS)))
$(eval $(call build-c-tests, $(FULLTEST), testStoreEvents, \
                                          testStoreEvents.c $(commontest-FILES) \
                                          test_utils_sync.c, \
                                          $(test-LIBS)))
$(eval $(call build-c-tests, $(FULLTEST), multi_prod_node_msg, \
                                          multi_prod_node_msg.c $(commontest-FILES), \
                                          $(test-LIBS))) 
$(eval $(call build-c-tests, $(FULLTEST), simple_thread_test, \
                                          simple_thread_test.c $(commontest-FILES), \
                                          $(test-LIBS))) 
$(eval $(call build-c-tests, $(FULLTEST), multi_thread_QoS1, \
                                          multi_thread_QoS1.c $(commontest-FILES), \
                                          $(test-LIBS))) 
$(eval $(call build-c-tests, $(FULLTEST), 1M_Client_test, \
                                          1M_Client_test.c $(commontest-FILES) \
                                          test_utils_sync, \
                                          $(test-LIBS))) 
$(eval $(call build-c-tests, $(FULLTEST), Cars_HQ_multiThreads, \
                                          Cars_HQ_multiThreads.c \
                                          test_utils_sync.c \
                                          $(commontest-FILES), \
                                          $(test-LIBS))) 
$(eval $(call build-c-tests, $(FULLTEST), Car_HQ_TwoWayMsgs_multiThread, \
                                          Car_HQ_TwoWayMsgs_multiThread.c \
                                          test_utils_sync \
                                          $(commontest-FILES), \
                                          $(test-LIBS)))
$(eval $(call build-c-tests, $(BASETEST), testMixedRestart, \
                                          testMixedRestart.c test_utils_sync.c \
                                          $(commontest-FILES), \
                                          $(test-LIBS)))
$(eval $(call build-c-tests, $(PRIVTEST), testEngineStartup, \
                                          testEngineStartup.c \
                                          test_utils_config.c \
                                          $(commontest-FILES), \
                                          $(test-LIBS)))
$(eval $(call build-c-tests, $(PRIVTEST), testHashAlgorithm, \
                                          testHashAlgorithm.c \
                                          $(commontest-FILES), \
                                          $(test-LIBS)))
$(eval $(call build-c-tests, $(BASETEST), testLateOwner, \
                                          testLateOwner.c \
                                          test_utils_sync.c \
                                          test_utils_client.c \
                                          $(commontest-FILES), \
                                          $(test-LIBS)))
$(eval $(call build-c-tests, $(BASETEST), testRehydrateInflightMsgs, \
                                          testRehydrateInflightMsgs.c \
                                          test_utils_sync.c \
                                          test_utils_client.c \
                                          $(commontest-FILES), \
                                          $(test-LIBS)))
$(eval $(call build-c-tests, $(BASETEST), testMoreLateThings, \
                                          testMoreLateThings.c test_utils_client.c \
                                          $(commontest-FILES) \
                                          test_utils_sync.c, \
                                          $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_topicTree, \
                                              test_topicTree.c \
                                              test_utils_sync.c \
                                              $(cunittest-FILES) \
                                              test_utils_client.c, \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_topicTreeDurable, \
                                              test_topicTreeDurable.c \
                                              test_utils_sync.c test_utils_config.c \
                                              $(cunittest-FILES) \
                                              test_utils_client.c, \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_sharedSubs, \
                                              test_sharedSubs.c $(cunittest-FILES) \
                                              test_utils_client.c \
                                              test_utils_sync.c \
                                              test_utils_pubsuback.c \
                                              test_utils_config.c, \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_updateSub, \
                                              test_updateSub.c $(cunittest-FILES) \
                                              test_utils_client.c \
                                              test_utils_sync.c, \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_retainedMsgs, \
                                              test_retainedMsgs.c $(cunittest-FILES) \
                                              test_utils_client.c \
                                              test_utils_sync.c, \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_queue1, \
                                              test_queue1.c $(cunittest-FILES), \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_queue3, \
                                              test_queue3.c \
                                              $(cunittest-FILES) \
                                              test_utils_sync.c, \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_queue4, \
                                              test_queue4.c \
                                              $(cunittest-FILES) \
                                              test_utils_sync.c, \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_queue4a, \
                                              test_queue4a.c \
                                              $(cunittest-FILES) \
                                              test_utils_sync.c, \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_clientStateRestart, \
                                              test_clientStateRestart.c $(cunittest-FILES) \
                                              test_utils_sync.c, \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_PMR27280.499.000, \
                                              test_PMR27280.499.000.c $(cunittest-FILES) \
                                              test_utils_client.c \
                                              test_utils_sync.c, \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_jobQueue, \
                                              test_jobQueue.c \
                                              $(cunittest-FILES), \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_flow, \
                                              test_flow.c $(cunittest-FILES) \
                                              test_utils_sync.c, \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_engine, \
                                              test_engine.c \
                                              test_clientState.c \
                                              test_qos0.c \
                                              test_qos0ExplicitSuspend.c \
                                              test_qos1.c \
                                              test_qos2.c \
                                              test_storeMQRecords.c \
                                              test_willMessage.c \
                                              test_batchAcks.c \
                                              test_SCRMigration.c \
                                              test_utils_sync.c \
                                              test_lockManager.c \
                                              test_engineTimers.c \
                                              test_utils_client.c \
                                              test_utils_pubsuback.c \
                                              $(cunittest-FILES), \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_memHandler, \
                                              test_memHandler.c $(cunittest-FILES), \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_queue2, \
                                              test_queue2.c $(cunittest-FILES) \
                                              test_utils_sync.c, \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_globalTran, \
                                              test_globalTran.c \
                                              $(cunittest-FILES) \
                                              test_utils_sync.c \
                                              test_utils_client.c, \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_tranRestart,   \
                                              test_tranRestart.c  \
                                              $(cunittest-FILES) \
                                              test_utils_sync.c,  \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_transaction, \
                                              test_transaction.c $(cunittest-FILES) \
                                              test_utils_file.c \
                                              test_utils_sync.c, \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_transactionRecovery, \
                                              test_transactionRecovery.c $(cunittest-FILES) \
                                              test_utils_client.c \
                                              test_utils_file.c \
                                              test_utils_sync.c, \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_session, \
                                              test_session.c $(cunittest-FILES) \
                                              test_utils_sync.c, \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_restoreTable, \
                                              test_restoreTable.c $(cunittest-FILES) \
                                              test_utils_file.c, \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_namedQueues, \
                                              test_namedQueues.c $(cunittest-FILES) \
                                              test_utils_client.c test_utils_config.c \
                                              test_utils_sync.c, \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_temporaryQueues, \
                                              test_temporaryQueues.c $(cunittest-FILES) \
                                              test_utils_client.c test_utils_config.c \
                                              test_utils_sync.c, \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_awkwardQDelete, \
                                              test_awkwardQDelete.c $(cunittest-FILES) \
                                              test_utils_preload.c \
                                              test_utils_sync.c \
                                              test_utils_client.c, \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_31356, \
                                              test_31356.c $(cunittest-FILES) \
                                              test_utils_sync.c \
                                              test_utils_client.c, \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_queueMsgExpiry, \
                                              test_queueMsgExpiry.c $(cunittest-FILES) \
                                              test_utils_client.c \
                                              test_utils_sync.c, \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_waiterStatus, \
                                              test_waiterStatus.c $(cunittest-FILES) \
                                              test_utils_client.c \
                                              test_utils_sync.c, \
                                              $(test-LIBS)))                                              
$(eval $(call build-cunit-tests, $(BASETEST), test_multiConsumerQ, \
                                              test_multiConsumerQ.c \
                                              test_utils_client.c \
                                              test_utils_preload.c \
                                              test_utils_sync.c \
                                              $(cunittest-FILES), \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_policyInfo, \
                                              test_policyInfo.c $(cunittest-FILES) \
                                              test_utils_client.c \
                                              test_utils_config.c \
                                              test_utils_sync.c, \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(FULLTEST), test_failedAllocs, \
                                              test_failedAllocs.c $(cunittest-FILES) \
                                              test_utils_memory.c \
                                              test_utils_client.c \
                                              test_utils_sync.c, \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_subscriptionMonitor, \
                                              test_subscriptionMonitor.c $(cunittest-FILES) \
                                              test_utils_client.c \
                                              test_utils_sync.c, \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_topicMonitor, \
                                              test_topicMonitor.c $(cunittest-FILES) \
                                              test_utils_client.c test_utils_config.c \
                                              test_utils_sync.c, \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_topicMonitorRecovery, \
                                              test_topicMonitorRecovery.c $(cunittest-FILES) \
                                              test_utils_client.c test_utils_config.c \
                                              test_utils_sync.c, \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(PRIVTEST), test_engineStatsCLI, \
                                              test_engineStatsCLI $(cunittest-FILES) \
                                              test_utils_monitoring.c \
                                              test_utils_client.c \
                                              test_utils_sync.c, \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_systemTopic, \
                                              test_systemTopic.c $(cunittest-FILES) \
                                              test_utils_client.c \
                                              test_utils_sync.c, \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(FULLTEST), test_dumpFormatting, \
                                              test_dumpFormatting.c  $(cunittest-FILES), \
                                              $(test-LIBS) $(imadumpformatter-LIBS)))
$(eval $(call build-cunit-tests, $(FULLTEST), test_mempool, \
                                              test_mempool.c  $(cunittest-FILES), \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(FULLTEST), test_asyncInfrastructure, \
                                              test_asyncInfrastructure.c  $(cunittest-FILES), \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(FULLTEST), test_asyncPublish, \
                                              test_asyncPublish.c  $(cunittest-FILES) \
                                              test_utils_client.c \
                                              test_utils_sync.c   \
                                              test_utils_asyncPause.c, \
                                              $(test-LIBS) $(imadumpformatter-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_SessionWideMQTTFlowControl, \
                                              test_SessionWideMQTTFlowControl.c  $(cunittest-FILES) \
                                              test_utils_client.c \
                                              test_utils_sync.c, \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_disconnectedClientNotification, \
                                              test_disconnectedClientNotification.c $(cunittest-FILES) \
                                              test_utils_client.c \
                                              test_utils_sync.c, \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_discardOldMsgs, \
                                              test_discardOldMsgs.c \
                                              test_utils_sync.c \
                                              test_utils_client.c \
                                              $(cunittest-FILES), \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_sharedMQTTFlow, \
                                              test_sharedMQTTFlow.c \
                                              test_utils_client.c \
                                              test_utils_sync.c \
                                              $(cunittest-FILES), \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_remoteServers, \
                                              test_remoteServers.c \
                                              test_utils_client.c \
                                              test_utils_sync.c \
                                              $(cunittest-FILES), \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_remoteServersSync, \
                                              test_remoteServersSync.c \
                                              test_utils_client.c \
                                              test_utils_sync.c \
                                              $(cunittest-FILES), \
                                              $(test-LIBS)))
$(eval $(call build-c-tests, $(PRIVTEST), testCGroups, \
                                          testCGroups.c \
                                          $(commontest-FILES), \
                                          $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_expiringGet, \
                                              test_expiringGet.c \
                                              test_utils_client.c \
                                              test_utils_sync.c \
                                              $(cunittest-FILES), \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_exportCrypto,  \
                                              test_exportCrypto.c \
                                              test_utils_client.c \
                                              test_utils_sync.c \
                                              $(cunittest-FILES), \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_exportResources, \
                                              test_exportResources.c \
                                              test_utils_client.c \
                                              test_utils_sync.c \
                                              $(cunittest-FILES), \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_exportBufferedMsgs, \
                                              test_exportBufferedMsgs.c \
                                              test_utils_sync.c \
                                              test_utils_client.c \
                                              $(cunittest-FILES), \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_mqttPersistSharedSubs, \
                                              test_mqttPersistSharedSubs.c \
                                              test_utils_sync.c \
                                              test_utils_client.c \
                                              test_utils_pubsuback.c \
                                              $(cunittest-FILES), \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_fullCleanPagesScan, \
                                              test_fullCleanPagesScan.c \
                                              test_utils_sync.c \
                                              test_utils_client.c \
                                              test_utils_pubsuback.c \
                                              $(cunittest-FILES), \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_nonAcker, \
                                              test_nonAcker.c \
                                              test_utils_sync.c \
                                              test_utils_client.c \
                                              test_utils_pubsuback.c \
                                              $(cunittest-FILES), \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_183547, \
                                              test_183547.c \
                                              test_utils_sync.c \
                                              test_utils_client.c \
                                              test_utils_pubsuback.c \
                                              $(cunittest-FILES), \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_diagnostics, \
                                              test_diagnostics.c \
                                              test_diagnosticsFileContent.c \
                                              test_utils_sync.c \
                                              $(cunittest-FILES), \
                                              $(test-LIBS)))
$(eval $(call build-c-tests, $(BASETEST), testBadStore, \
                                          testBadStore.c \
                                          test_utils_config.c \
                                          test_utils_client.c \
                                          test_utils_sync.c \
                                          $(commontest-FILES), \
                                          $(test-LIBS)))
$(eval $(call build-cunit-tests, $(FULLTEST), test_clientStateExpiry, \
                                              test_clientStateExpiry.c $(cunittest-FILES) \
                                              test_utils_client.c \
                                              test_utils_sync.c, \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_clientStateRecovery, \
                                              test_clientStateRecovery.c $(cunittest-FILES) \
                                              test_utils_client.c \
                                              test_utils_sync.c, \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(FULLTEST), test_adminDeleteSubWithMDRs, \
                                              test_adminDeleteSubWithMDRs.c $(cunittest-FILES) \
                                              test_utils_client.c \
                                              test_utils_pubsuback.c \
                                              test_utils_sync.c, \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(PRIVTEST), test_recoveryPerf, \
                                              test_recoveryPerf.c $(cunittest-FILES) \
                                              test_utils_client.c \
                                              test_utils_message.c \
                                              test_utils_sync.c, \
                                              $(test-LIBS)))
$(eval $(call build-c-tests, $(PRIVTEST), testFillSubs, \
                                          testFillSubs.c \
                                          test_utils_sync.c \
                                          test_utils_client.c \
                                          $(commontest-FILES), \
                                          $(test-LIBS)))
$(eval $(call build-c-tests, $(PRIVTEST), testBadFillSubs, \
                                          testBadFillSubs.c \
                                          test_utils_sync.c \
                                          test_utils_client.c \
                                          $(commontest-FILES), \
                                          $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_resourceSets, \
                                              test_resourceSets.c $(cunittest-FILES) \
                                              test_utils_client.c \
                                              test_utils_sync.c \
                                              test_utils_config.c, \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(BASETEST), test_clusterRequestedTopicsRecovery, \
                                              test_clusterRequestedTopicsRecovery.c $(cunittest-FILES) \
                                              test_utils_config.c test_utils_sync.c, \
                                              $(test-LIBS)))
$(eval $(call build-cunit-tests, $(FULLTEST), test_startStop, \
                                              test_startStop.c \
                                              test_utils_config.c \
                                              $(cunittest-FILES), \
                                              $(test-LIBS)))

# Run test_disconnectedClientNotification in 'FULL' mode when run as part of full test
CUNIT_FULL += cunit_test_disconnectedClientNotification_full
DEBUG_CUNIT_FULL += cunit_test_disconnectedClientNotification_debug_full
COVERAGE_CUNIT_FULL += cunit_test_disconnectedClientNotification_coverage_full

# Experimental source - may not get integrated in final product
# EXP-TARGETS += $(BINDIR)/twolockqueue$(EXE)
# $(BINDIR)/twolockqueue$(EXE): $(call objects, twolockqueue.c $(commontest-FILES)) \
#                               $(call libs, $(test-LIBS))
# 	$(call build-c-test)

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
         
