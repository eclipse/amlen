// Copyright (c) 2010-2021 Contributors to the Eclipse Foundation
// 
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// 
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0
// 
// SPDX-License-Identifier: EPL-2.0
//
/*
 * MembershipManagerImpl.cpp
 *
 *  Created on: 16/03/2010
 */

#include <cstdlib> //for rand(), drand48(),
#include <cassert>

#include "FirstViewDeliveryTask.h"
#include "MembershipPeriodicTask.h"
#include "MembershipTerminationTask.h"
#include "MembershipTermination2Task.h"
#include "ChangeOfMetadataDeliveryTask.h"
#include "ClearRetainAttrTask.h"
#include "LEWarmupTask.h"

#include "MembershipManagerImpl.h"


namespace spdr
{

using namespace trace;

ScTraceComponent* MembershipManagerImpl::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Mem,
		trace::ScTrConstants::Layer_ID_Membership,
		"MembershipManagerImpl",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

/*
 * Threading: App-thread
 */
MembershipManagerImpl::MembershipManagerImpl(
		const String& instID,
		const SpiderCastConfigImpl& config,
		NodeIDCache& nodeIDCache,
		VirtualIDCache& vidCache,
		CoreInterface& coreInterface) :
		ScTraceContext(tc_,instID,config.getMyNodeID()->getNodeName()),
		_instID(instID),
		_config(config),
		_coreInterface(coreInterface),
		_nodeIDCache(nodeIDCache),
		_nodeVirtualIDCache(vidCache),
		_started(false),
		_closed(false),
		_close_soft(false),
		_close_remove_retained_att(false),
		_close_done(false),
		_close_remove_retained_att_ack(false),
		_first_periodic_task(true),
		bootstrap(new BootstrapMultimap(_instID,_config.getBootstrapSet(),_config.getMyNodeID(), _nodeVirtualIDCache,
				_config.isFullViewBootstrapSet())),
//		bootstrapSet(_instID,_config.getBootstrapSet(),_config.getMyNodeID(), _nodeVirtualIDCache,
//				_config.isFullViewBootstrapSet()),
		bootstrapStart(boost::posix_time::not_a_date_time),
		myID(_config.getMyNodeID()),
		myVID(_nodeVirtualIDCache.get(myID->getNodeName())),
		myVersion(_coreInterface.getIncarnationNumber(), (int64_t)1),
		outgoingMemMessage(),
		outgoingAttMessage(),
		viewMap(),
		ringSet(),
		updateDB(instID, myID->getNodeName(), config),
		attributeManager(instID, config, viewMap, nodeHistorySet.getNodeHistoryMap(), myID, myVersion, nodeIDCache,coreInterface),
		highPriorityMonitor_SPtr(),
		leaderElectionViewKeeper_SPtr(),
		leaderElectionCandidate_SPtr(),
		intMemConsumer_Vec(MembershipManager::IntMemConsumer_Max),
		intMemConsumer_FirstViewDelivered(false)
{
	Trace_Entry(this,"MembershipManagerImpl()");

	outgoingMemMessage = SCMessage_SPtr(new SCMessage);
	(*outgoingMemMessage).setBuffer(ByteBuffer::createByteBuffer(1024));
	outgoingAttMessage = SCMessage_SPtr(new SCMessage);
	(*outgoingAttMessage).setBuffer(ByteBuffer::createByteBuffer(1024));

	srand(static_cast<unsigned int>(_coreInterface.getIncarnationNumber()));
	srand48(static_cast<long int>(_coreInterface.getIncarnationNumber()));

	//do not call additional method on coreInterface in ctor

	Trace_Exit(this,"MembershipManagerImpl()");
}

MembershipManagerImpl::~MembershipManagerImpl()
{
	Trace_Entry(this,"~MembershipManagerImpl()");
}

/*
 * Threading: App-thread
 */
void MembershipManagerImpl::init()
{
	Trace_Entry(this,"init()");

	topoMgr_SPtr = _coreInterface.getTopologyManager();
	neighborTable_SPtr = topoMgr_SPtr->getNeighborTable();
	taskSchedule_SPtr = _coreInterface.getTopoMemTaskSchedule();

	periodicTask_SPtr = AbstractTask_SPtr(
			new MembershipPeriodicTask(_coreInterface));

	historyPruneTask_SPtr = AbstractTask_SPtr(
				new NodeHistoryPruneTask(
						_instID,
						nodeHistorySet,
						taskSchedule_SPtr,
						_config.getNodeHistoryRetentionTimeSec(),
						historyPruneTask_SPtr ));

	if (_config.isHighPriorityMonitoringEnabled())
	{
		highPriorityMonitor_SPtr.reset(new HighPriorityMonitor(_instID,_config, _coreInterface.getCommAdapter()));
		registerInternalMembershipConsumer(
				boost::static_pointer_cast<SCMembershipListener>(highPriorityMonitor_SPtr),
				IntMemConsumer_HighPriorityMonitor);
	}

	if (_config.isLeaderElectionEnabled())
	{
		leaderElectionViewKeeper_SPtr.reset(new leader_election::LEViewKeeper(_instID,_config));
		registerInternalMembershipConsumer(
				boost::static_pointer_cast<SCMembershipListener>(leaderElectionViewKeeper_SPtr),
				IntMemConsumer_LeaderElection);
	}

	Trace_Exit(this,"init()");
}

void MembershipManagerImpl::destroyCrossRefs()
{
	Trace_Entry(this,"destroyCrossRefs()");
	//reset all shared pointers to break cycles.

	topoMgr_SPtr.reset();
	taskSchedule_SPtr.reset();
	periodicTask_SPtr.reset();
	historyPruneTask_SPtr.reset();
	intMemConsumer_Vec.clear();
	highPriorityMonitor_SPtr.reset();
	leaderElectionViewKeeper_SPtr.reset();

	Trace_Exit(this,"destroyCrossRefs()");
}

void MembershipManagerImpl::reportStats(boost::posix_time::ptime time, bool labels)
{
	attributeManager.reportStats(time,labels);

	if (ScTraceBuffer::isConfigEnabled(tc_))
	{
		std::string time_str(boost::posix_time::to_iso_extended_string(time));

		std::ostringstream oss;
		oss << std::endl;
		if (labels)
		{
			oss << _instanceID << ", " << time_str << ", SC_Stats_Membership, ViewSize, HistSize" << std::endl;
		}
		else
		{
			oss << _instanceID << ", " << time_str << ", SC_Stats_Membership, " << viewMap.size() << ", " << nodeHistorySet.size() << std::endl;

		}

		ScTraceBufferAPtr buffer = ScTraceBuffer::config(this, "reportStats()", oss.str());
		buffer->invoke();
	}

}

/*
 * Threading: App-thread
 */
void MembershipManagerImpl::start()
{
	Trace_Entry(this,"start()");

	{
		boost::recursive_mutex::scoped_lock lock(membership_mutex);
		_started = true;
	}

	//insert myself to the view
	//viewAddNode(myID, myVersion); // let the first periodic task do it

	taskSchedule_SPtr->scheduleDelay(
			periodicTask_SPtr,
			TaskSchedule::ZERO_DELAY);

	//Never expire nodes from history if RetainAttr is true
	if (_config.isRetainAttributesOnSuspectNodesEnabled())
	{
		taskSchedule_SPtr->scheduleDelay(
				historyPruneTask_SPtr,
				boost::posix_time::seconds(_config.getNodeHistoryRetentionTimeSec()/2));
	}

//	if (_config.isFullViewBootstrapSet() &&
//			_config.getFullViewBootstrapSetPolicy() == config::FullViewBootstrapPolicy_Tree_VALUE)
//	{
//		bootstrapSet.setPolicy(config::FullViewBootstrapPolicy_Tree_VALUE);
//		bootstrapStart = boost::posix_time::microsec_clock::universal_time();
//	}

	Trace_Exit(this,"start()");
}

/*
 * Called from SpiderCastImpl.close()
 *
 * Threading: App / Delivery (normal), or any failing thread
 */
bool MembershipManagerImpl::terminate(bool soft, bool removeRetained, int timeout_millis)
{
	if (ScTraceBuffer::isEventEnabled(tc_))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::event(this, "terminate()");
		buffer->addProperty<bool>("soft", soft);
		buffer->addProperty<bool>("removeRetained", removeRetained);
		buffer->addProperty<int>("timeout_millis", timeout_millis);
		buffer->invoke();
	}

	bool do_termination_task = true;
	bool ack_received = false;

	{
		boost::recursive_mutex::scoped_lock lock(membership_mutex);

		_closed = true;
		_close_done = false;
		_close_soft = soft;
		_close_remove_retained_att = removeRetained; //RetainAtt

		//close membership service
		if (membershipServiceImpl_SPtr)
		{
			membershipServiceImpl_SPtr->internalClose();
			membershipServiceImpl_SPtr.reset();
		}

		if (!_started)
			do_termination_task = false;
	}

	if (do_termination_task)
	{
		//schedule a termination task to send leave messages and clean-up
		taskSchedule_SPtr->scheduleDelay(
				AbstractTask_SPtr(
						new MembershipTerminationTask(_coreInterface)),
				TaskSchedule::ZERO_DELAY);

		if (soft && !removeRetained)
		{
			taskSchedule_SPtr->scheduleDelay(
					AbstractTask_SPtr(
							new MembershipTermination2Task(_coreInterface)),
							boost::posix_time::milliseconds( (timeout_millis>0 ? timeout_millis : 1) ));
		}

		//FIXME what happens here if the closing thread is the MemTopo? (if soft/retained: blocks, no leave sent; if retained - no ack as well)
		if (soft || removeRetained)
		{
			Trace_Event(this, "terminate()", "Waiting Leave_Ack / TerminationGrace");

			boost::posix_time::ptime time_out =
					boost::posix_time::microsec_clock::universal_time()
							+ boost::posix_time::milliseconds(timeout_millis);
			try
			{
				boost::recursive_mutex::scoped_lock lock(membership_mutex);
				while (!_close_done
						&& (boost::posix_time::microsec_clock::universal_time()
								< time_out))
				{
					_close_done_condition_var.timed_wait(lock,
							boost::posix_time::milliseconds(timeout_millis));
				}

				ack_received = _close_remove_retained_att_ack;
			}
			catch (boost::thread_interrupted& ex)
			{
				Trace_Event(this, "terminate()()",
						"Interrupted while waiting for LeaveAck", "id",
						ScTraceBuffer::stringValueOf<boost::thread::id>(
								boost::this_thread::get_id()));
			}
		}
	}

	Trace_Exit<bool>(this, "terminate()", ack_received);
	return ack_received;
}

/*
 * Called by MembershipTerminationTask,
 * Threading: MemTopo-thread
 */
void MembershipManagerImpl::terminationTask()
{
	Trace_Entry(this, "terminationTask()");

	bool soft = false;
	bool remove_retained = false;

	{
		boost::recursive_mutex::scoped_lock lock(membership_mutex);
		if (!_closed) //must be closed to get here
		{
			std::string what("Error: Termination task must be after close, but _closed=false");
			Trace_Error(this, "terminationTask()", what);
			throw SpiderCastRuntimeError(what);
			//assert(_closed);
		}


		soft = _close_soft;
		remove_retained = _close_remove_retained_att;
	}

	//if soft, send leave messages
	if (soft || remove_retained)
	{
		Trace_Event(this, "terminationTask()","Sending leave messages");
		int32_t exitCode = static_cast<int32_t>(remove_retained ? spdr::event::STATUS_REMOVE : spdr::event::STATUS_LEAVE);
		sendLeaveMsg(exitCode);
	}

	//membership service is closed by now.

	//cancel all tasks
	periodicTask_SPtr->cancel();
	historyPruneTask_SPtr->cancel();

	{
		boost::recursive_mutex::scoped_lock lock(membership_mutex);
		if (!remove_retained)
		{
			_close_done = true;
			_close_done_condition_var.notify_all();
		}
	}

	Trace_Event(this, "terminationTask()","Executed");

	Trace_Exit(this, "terminationTask()");
}

/*
 * Called by MembershipTermination2Task,
 * Threading: MemTopo-thread
 */
void MembershipManagerImpl::terminationGraceTask()
{
	Trace_Entry(this, "terminationGraceTask()");

	{
		boost::recursive_mutex::scoped_lock lock(membership_mutex);
		if (_close_soft && !_close_remove_retained_att)
		{
			_close_done = true;
			_close_done_condition_var.notify_all();

			Trace_Event(this, "terminationGraceTask()","done, notified all");
		}
	}

	Trace_Exit(this, "terminationGraceTask()");
}

void MembershipManagerImpl::clearRetainAttrTask()
{
	Trace_Entry(this, "clearRetainAttrTask()");

	{
		boost::recursive_mutex::scoped_lock lock(membership_mutex);
		if (!_closed)
		{
			while (!clearRetainAttrQ_.empty())
			{
				std::pair<NodeIDImpl_SPtr,int64_t> remove  = clearRetainAttrQ_.front();
				clearRetainAttrQ_.pop_front();
				bool changed = nodeHistorySet.forceRemoveRetained(remove.first,remove.second);
				if (changed)
				{
					spdr::pair<NodeInfo,bool> info = nodeHistorySet.getNodeInfo(remove.first);
					if (info.second)
					{
						bool added = updateDB.addToRetained(remove.first, info.first.nodeVersion, spdr::event::STATUS_REMOVE);
						Trace_Event(this, "clearRetainAttrTask()",added ? "added to updateDB" : "not added to updateDB");
						notifyLeave(remove.first,info.first.nodeVersion, spdr::event::STATUS_REMOVE, info.first.attributeTable);
					}
				}
			}
		}
	}

	Trace_Exit(this, "clearRetainAttrTask()");
}
NodeIDImpl_SPtr MembershipManagerImpl::getRandomNode()
{
	Trace_Entry(this, "getRandomNode");

	bool skip_closed = false;
	{
		boost::recursive_mutex::scoped_lock lock(membership_mutex);
		if (_closed)
		{
			skip_closed = true;
		}
	}

	if (skip_closed)
	{
		Trace_Exit(this, "getRandomNode", "skip-closed");
		return NodeIDImpl_SPtr();
	}

	NodeIDImpl_SPtr id;
	int size = viewMap.size();

	//just for testing
	//	if (size>3)
	//	{
	//		return getRandomizedStructuredNode().first;
	//	}

	if ( size > 1 )
	{
		//=========================================================
		//		while (!id)
		//		{
		//			int r = rand() % size; //[0,size-1]
		//			MembershipViewMap::iterator it = viewMap.begin();
		//			while (r > 0)
		//			{
		//				--r;
		//				++it;
		//			}
		//			//do it again if it is me...
		//			if ( (*(it->first)) != (*myID) )
		//			{
		//				id = it->first;
		//			}
		//		}

		//==========================================================
		//		int r = (rand() % (size-1)); //[0,size-2]
		//		MembershipViewMap::iterator it = viewMap.begin();
		//
		//		if ((*(it->first)) == (*myID))
		//		{
		//			++it;
		//		}
		//
		//		while (r>0)
		//		{
		//			if ((*(it->first)) == (*myID))
		//			{
		//				++it;
		//			}
		//
		//			--r;
		//			++it;
		//		}
		//
		//		id = it->first;

		MembershipViewMap::iterator it = viewMap.find(myID);
		if (it == viewMap.end())
		{
			throw SpiderCastRuntimeError("my ID cannot be found in the view");
		}

		int r = (rand() % (size-1))+1; //[1,size-1]
		while (r>0)
		{
			--r;
			++it;
			if (it == viewMap.end())
			{
				it = viewMap.begin();
			}
		}

		id = it->first;
	}


	Trace_Exit(this, "getRandomNode", "node", NodeIDImpl::stringValueOf(id));
	return id;
}

std::pair<NodeIDImpl_SPtr, int> MembershipManagerImpl::getRandomizedStructuredNode()
{
	using namespace spdr::util;

	Trace_Entry(this, "getRandomizedStructuredNode");

	bool skip_closed = false;
	{
		boost::recursive_mutex::scoped_lock lock(membership_mutex);
		if (_closed)
		{
			skip_closed = true;
		}
	}

	if (skip_closed)
	{
		Trace_Exit(this, "getRandomizedStructuredNode", "skip-closed");
		return std::make_pair(NodeIDImpl_SPtr(), 1);
	}

	int size = viewMap.size();
	NodeIDImpl_SPtr id;

	if (size >3)
	{
		//sanity check, the range [1/n,1] has nodes other then me.
		{
			VirtualID_SPtr vid_offset(new VirtualID(VirtualID::createFromRandom(1.0/size)));
			VirtualID_SPtr target_vid(new VirtualID(*myVID));
			target_vid->add(*vid_offset);

			MembershipRing::iterator pos = ringSet.lower_bound(target_vid);
			if (pos == ringSet.end())
			{
				pos = ringSet.begin();
			}

			if (ScTraceBuffer::isDebugEnabled(tc_))
			{
				ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this,
						"getRandomizedStructuredNode", "check");
				buffer->addProperty<int>("size", size);
				buffer->addProperty<double>("start-range",1.0/size );
				buffer->addProperty("offset-VID", vid_offset->toString());
				buffer->addProperty("my-VID", myVID->toString());
				buffer->addProperty("target-VID", target_vid->toString());
				buffer->addProperty("target", pos->second->getNodeName());
				buffer->invoke();
			}

			if (pos->second->getNodeName() == myID->getNodeName())
			{
				//its just me
				Trace_Exit(this, "getRandomizedStructuredNode",
							"empty range");
				return std::make_pair(id,size);
			}
		}

			bool found = false;
			while (!found)
			{
				double d = exp(::log(size) * (drand48() - 1.0));
				VirtualID_SPtr vid_offset(new VirtualID(VirtualID::createFromRandom(d)));
				VirtualID_SPtr target_vid(new VirtualID(*myVID));
				target_vid->add(*vid_offset);

				if (ScTraceBuffer::isDebugEnabled(tc_))
				{
					ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this, "getRandomizedStructuredNode", "target");
					buffer->addProperty<int>("size", size);
					buffer->addProperty<double>("offset", d);
					buffer->addProperty("offset-VID", vid_offset->toString());
					buffer->addProperty("my-VID", myVID->toString());
					buffer->addProperty("target-VID", target_vid->toString());
					buffer->invoke();
				}

				MembershipRing::iterator pos = ringSet.lower_bound(target_vid);
				if (pos == ringSet.end())
				{
					pos = ringSet.begin();
				}

				if (pos->second->getNodeName() != myID->getNodeName())
				{
					found = true;
					id = pos->second;

					if (ScTraceBuffer::isDebugEnabled(tc_))
					{
						ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this, "getRandomizedStructuredNode", "found");
						buffer->addProperty("node-VID", pos->first->toString());
						buffer->addProperty("node",  NodeIDImpl::stringValueOf(id));
						buffer->invoke();
					}
				}
			}
		}

	Trace_Exit(this, "getRandomizedStructuredNode",
			"node", NodeIDImpl::stringValueOf(id));
	return std::make_pair(id,size);
}

std::pair<NodeIDImpl_SPtr, bool> MembershipManagerImpl::getDiscoveryNode()
{
	Trace_Entry(this, "getDiscoveryNode");

	bool skip_closed = false;
	{
		boost::recursive_mutex::scoped_lock lock(membership_mutex);
		if (_closed)
		{
			skip_closed = true;
		}
	}

	if (skip_closed)
	{
		Trace_Exit(this, "getDiscoveryNode", "skip-closed");
		return std::make_pair(NodeIDImpl_SPtr(),true);
	}

	NodeIDImpl_SPtr id;
	int num_bs_cand = bootstrap->getNumNotInView();
	int num_hist_cand = nodeHistorySet.size();
	bool from_bss = true;

	if (num_hist_cand == 0 && num_bs_cand > 0)
	{
		//id = bootstrapSet.getNextNode_NotInView();
		id = bootstrap->getNextNode_NotInView();
	}
	else if (num_hist_cand > 0 && num_bs_cand == 0)
	{
		id = nodeHistorySet.getNextNode();
		from_bss = false;
	}
	else if (num_hist_cand > 0 && num_bs_cand > 0)
	{
		//mix
		double r_bs = (double)num_bs_cand / (num_bs_cand+num_hist_cand);
		int k = rand();
		if ( (double)k / RAND_MAX >= r_bs)
		{
			id = nodeHistorySet.getNextNode();
			from_bss = false;
		}
		else
		{
			//id = bootstrapSet.getNextNode_NotInView();
			id = bootstrap->getNextNode_NotInView();
			if (id && !id->isNameANY() && nodeHistorySet.contains(id))
			{
				from_bss = false;
			}
		}
	}


	if (ScTraceBuffer::isDebugEnabled(tc_))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this, "getDiscoveryNode()");
		buffer->addProperty<int>("num-BSS-cand",num_bs_cand);
		buffer->addProperty<int>("num-Hist-cand",num_hist_cand);
		buffer->addProperty<bool>("from-BSS",from_bss);
		buffer->addProperty("node", NodeIDImpl::stringValueOf(id));
		buffer->invoke();
	}

	Trace_Exit(this, "getDiscoveryNode()");
	return std::make_pair(id,from_bss);
}


/*
 * @see spdr::MembershipManager
 */
bool MembershipManagerImpl::isFromHistorySet(NodeIDImpl_SPtr discoveryNode)
{
	Trace_Entry(this,"isFromHistorySet()", "node", toString<NodeIDImpl>(discoveryNode));

	//any version
	bool res = nodeHistorySet.containsVerGreaterEqual(discoveryNode, NodeVersion());

	Trace_Exit<bool>(this,"isFromHistorySet()", res);
	return res;
}

void MembershipManagerImpl::reportSuspect(
		NodeIDImpl_SPtr suspectedNode)
{
	Trace_Entry(this,"reportSuspect()");

	Trace_Event(this, "reportSuspect()", "A neighbor is under failure suspicion",
			"suspect", NodeIDImpl::stringValueOf(suspectedNode));

	bool skip_closed = false;
	{
		boost::recursive_mutex::scoped_lock lock(membership_mutex);
		if (_closed)
		{
			skip_closed = true;
		}
	}

	if (skip_closed)
	{
		Trace_Exit(this, "reportSuspect", "skip-closed");
		return;
	}

	//never report suspect on myself
	if (*suspectedNode == *myID)
	{
		Trace_Error(this, "reportSuspect()", "Error: Cannot report suspect on my node-ID");
		throw SpiderCastRuntimeError("Cannot report suspect on my node-ID");
	}

	disconnectedNeighbor(suspectedNode);

	MembershipViewMap::iterator pos = viewMap.find(suspectedNode);
	if (pos != viewMap.end())
	{
		NodeVersion suspect_ver = pos->second.nodeVersion;
		Trace_Event(this, "reportSuspect()", "found in view, processing");
		bool view_changed = viewProcessSuspicion(
				StringSPtr( new String(myID->getNodeName())), //reporter
				StringSPtr( new String(suspectedNode->getNodeName())), //suspect
				suspect_ver); //suspect-version
		if (view_changed)
		{
			//Scheduled when a report-suspect causes a view change,
			//to avoid setting calling "refreshSuccessorList()"
			//to set the successor from within the "reportSuspect()" call.
			AbstractTask_SPtr task(new RefreshSuccessorListTask(_coreInterface));
			taskSchedule_SPtr->scheduleDelay(task, TaskSchedule::ZERO_DELAY);
			Trace_Event(this, "reportSuspect()", "view changed, scheduled a RefreshSuccessorListTask");
		}

		//send the suspicion to the monitors
		if (_config.isHighPriorityMonitoringEnabled())
		{
			outgoingMemMessage->writeH1Header(SCMessage::Type_Mem_Node_Update_UDP);
			ByteBuffer_SPtr bb = outgoingMemMessage->getBuffer();
			bb->writeString(_config.getBusName());
			bb->writeString(myID->getNodeName());
			bb->writeLong(myVersion.getIncarnationNumber()); //SplitBrain
			bb->writeInt((int32_t)0); //leave
			bb->writeInt((int32_t)0); //alive
			bb->writeInt((int32_t)1); //suspicion
			bb->writeString(myID->getNodeName()); //reporting node
			bb->writeString(suspectedNode->getNodeName()); //suspect node
			bb->writeNodeVersion(suspect_ver); //suspect version
			bb->writeInt((int32_t)0); //retained
			outgoingMemMessage->updateTotalLength();
			if (_config.isCRCMemTopoMsgEnabled())
			{
				outgoingMemMessage->writeCRCchecksum();
			}
			highPriorityMonitor_SPtr->send2Monitors(outgoingMemMessage);
			Trace_Event(this, "reportSuspect()", "sent to HPMs");
		}

		// TODO schedule an urgent update task if suspicions are flooded
	}
	else
	{
		Trace_Event(this, "reportSuspect()", "not found in view, ignoring");
	}

	Trace_Exit(this,"reportSuspect()");
}

//SplitBrain
bool MembershipManagerImpl::reportSuspicionDuplicateRemoteNode(NodeIDImpl_SPtr suspectedNode, int64_t incarnationNumber)
{
	Trace_Event(this,"reportSuspicionDuplicateRemoteNode()",
			"suspect", spdr::toString(suspectedNode),
			"inc", boost::lexical_cast<std::string>(incarnationNumber));

	bool result = false;

	bool skip_closed = false;
	{
		boost::recursive_mutex::scoped_lock lock(membership_mutex);
		if (_closed)
		{
			skip_closed = true;
		}
	}

	if (skip_closed)
	{
		Trace_Exit(this, "reportSuspicionDuplicateRemoteNode", "skip-closed");
		return result;
	}

	//never report suspect on myself
	if (*suspectedNode == *myID)
	{
		Trace_Error(this, "reportSuspicionDuplicateRemoteNode()", "Error: Cannot report SuspicionDuplicateRemoteNode on my node-ID");
		throw SpiderCastRuntimeError("Cannot report SuspicionDuplicateRemoteNode on my node-ID");
	}

	bool view_changed = false;
	MembershipViewMap::iterator pos = viewMap.find(suspectedNode);
	if (pos != viewMap.end())
	{
		NodeVersion ver = pos->second.nodeVersion;
		Trace_Event(this,"reportSuspicionDuplicateRemoteNode()", "found in view","version",ver.toString());
		if (incarnationNumber > ver.getIncarnationNumber())
		{
			NodeVersion injectedVer(ver.getIncarnationNumber(),0x7FFFFFFFFFFFFFFF);
			StringSPtr suspectName(new String(suspectedNode->getNodeName()));
			view_changed = processMsgLeave(suspectName, injectedVer, event::STATUS_SUSPECT_DUPLICATE_NODE);
			Trace_Warning(this,"reportSuspicionDuplicateRemoteNode()",
					"Warning: Duplicate node detected in view: Injected a Leave message with status STATUS_SUSPECT_DUPLICATE_NODE",
					"suspect",suspectedNode->getNodeName(), "injected-ver",injectedVer.toString());
			result = true;
		}
		else
		{
			Trace_Event(this,"reportSuspicionDuplicateRemoteNode()", "false suspicion, ignoring");
		}
	}
	else
	{
		std::pair<NodeInfo,bool> res = nodeHistorySet.getNodeInfo(suspectedNode);
		if (res.second)
		{
			if (res.first.nodeVersion.getIncarnationNumber() < incarnationNumber)
			{
				//Do not inject a LEAVE to prevent double Node_Leave events.
				Trace_Event(this,"reportSuspicionDuplicateRemoteNode()",
						"Alert: Duplicate node suspicion detected in history. This may be a new node trying to join, with simultaneous discovery. Ignoring.",
						"suspect",suspectedNode->getNodeName(), "inc", spdr::stringValueOf(incarnationNumber),
						"history-ver", res.first.nodeVersion.toString(),
						"history-status", spdr::stringValueOf(res.first.status));
			}
			else
			{
				Trace_Event(this,"reportSuspicionDuplicateRemoteNode()", "checked history, false suspicion, ignoring");
			}
		}
		else
		{
			Trace_Event(this,"reportSuspicionDuplicateRemoteNode()", "not in history, ignoring");
		}
	}

	if (!skip_closed && view_changed)
	{
		//Scheduled when a report-SuspicionDuplicateRemoteNode causes a view change,
		//to avoid setting calling "refreshSuccessorList()"
		//to set the successor from within the "reportSuspicionDuplicateRemoteNode()" call.
		AbstractTask_SPtr task(new RefreshSuccessorListTask(_coreInterface));
		taskSchedule_SPtr->scheduleDelay(task, TaskSchedule::ZERO_DELAY);
		Trace_Event(this, "reportSuspicionDuplicateRemoteNode()", "view changed, scheduled a RefreshSuccessorListTask");
	}

	Trace_Exit(this,"reportSuspicionDuplicateRemoteNode()", result);
	return result;
}

void MembershipManagerImpl::newNeighbor(
		NodeIDImpl_SPtr node)
{
	Trace_Entry(this,"newNeighbor()");

	if (ScTraceBuffer::isEventEnabled(tc_))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::event(this, "newNeighbor()");
		buffer->addProperty("node", NodeIDImpl::stringValueOf(node));
		buffer->invoke();
	}

	bool skip_closed = false;
	{
		boost::recursive_mutex::scoped_lock lock(membership_mutex);
		if (_closed)
		{
			skip_closed = true;
		}
	}

	if (skip_closed)
	{
		Trace_Exit(this, "newNeighbor", "skip-closed");
		return;
	}

	neighborsChangeQ.push_back(std::make_pair(node,true));
	AbstractTask_SPtr task = AbstractTask_SPtr(
			new NeighborChangeTask(_coreInterface));
	taskSchedule_SPtr->scheduleDelay(task, TaskSchedule::ZERO_DELAY);

	Trace_Exit(this,"newNeighbor()");
}

void MembershipManagerImpl::disconnectedNeighbor(NodeIDImpl_SPtr node)
{
	Trace_Entry(this,"disconnectedNeighbor()");

	bool skip_closed = false;
	{
		boost::recursive_mutex::scoped_lock lock(membership_mutex);
		if (_closed)
		{
			skip_closed = true;
		}
	}

	if (skip_closed)
	{
		Trace_Exit(this, "disconnectedNeighbor", "skip-closed");
		return;
	}

	neighborsChangeQ.push_back(std::make_pair(node,false));

	AbstractTask_SPtr task = AbstractTask_SPtr(
			new NeighborChangeTask(_coreInterface));
	taskSchedule_SPtr->scheduleDelay(task, TaskSchedule::ZERO_DELAY);

	if (ScTraceBuffer::isDebugEnabled(tc_))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this, "disconnectedNeighbor()", "scheduled a NeighborChangeTask");
		buffer->addProperty("node", NodeIDImpl::stringValueOf(node));
		buffer->invoke();
	}

	Trace_Exit(this,"disconnectedNeighbor()");
}


void MembershipManagerImpl::processIncomingMembershipMessage(
		SCMessage_SPtr membershipMsg)
{
	Trace_Entry(this,"processIncomingMembershipMessage()");

	bool view_changed = false;
	SCMessage::H1Header h1 = (*membershipMsg).readH1Header();
	Trace_Debug(this, "processIncomingMembershipMessage()","",
			"sender", NodeIDImpl::stringValueOf(membershipMsg->getSender()),
			"type", SCMessage::getMessageTypeName(h1.get<1>()));

	verifyCRC(membershipMsg, "processIncomingMembershipMessage()", "in");

	bool skip_closed = false;

	{
		boost::recursive_mutex::scoped_lock lock(membership_mutex);
		if (_closed && (h1.get<1>() != SCMessage::Type_Mem_Node_Leave_Ack))
		{
			skip_closed = true;
		}
	}

	if (skip_closed)
	{
		Trace_Exit(this, "processIncomingMembershipMessage", "skip-closed");
		return;
	}

	switch (h1.get<1>()) //message type
	{
	case SCMessage::Type_Mem_Node_Leave:
	{
		ByteBuffer_SPtr bb_sptr = membershipMsg->getBuffer();
		StringSPtr name = bb_sptr->readStringSPtr();
		NodeVersion ver = membershipMsg->readNodeVersion();
		spdr::event::NodeStatus node_status = static_cast<spdr::event::NodeStatus>(bb_sptr->readInt());
		if (ScTraceBuffer::isEventEnabled(tc_))
		{
			ScTraceBufferAPtr buffer = ScTraceBuffer::event(this, "processIncomingMembershipMessage()", "Leave");
			buffer->addProperty("name", *name);
			buffer->addProperty("version", ver.toString());
			buffer->addProperty<int>("NodeStatus", node_status);
			buffer->invoke();
		}
		view_changed = processMsgLeave(name, ver, node_status);

		//send the leave to the monitors
		if (_config.isHighPriorityMonitoringEnabled())
		{
			outgoingMemMessage->writeH1Header(SCMessage::Type_Mem_Node_Update_UDP);
			ByteBuffer_SPtr bb = outgoingMemMessage->getBuffer();
			bb->writeString(_config.getBusName());
			bb->writeString(myID->getNodeName());
			bb->writeLong(myVersion.getIncarnationNumber()); //SplitBrain incarnation

			bb->writeInt((int32_t)1); //leave
			bb->writeString(*name);
			bb->writeNodeVersion(ver);
			bb->writeInt(static_cast<int>(node_status));

			bb->writeInt((int32_t)0); //alive
			bb->writeInt((int32_t)0); //suspicion
			bb->writeInt((int32_t)0); //retained

			outgoingMemMessage->updateTotalLength();
			if (_config.isCRCMemTopoMsgEnabled())
			{
				outgoingMemMessage->writeCRCchecksum();
			}
			highPriorityMonitor_SPtr->send2Monitors(outgoingMemMessage);
			Trace_Debug(this, "processIncomingMembershipMessage()", "Leave send to HPMs");
		}

		//for RetainAtt send LeaveAck
		if (*name == membershipMsg->getSender()->getNodeName())
		{
			sendLeaveAckMsg(membershipMsg->getSender());
		}
	}
	break;

	case SCMessage::Type_Mem_Node_Leave_Ack:
	{
		//RetainAtt
		if (_config.isRetainAttributesOnSuspectNodesEnabled())
		{
			ByteBuffer_SPtr bb = membershipMsg->getBuffer();
			String target = bb->readString();
			if (target == myID->getNodeName())
			{
				boost::recursive_mutex::scoped_lock lock(membership_mutex);
				_close_done = true;
				_close_remove_retained_att_ack = true;
				_close_done_condition_var.notify_all();
			}
			else
			{
				String what = "Error: LeaveAck wrong target, it is not me!";
				Trace_Error(this, "processIncomingMembershipMessage()", what,
						"target", target);
				throw SpiderCastRuntimeError(what);
			}
		}
	}
	break;

	case SCMessage::Type_Mem_Node_Update:
	{
		view_changed = processMsgNodeUpdate(membershipMsg);
	}
	break;

	case SCMessage::Type_Mem_Node_Update_UDP:
	{
		ByteBuffer_SPtr bb = membershipMsg->getBuffer();
		//These fields are skipped cause they are used by UDPComm only
		bb->readString(); //Bus name
		bb->readString(); //Sender name
		bb->readLong(); //Incarnation, SplitBrain

		view_changed = processMsgNodeUpdate(membershipMsg);
	}
	break;

	case SCMessage::Type_Mem_Metadata_Update:
	{
		//SplitBrain take into account sender's incarnation
		bool sendRequest = attributeManager.processIncomingUpdateMessage(
				membershipMsg, outgoingAttMessage);
		if (sendRequest)
		{
			Trace_Debug(this, "processIncomingMembershipMessage()", "sending request");

			//if the send request fails, undo the markPending from that neighbor and resent a request to all
			bool success = neighborTable_SPtr->sendToNeighbor(
					membershipMsg->getSender(), outgoingAttMessage);

			if (!success)
			{

				Trace_Debug(this, "processIncomingMembershipMessage()", "request failed, undoing the update");
				bool sendRequest2 = attributeManager.undoPendingRequests(membershipMsg->getSender(), outgoingAttMessage);
				if (sendRequest2)
				{
					Trace_Debug(this, "processIncomingMembershipMessage()", "sending a request to all neighbors");
					//std::pair<int,int> result = neighborTable_SPtr->sendToAllNeighbors(outgoingAttMessage);
					std::pair<int,int> result = neighborTable_SPtr->sendToAllRoutableNeighbors(outgoingAttMessage);

					if (!(result.first == result.second && result.second > 0))
					{
						Trace_Debug(this, "processIncomingMembershipMessage()", "send to all failed");
					}
				}
				else
				{
					Trace_Debug(this, "processIncomingMembershipMessage()", "request failed, nothing to undo");
				}
			}
		}
	}
	break;

	case SCMessage::Type_Mem_Metadata_Request:
	case SCMessage::Type_Mem_Metadata_RequestPush:
	{
		//SplitBrain take into account sender's incarnation
		bool sendReply = attributeManager.processIncomingRequestMessage(
				membershipMsg, outgoingAttMessage,
				(h1.get<1>() == SCMessage::Type_Mem_Metadata_RequestPush));
		if (sendReply)
		{
			bool success = neighborTable_SPtr->sendToNeighbor(
					membershipMsg->getSender(), outgoingAttMessage);

			Trace_Debug(this, "processIncomingMembershipMessage()", "sent reply",
					"success", ScTraceBuffer::stringValueOf(success));
		}
	}
	break;

	case SCMessage::Type_Mem_Metadata_Reply:
	{
		std::pair<bool,bool> res = attributeManager.processIncomingReplyMessage(
				membershipMsg, outgoingAttMessage);

		if (res.first) //notify
		{
			Trace_Debug(this, "processIncomingMembershipMessage()", "notifying App");
			scheduleChangeOfMetadataDeliveryTask();
		}

		if (res.second) //push new requests
		{
			Trace_Debug(this, "processIncomingMembershipMessage()", "request invalidations, pushing a new request to all neighbors");

			//std::pair<int,int> result = neighborTable_SPtr->sendToAllNeighbors(outgoingAttMessage);
			std::pair<int,int> result = neighborTable_SPtr->sendToAllRoutableNeighbors(outgoingAttMessage);

			if (result.first == result.second && result.second > 0)
			{
				Trace_Debug(this, "processIncomingMembershipMessage()", "Failed to push a request to all neighbors");
			}
		}
	}
	break;

	default:
	{
		String what("Error: Unexpected message type ");
		what.append(SCMessage::getMessageTypeName(h1.get<1>()));
		Trace_Error(this,"processIncomingMembershipMessage()", what);
		throw SpiderCastRuntimeError(what);
	}
	}

	{
		boost::recursive_mutex::scoped_lock lock(membership_mutex);
		if (_closed)
			skip_closed = true;
	}

	if (!skip_closed && view_changed )
	{
		refreshSuccessorList();
	}

	Trace_Exit(this,"processIncomingMembershipMessage()");
}

NodeVersion MembershipManagerImpl::getMyNodeVersion() const
{
	return NodeVersion(myVersion);
}

/*
 * @see spdr::MembershipManager
 */
void MembershipManagerImpl::getDiscoveryView(
		SCMessage_SPtr discoveryMsg)
{
	Trace_Entry(this,"getDiscoveryView()");

	bool skip_closed = false;
	{
		boost::recursive_mutex::scoped_lock lock(membership_mutex);
		if (_closed)
		{
			skip_closed = true;
		}
	}

	boost::shared_ptr<ByteBuffer> bb = (*discoveryMsg).getBuffer();

	if (skip_closed)
	{
		//make sure it is a legal message
		(*bb).writeInt(0);
		(*discoveryMsg).updateTotalLength();
		if (_config.isCRCMemTopoMsgEnabled())
		{
			(*discoveryMsg).writeCRCchecksum();
		}

		Trace_Exit(this, "getDiscoveryView", "skip-closed");
		return;
	}

	int32_t view_size = viewMap.size();

	(*bb).writeInt(view_size);

	std::ostringstream view_trace;
	view_trace << "size=" << view_size;

	for  (MembershipViewMap::iterator it = viewMap.begin(); it != viewMap.end(); ++it)
	{
		(*discoveryMsg).writeNodeID((*it).first);
		(*discoveryMsg).writeNodeVersion((*it).second.nodeVersion);

		if (ScTraceBuffer::isDebugEnabled(tc_))
		{
			view_trace << ", " << (*it).first->getNodeName();
		}
	}
	(*discoveryMsg).updateTotalLength();

	if (_config.isCRCMemTopoMsgEnabled())
	{
		(*discoveryMsg).writeCRCchecksum();
	}


	Trace_Debug(this,"getDiscoveryView()", view_trace.str());

	Trace_Exit(this,"getDiscoveryView()");
}

/*
 * @see spdr::MembershipManager
 */
void MembershipManagerImpl::getDiscoveryViewPartial(SCMessage_SPtr discoveryMsg, int num_id)
{
	if (ScTraceBuffer::isEntryEnabled(tc_))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::entry(this, "getDiscoveryViewPartial()");
		buffer->addProperty<int>("num", num_id);
		buffer->invoke();
	}

	bool skip_closed = false;
	{
		boost::recursive_mutex::scoped_lock lock(membership_mutex);
		if (_closed)
		{
			skip_closed = true;
		}
	}

	boost::shared_ptr<ByteBuffer> bb = discoveryMsg->getBuffer();

	if (skip_closed)
	{
		//make sure it is a legal message
		bb->writeInt(0);
		discoveryMsg->updateTotalLength();
		if (_config.isCRCMemTopoMsgEnabled())
		{
			discoveryMsg->writeCRCchecksum();
		}

		Trace_Exit(this, "getDiscoveryView", "skip-closed");
		return;
	}

	if (num_id < 0)
	{
		num_id = 0;
	}

	int32_t view_size = viewMap.size();

	if (view_size > num_id+1)
	{
		view_size = num_id+1;
	}

	bb->writeInt(view_size);

	std::ostringstream view_trace;
	view_trace << "size=" << view_size;

	MembershipViewMap::const_iterator it = viewMap.find(myID);

	while (view_size > 0)
	{
		discoveryMsg->writeNodeID(it->first);
		discoveryMsg->writeNodeVersion(it->second.nodeVersion);

		if (ScTraceBuffer::isDebugEnabled(tc_))
		{
			view_trace << ", " << it->first->getNodeName();
		}

		--view_size;
		++it;
		if (it == viewMap.end())
		{
			it = viewMap.begin();
		}
	}

	discoveryMsg->updateTotalLength();
	if (_config.isCRCMemTopoMsgEnabled())
	{
		discoveryMsg->writeCRCchecksum();
	}

	Trace_Debug(this,"getDiscoveryViewPartial()", view_trace.str());

	Trace_Exit(this,"getDiscoveryViewPartial()");
}

/*
 * @see spdr::MembershipManager
 */
int MembershipManagerImpl::getDiscoveryViewMultipart(
		std::vector<SCMessage_SPtr>& discoveryMsgMultipart)
{
	Trace_Entry(this,"getDiscoveryViewMultipart()");

	bool skip_closed = false;
	{
		boost::recursive_mutex::scoped_lock lock(membership_mutex);
		if (_closed)
		{
			skip_closed = true;
		}
	}

	uint32_t msgIndex = 0;
	SCMessage_SPtr msg = discoveryMsgMultipart[msgIndex];
	ByteBuffer_SPtr bb = msg->getBuffer();

	if (skip_closed)
	{
		//make sure it is a legal message
		bb->writeInt(0);
		msg->updateTotalLength();
		if (_config.isCRCMemTopoMsgEnabled())
		{
			msg->writeCRCchecksum();
		}

		Trace_Exit(this, "getDiscoveryViewMultipart()", "skip-closed");
		return 1;
	}

//	//increase my version every time this node gets discovered
//	//this covers the case when it is discovered by a node from a healed partition,
//	//which has this node in its history with the current version
//	myVersion.addToMinorVersion(1);
//	updateDB.addToAlive(myID,myVersion);
//	viewMap[myID].nodeVersion = myVersion;
//	Trace_Debug(this, "getDiscoveryViewMultipart()",
//			"I was discovered, increased my version & propagate",
//			"version", myVersion.toString());

	const int32_t view_size = viewMap.size();
	const size_t payload_start_position = bb->getPosition();
	const uint32_t data_max_size = _config.getUDPPacketSizeBytes() - (_config.isCRCMemTopoMsgEnabled()?4:0);

	if (ScTraceBuffer::isDebugEnabled(tc_))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this, "getDiscoveryViewMultipart()",
				"preparing to write");
		buffer->addProperty<int>("#view", view_size);
		buffer->addProperty<int>("payload-start", payload_start_position);
		buffer->addProperty<int>("data-max-size", data_max_size);
		buffer->invoke();
	}

	std::ostringstream view_trace;
	view_trace << "size=" << view_size;

	int32_t num_items =0;
	bb->writeInt(num_items); //skip

	MembershipViewMap::iterator it = viewMap.begin();
	while ( it != viewMap.end())
	{
		size_t position_before_last_item = bb->getPosition();
		(*msg).writeNodeID((*it).first);
		(*msg).writeNodeVersion((*it).second.nodeVersion);

//		if (ScTraceBuffer::isDebugEnabled(tc_))
//		{
//			ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this, "getDiscoveryViewMultipart()",
//					"written item");
//			buffer->addProperty<int>("#item", num_items);
//			buffer->addProperty<int>("#packet", msgIndex);
//			buffer->addProperty("ID", it->first->getNodeName());
//			buffer->invoke();
//		}

		if (bb->getPosition() > data_max_size) //packet overflow
		{
			if (num_items == 0)
			{
				Trace_Error(this, "getDiscoveryViewMultipart()",
						"Error: Single discovery item bigger then UDP packet");
				throw SpiderCastRuntimeError("Single discovery item bigger then UDP packet");
			}

			if (ScTraceBuffer::isEventEnabled(tc_))
			{
				ScTraceBufferAPtr buffer = ScTraceBuffer::event(this, "getDiscoveryViewMultipart()",
						"closing packet");
				buffer->addProperty<int>("#packet", msgIndex);
				buffer->addProperty("#items", num_items);
				buffer->invoke();
			}

			if (ScTraceBuffer::isDebugEnabled(tc_))
			{
				view_trace << " <-p" << msgIndex << "-|, ";
			}

			//cancel last item, close packet
			bb->setPosition(payload_start_position);
			bb->writeInt(num_items);
			bb->setPosition(position_before_last_item);

			msg->updateTotalLength();
			if (_config.isCRCMemTopoMsgEnabled())
			{
				msg->writeCRCchecksum();
			}

			//open a new packet
			++msgIndex;
			num_items = 0;

			{
				ByteBuffer_SPtr bb2;
				if (discoveryMsgMultipart.size() > msgIndex)
				{
					msg = discoveryMsgMultipart[msgIndex];
					bb2 = msg->getBuffer();
				}
				else
				{
					msg.reset(new SCMessage);
					bb2 = ByteBuffer::createByteBuffer(_config.getUDPPacketSizeBytes());
					msg->setBuffer(bb2);
					discoveryMsgMultipart.push_back(msg);
				}

				bb2->setPosition(0);
				bb2->writeByteArray(bb->getBuffer(),payload_start_position); //copy header
				bb2->writeInt(num_items); //skip
				bb = bb2;
			}
		}
		else
		{
			if (ScTraceBuffer::isDebugEnabled(tc_))
			{
				view_trace << ", " << it->first->getNodeName();
			}

			++it;
			++num_items;
		}
	}

	size_t last_position = bb->getPosition();
	bb->setPosition(payload_start_position);
	bb->writeInt(num_items);
	bb->setPosition(last_position);

	msg->updateTotalLength();
	if (_config.isCRCMemTopoMsgEnabled())
	{
		msg->writeCRCchecksum();
	}


	Trace_Debug(this,"getDiscoveryViewMultipart()", view_trace.str());

	Trace_Exit<int>(this,"getDiscoveryViewMultipart()", msgIndex+1);
	return (msgIndex+1);
}

/*
 * @see spdr::MembershipManager
 */
bool MembershipManagerImpl::processIncomingMulticastDiscoveryNodeView(NodeIDImpl_SPtr peerID, const NodeVersion& ver, bool isRequest, bool isBootstrap)
{
	Trace_Debug(this, "processIncomingMulticastDiscoveryNodeView()", "",
			"peer", spdr::toString(peerID), "ver", ver.toString(),
			"isReq", (isRequest?"T":"F"), "isBoot", (isBootstrap?"T":"F"));

	bool skip_closed = false;
	{
		boost::recursive_mutex::scoped_lock lock(membership_mutex);
		if (_closed)
		{
			skip_closed = true;
		}
	}

	if (skip_closed)
	{
		Trace_Exit(this, "processIncomingDiscoveryView", "skip-closed");
		return false;
	}


	bool reply = false;
	bool view_changed = viewMergeAliveNode(peerID, ver);
	bool inView = (viewMap.count(peerID) >0);

	if (isRequest && !isBootstrap && !view_changed && !inView)
	{
		myVersion.addToMinorVersion(1);
		updateDB.addToAlive(myID,myVersion);
		viewMap[myID].nodeVersion = myVersion;
		attributeManager.writeMyRebuttalKey();

		Trace_Debug(this, "processIncomingMulticastDiscoveryNodeView()",
				"I was discovered from multicast request, but the sender was not inserted into the view. Increased my version & propagate",
				"version", myVersion.toString());
	}

	if (!isRequest)
	{
		myVersion.addToMinorVersion(1);
		updateDB.addToAlive(myID,myVersion);
		viewMap[myID].nodeVersion = myVersion;
		attributeManager.writeMyRebuttalKey();

		Trace_Debug(this, "processIncomingMulticastDiscoveryNodeView()",
				"I was discovered from multicast reply. Increased my version & propagate",
				"version", myVersion.toString());
	}

	//suppress replies for nodes that are in view pre & post update
	if (isRequest)
	{
		if (view_changed)
		{
			reply = true;
		}
		else if (!inView)
		{
			reply = true;
		}
	}

	if (view_changed)
	{
		Trace_Debug(this, "processIncomingMulticastDiscoveryNodeView()", "view changed");
		refreshSuccessorList();
	}

	Trace_Exit(this, "processIncomingMulticastDiscoveryNodeView()", reply);
	return reply;
}

/*
 * @see spdr::MembershipManager
 */
void MembershipManagerImpl::processIncomingDiscoveryView(
		SCMessage_SPtr discoveryMsg, bool isRequest, bool isBootstrap)
{
	Trace_Entry(this, "processIncomingDiscoveryView()");

	bool skip_closed = false;
	{
		boost::recursive_mutex::scoped_lock lock(membership_mutex);
		if (_closed)
		{
			skip_closed = true;
		}
	}

	if (skip_closed)
	{
		Trace_Exit(this, "processIncomingDiscoveryView", "skip-closed");
		return;
	}

	//increase my version every time this node gets discovered, but was not drawn from bootstrap.
	//this covers the case when it is discovered by a node from a healed partition,
	//which has this node in its history with the current version
	if (isRequest && !isBootstrap)
	{
		myVersion.addToMinorVersion(1);
		updateDB.addToAlive(myID,myVersion);
		viewMap[myID].nodeVersion = myVersion;
		attributeManager.writeMyRebuttalKey();
		Trace_Debug(this, "processMsgUpdate()",	"I was discovered from history, increased my version & propagate",
			"version", myVersion.toString());
	}

	boost::shared_ptr<ByteBuffer> bb = discoveryMsg->getBuffer();
	int32_t view_size = 0;
	bool view_changed = false;
	try
	{
		 view_size = bb->readInt();
	}
	catch (IndexOutOfBoundsException& e)
	{
		String what(e.what());
		what.append(", Exception while unmarshaling discovery message from ");
		what.append(discoveryMsg->getSender()->getNodeName());
		throw MessageUnmarshlingException(what);
	}

	std::ostringstream view_trace;
		view_trace << "size=" << view_size << ", node-names: ";

	for (int32_t i = 0; i < view_size; ++i)
	{
		NodeIDImpl_SPtr id = (*discoveryMsg).readNodeID();
		NodeVersion ver = (*discoveryMsg).readNodeVersion();
		bool res = viewMergeAliveNode(id, ver);
		view_changed = view_changed || res;

		if (ScTraceBuffer::isEventEnabled(tc_))
		{
			view_trace << id->getNodeName() << " ";
		}
	} //each node in view

	Trace_Debug(this,"processIncomingDiscoveryView()", view_trace.str(),
			"sender", discoveryMsg->getSender()->getNodeName());

	if (view_changed)
	{
		Trace_Debug(this, "processIncomingDiscoveryView()", "view changed");
		refreshSuccessorList();
	}

	Trace_Exit(this, "processIncomingDiscoveryView()");
}


/*
 * @see spdr::MembershipManager
 */
void MembershipManagerImpl::destroyMembershipService()
{
	boost::recursive_mutex::scoped_lock lock(membership_mutex);

	membershipServiceImpl_SPtr.reset(); //reference null pointer
	//TODO reset the notify flags on the attribute manager
}


void MembershipManagerImpl::destroyLeaderElectionService()
{
	boost::recursive_mutex::scoped_lock lock(membership_mutex);
	leaderElectionCandidate_SPtr.reset();
}

/*
 * schedule a task to notify the App using the membership service
 * schedule only if a task is not scheduled already
 *
 * App-thread (from MembershipService), MemTopoThread (processing a Reply message)
 */
void MembershipManagerImpl::scheduleChangeOfMetadataDeliveryTask()
{
	Trace_Entry(this, "scheduleChangeOfMetadataDeliveryTask()");

	if (attributeManager.testAndSetNotifyTaskScheduled())
	{
		AbstractTask_SPtr notifyMetadata = AbstractTask_SPtr(
				new ChangeOfMetadataDeliveryTask(_coreInterface));
		taskSchedule_SPtr->scheduleDelay(notifyMetadata, TaskSchedule::ZERO_DELAY);
	}

	Trace_Exit(this, "scheduleChangeOfMetadataDeliveryTask()");
}

/*
 * @see spdr::MembershipManager
 */
int MembershipManagerImpl::getViewSize()
{
	return (int)viewMap.size();
}

bool  MembershipManagerImpl::clearRemoteNodeRetainedAttributes(NodeID_SPtr target, int64_t incarnation)
{
	Trace_Entry(this, "clearRemoteNodeRetainedAttributes()");

	{
		boost::recursive_mutex::scoped_lock lock(membership_mutex);
		if (_closed)
		{
			return false;
		}
		else
		{
			clearRetainAttrQ_.push_back(std::make_pair( boost::static_pointer_cast<NodeIDImpl,NodeID>(target), incarnation ));
			AbstractTask_SPtr task = AbstractTask_SPtr(
							new ClearRetainAttrTask(_coreInterface));
			taskSchedule_SPtr->scheduleDelay(task, TaskSchedule::ZERO_DELAY ) ;
		}
	}

	Trace_Exit(this, "clearRemoteNodeRetainedAttributes()");

	//TODO this returns true even when target is not in the history or is even in the view.

	return true;
}
//=======

MembershipService_SPtr MembershipManagerImpl::createMembershipService(
				const PropertyMap& properties,
				MembershipListener& membershipListener)
				throw (SpiderCastLogicError)
{
	Trace_Entry(this, "createMembershipService()");

	boost::recursive_mutex::scoped_lock lock(membership_mutex);

	if (_closed)
	{
		String what = "MembershipManager is closed";
		Trace_Exit(this, "createMembershipService()", "SpiderCastLogicError", what);
		throw SpiderCastLogicError(what);
	}

	if (membershipServiceImpl_SPtr)
	{
		String what = "Membership service already exists";
		Trace_Exit(this, "createMembershipService()", "SpiderCastLogicError", what);
		throw SpiderCastLogicError(what);
	}
	else
	{
		membershipServiceImpl_SPtr = boost::shared_ptr<MembershipServiceImpl>(
				new MembershipServiceImpl(
						_instID,
						_config.getMyNodeID(),
						_config.getBusName_SPtr(),
						_coreInterface.getMembershipManager(),
						_coreInterface.getHierarchyManager(),
						_config,
						properties,
						membershipListener) );

		AbstractTask_SPtr task = AbstractTask_SPtr(
				new FirstViewDeliveryTask(_coreInterface));
		attributeManager.testAndSetNotifyTaskScheduled(); //FirstViewDeliveryTask also notifies attributes
		taskSchedule_SPtr->scheduleDelay(task, TaskSchedule::ZERO_DELAY);

		Trace_Exit(this, "createMembershipService()");
		return boost::static_pointer_cast<MembershipService>(membershipServiceImpl_SPtr);
	}
}

leader_election::LeaderElectionService_SPtr MembershipManagerImpl::createLeaderElectionService(
		leader_election::LeaderElectionListener& electionListener,
		bool candidate,
		const PropertyMap& properties)
throw (SpiderCastRuntimeError,SpiderCastLogicError)
{
	Trace_Entry(this, "createLeaderElectionService()");

	{
		boost::recursive_mutex::scoped_lock lock(membership_mutex);

		if (_closed)
		{
			String what = "MembershipManager is closed";
			Trace_Exit(this, "createMembershipService()", "SpiderCastLogicError", what);
			throw SpiderCastLogicError(what);
		}

		if (leaderElectionCandidate_SPtr)
		{
			String what = "Leader election service already exists";
			Trace_Exit(this, "createLeaderElectionService()", "SpiderCastLogicError", what);
			throw SpiderCastLogicError(what);
		}
		else
		{
			leaderElectionCandidate_SPtr.reset(new leader_election::LECandidate(
					_instID,
					_config,
					_coreInterface.getMembershipManager(),
					_coreInterface.getTopoMemTaskSchedule(),
					electionListener,
					candidate,
					properties));

			leaderElectionViewKeeper_SPtr->setService(
					boost::static_pointer_cast<leader_election::LEViewListener>(
							leaderElectionCandidate_SPtr));

			AbstractTask_SPtr task = AbstractTask_SPtr(new leader_election::LEWarmupTask(
					_instID,leaderElectionViewKeeper_SPtr,leaderElectionCandidate_SPtr));
			boost::posix_time::time_duration warmupTO = boost::posix_time::millisec(
					leaderElectionCandidate_SPtr->getWarmupTimeoutMillis());
			taskSchedule_SPtr->scheduleDelay(task, warmupTO);
		}
	}

	return leaderElectionCandidate_SPtr;
}

/*
 * Note: synch if adding listeners by threads other than MemTopo after start.
 */
void MembershipManagerImpl::registerInternalMembershipConsumer(
				boost::shared_ptr<SCMembershipListener> listener,
				InternalMembershipConsumer who)
{
	Trace_Entry(this, "registerInternalMembershipConsumer()", intMemConsumerName[who]);

	{
		boost::recursive_mutex::scoped_lock lock(membership_mutex);

		switch (who)
		{
		case MembershipManager::IntMemConsumer_Hierarchy:
		case MembershipManager::IntMemConsumer_PubSub:
		case MembershipManager::IntMemConsumer_HighPriorityMonitor:
		case MembershipManager::IntMemConsumer_LeaderElection:
		{
			intMemConsumer_Vec[who] = listener;
			intMemConsumer_FirstViewDelivered = false;
			break;
		}

		default:
		{
			String what("Trying to register an unknown InternalMembershipConsumer ");
			what.append(intMemConsumerName[who]);
			throw SpiderCastRuntimeError(what);
		}

		}
	}

	Trace_Exit(this, "registerInternalMembershipConsumer()");
}

void MembershipManagerImpl::getDelegateFullView(SCMessage_SPtr viewupdateMsg, bool includeAttributes)
{
	Trace_Entry(this, "getDelegateFullView()", (includeAttributes?"true":"false") );

	if (!includeAttributes)
	{
		ByteBuffer_SPtr buffer = viewupdateMsg->getBuffer();
		buffer->writeInt((int32_t)viewMap.size());
		for (MembershipViewMap::const_iterator it = viewMap.begin(); it != viewMap.end(); ++it)
		{
			buffer->writeNodeID(it->first);
			buffer->writeLong(it->second.nodeVersion.getIncarnationNumber());
			buffer->writeInt(0); //no attribute table, zero size
		}
	}
	else
	{
		attributeManager.prepareFullUpdateMsg4Supervisor(viewupdateMsg);
	}

	Trace_Exit(this, "getDelegateFullView()");
}


//=== Tasks ===================================================================

void MembershipManagerImpl::periodicTask()
{
	//Trace_Entry(this, "periodicTask()");

	bool last_task = false;

	{
		boost::recursive_mutex::scoped_lock lock(membership_mutex);
		if (_closed)
		{
			Trace_Debug(this, "periodicTask()", "Closed, last task");
			last_task = true;
		}
	}

	if (!last_task)
	{
		if (_first_periodic_task)
		{
			Trace_Debug(this, "periodicTask()", "first task");
			//insert myself to the view
			viewAddNode(myID,myVersion);
			_first_periodic_task = false;

			//schedule a first-view delivery for internal consumers
			for (size_t i=0; i<intMemConsumer_Vec.size(); ++i)
			{
				if (intMemConsumer_Vec[i])
				{
					taskSchedule_SPtr->scheduleDelay( AbstractTask_SPtr(
							new FirstViewDeliveryTask(_coreInterface)), TaskSchedule::ZERO_DELAY);
					break;
				}
			}
		}

		//send a full viewMap to new neighbors
		//send a full meta-data digest to new neighbors
		//the neighbor-change-Q will normally be empty
		neighborChangeTask();
		//send an update to old neighbors
		if (!updateDB.empty())
		{
			Trace_Debug(this, "periodicTask()", "Sending membership update to all");
			prepareUpdateViewMsg(outgoingMemMessage);

			//std::pair<int,int> result = neighborTable_SPtr->sendToAllNeighbors(outgoingMemMessage);
			std::pair<int,int> result = neighborTable_SPtr->sendToAllRoutableNeighbors(outgoingMemMessage);

			if (result.first == result.second && result.second > 0)
			{
				updateDB.clear();
			}
			else
			{
				Trace_Debug(this, "periodicTask()",
						"Failed to send membership update to all, skipping update-DB clear");
			}
		}

		if (attributeManager.isUpdateNeeded()) 	//sometime there is no need to send a Diff
		{
			Trace_Debug(this, "periodicTask()", "Sending attribute differential update to all");

			//Between the prepare and the mark, my-attribute table may change,
			//and this should no be overwritten by the mark-version-sent
			uint64_t my_table_version_sent = attributeManager.prepareDiffUpdateMsg(outgoingAttMessage);

			//std::pair<int,int> result = neighborTable_SPtr->sendToAllNeighbors(outgoingAttMessage);
			std::pair<int,int> result = neighborTable_SPtr->sendToAllRoutableNeighbors(outgoingAttMessage);

			if (result.first == result.second && result.second > 0)
			{
				attributeManager.markVersionSent(my_table_version_sent);
			}
			else
			{
				//this ensures that we don't lose synch with neighbors
				Trace_Debug(this, "periodicTask()",
						"Failed to send attribute differential update to all, skipping markVersionSent");
			}
		}

		//schedule next task
		taskSchedule_SPtr->scheduleDelay(
				periodicTask_SPtr,
				boost::posix_time::milliseconds(_config.getMembershipGossipIntervalMillis()));
	}
	else
	{
		Trace_Debug(this, "periodicTask()","skip-closed");
	}

	//Trace_Exit(this, "periodicTask()");
}

void MembershipManagerImpl::firstViewDeliveryTask()
{
	Trace_Entry(this, "firstViewDeliveryTask()");

	notifyViewChange();

	Trace_Exit(this, "firstViewDeliveryTask()");
}


void MembershipManagerImpl::neighborChangeTask()
{
	//Trace_Entry(this, "neighborChangeTask()");

	{
		boost::recursive_mutex::scoped_lock lock(membership_mutex);
		if (_closed)
		{
			Trace_Exit(this, "neighborChangeTask()","Closed");
			return;
		}
	}

	if (neighborsChangeQ.empty())
	{
		//Trace_Exit(this, "neighborChangeTask()","no neighbor changes");
		return;
	}
	else
	{
		Trace_Debug(this, "neighborChangeTask()","changes pending",
				"Q-size", boost::lexical_cast<std::string>(neighborsChangeQ.size()));
	}

	for (NeighborsChangeQ::iterator it = neighborsChangeQ.begin();
			it != neighborsChangeQ.end(); ++it)
	{
		if (it->second)
		{
			newNeighborTask(it->first);
		}
		else
		{
			disconnectedNeighborTask(it->first);
		}
	}

	neighborsChangeQ.clear();

	//Trace_Exit(this, "neighborChangeTask()");
}

/*
 * Send a full view message to a new neighbor.
 */
void MembershipManagerImpl::newNeighborTask(NodeIDImpl_SPtr id)
{
	Trace_Entry(this, "newNeighborTask()", "id", NodeIDImpl::stringValueOf(id));

	if (neighborTable_SPtr)
	{
		//full membership update
		prepareFullViewMsg(outgoingMemMessage);

		//full digest update
		bool sendAttMsg = attributeManager.prepareFullUpdateMsg(outgoingAttMessage);

		if (neighborTable_SPtr->contains(id))
		{
			bool rc1 = neighborTable_SPtr->sendToNeighbor(id, outgoingMemMessage);
			Trace_Debug(this, "newNeighborTask()", "sent full view to neighbor",
					"node", NodeIDImpl::stringValueOf(id),
					"success", (rc1?"T":"F"));

			bool rc2 = false;
			if (rc1 && sendAttMsg)
			{
				rc2 = neighborTable_SPtr->sendToNeighbor(id, outgoingAttMessage);
				Trace_Debug(this, "newNeighborTask()", "sent attribute update to neighbor",
						"node", NodeIDImpl::stringValueOf(id),
						"success", (rc2?"T":"F"));
			}
			else
			{
				Trace_Debug(this, "newNeighborTask()", "skipped attribute update to neighbor",
						"node", NodeIDImpl::stringValueOf(id));
			}

			Trace_Event(this, "newNeighborTask()", "sent full view and attribute update to neighbor",
					"node", NodeIDImpl::stringValueOf(id),
					"return-code-view", (rc1?"T":"F"),
					"sendAttrMsg", ScTraceBuffer::stringValueOf<bool>(sendAttMsg),
					"return-code-attr", (rc2?"T":"F"));

			//TODO what to do if send failed?
		}
		else
		{
			Trace_Debug(this, "newNeighborTask()", "not found in neighbor table",
									"node", NodeIDImpl::stringValueOf(id));
		}
	}
	else
	{
		String what("Error: MembershipManagerImpl neighbor-table pointer is null");
		NullPointerException ex(what);

		Trace_Error(this, "newNeighborTask()",what);

		throw ex;
	}

	Trace_Exit(this, "newNeighborTask()");
}

/*	Evaluate whether any table has a pending request from that
 * disconnected neighbor, and if so send a request to all.
 *
 */
void MembershipManagerImpl::disconnectedNeighborTask(NodeIDImpl_SPtr id)
{
	Trace_Entry(this, "disconnectedNeighborTask()", "id", NodeIDImpl::stringValueOf(id));

	bool send = attributeManager.undoPendingRequests(id, outgoingAttMessage);
	if (send)
	{
		Trace_Debug(this, "disconnectedNeighborTask()", "sending a request to all neighbors");

		//std::pair<int,int> result = neighborTable_SPtr->sendToAllNeighbors(outgoingAttMessage);
		std::pair<int,int> result = neighborTable_SPtr->sendToAllRoutableNeighbors(outgoingAttMessage);

		if (result.first == result.second && result.second > 0)
		{
			Trace_Debug(this, "disconnectedNeighborTask()", "Failed to send a request to all neighbors");
		}
	}

	Trace_Exit(this, "disconnectedNeighborTask()");
}

void MembershipManagerImpl::changeOfMetadataDeliveryTask()
{
	Trace_Entry(this,"changeOfMetadataDeliveryTask()");

	notifyChangeOfMetadata();

	Trace_Exit(this,"changeOfMetadataDeliveryTask()");
}

void MembershipManagerImpl::refreshSuccessorListTask()
{
	//Scheduled when a report-suspect causes a view change,
	//to avoid setting the successor from within the reportSuspect call.

	Trace_Entry(this,"refreshSuccessorListTask()");

	refreshSuccessorList();

	Trace_Exit(this,"refreshSuccessorListTask()");
}


//=== private methods =========================================================

//=== notifyApp* ===
//
void MembershipManagerImpl::notifyLeave(NodeIDImpl_SPtr id, const NodeVersion& ver, spdr::event::NodeStatus status, AttributeTable_SPtr attributeTable)
{
	using namespace spdr::event;

	Trace_Entry(this,"notifyLeave()", "id", NodeIDImpl::stringValueOf(id), "status", ScTraceBuffer::stringValueOf(status));

	{
		boost::recursive_mutex::scoped_lock lock(membership_mutex);

		if (!_closed)
		{
			//internal consumers

			if (intMemConsumer_FirstViewDelivered) //first view delivered?
			{
				SCMembershipEvent leave(SCMembershipEvent::Node_Leave, id);
				for (int i=0; i < MembershipManager::IntMemConsumer_Max; ++i)
				{
					if (intMemConsumer_Vec[i])
					{
						intMemConsumer_Vec[i]->onMembershipEvent(leave);
					}
				}
			}
			else
			{
				Trace_Debug(this,"notifyLeave()",
						"Internal-consumer skipped, wait for first view delivery", "id", id->getNodeName());
			}


			//Membership-service
			if (membershipServiceImpl_SPtr && !membershipServiceImpl_SPtr->isClosed())
			{
				if (membershipServiceImpl_SPtr->isFirstViewDelivered())
				{
					NodeID_SPtr id_SPtr = boost::static_pointer_cast<NodeID>(id);
					AttributeMap_SPtr attrMap;
					if (attributeTable)
					{
						attrMap = attributeTable->getAttributeMap4Notification();
						attributeTable->markLastNotify();
					}
					MetaData_SPtr metadata_SPtr(new MetaData(
							attrMap,
							ver.getIncarnationNumber(),
							status));
					MembershipEvent_SPtr nl_SPtr(new NodeLeaveEvent(id_SPtr, metadata_SPtr));

					Trace_Debug(this,"notifyLeave()", "Enqueue event", "id", id->getNodeName());
					membershipServiceImpl_SPtr->queueForDelivery(nl_SPtr);
				}
				else
				{
					Trace_Debug(this,"notifyLeave()",
							"Membership-service skipped, wait for first view delivery", "id", id->getNodeName());
					//notifyAppViewChange();
				}
			}
		}
	}

	Trace_Exit(this,"notifyLeave()");
}

void MembershipManagerImpl::notifyJoin(NodeIDImpl_SPtr id, int64_t incarnationNum, event::AttributeMap_SPtr attrMap)
{
	using namespace event;

	Trace_Entry(this,"notifyJoin()", "id", NodeIDImpl::stringValueOf(id));

	{
		boost::recursive_mutex::scoped_lock lock(membership_mutex);

		if (!_closed )
		{
			//internal consumers

			if (intMemConsumer_FirstViewDelivered) //first view delivered?
			{
				MetaData_SPtr metadata_SPtr(new MetaData(attrMap, incarnationNum, spdr::event::STATUS_ALIVE));
				SCMembershipEvent join(SCMembershipEvent::Node_Join, id, metadata_SPtr);
				for (int i = 0; i < MembershipManager::IntMemConsumer_Max; ++i)
				{
					if (intMemConsumer_Vec[i])
					{
						intMemConsumer_Vec[i]->onMembershipEvent(join);
					}
				}
			}
			else
			{
				Trace_Debug(this,"notifyJoin()",
						"Internal-consumer skipped, wait for first view delivery",
						"id", id->getNodeName());
			}

			//Membership-service
			if (membershipServiceImpl_SPtr && !membershipServiceImpl_SPtr->isClosed())
			{
				if (membershipServiceImpl_SPtr->isFirstViewDelivered())
				{
					NodeID_SPtr id_SPtr = boost::static_pointer_cast<NodeID>(id);
					MetaData_SPtr metadata_SPtr(new MetaData(attrMap, incarnationNum, spdr::event::STATUS_ALIVE));
					MembershipEvent_SPtr nj_SPtr(new NodeJoinEvent(id_SPtr, metadata_SPtr));

					Trace_Debug(this,"notifyJoin()", "Enqueue event", "id", id->getNodeName());
					membershipServiceImpl_SPtr->queueForDelivery(nj_SPtr);
				}
				else
				{
					Trace_Debug(this,"notifyJoin()",
							"Membership-service skipped, wait for first view delivery",
							"id", id->getNodeName());
				}
			}
		}
	}

	Trace_Exit(this,"notifyJoin()");
}

void MembershipManagerImpl::notifyChangeOfMetadata()
{
	Trace_Entry(this,"notifyChangeOfMetadata()");

	{
		boost::recursive_mutex::scoped_lock lock(membership_mutex);

		if (!_closed)
		{
			//There may be a race between the FirstViewDeliveryTask and this one,
			//so better be on the safe side, view-change also delivers attributes

			bool internal = false;
			bool external = false;

			if (intMemConsumer_FirstViewDelivered) //first view delivered
			{
				for (int i = 0; i < MembershipManager::IntMemConsumer_Max; ++i)
				{
					if (intMemConsumer_Vec[i])
					{
						internal = true;
						break;
					}
				}
			}
			else
			{
				Trace_Debug(this,"notifyChangeOfMetadata()", "Internal-consumer skipped, waiting for first view delivery");
			}

			//Membership-service
			if ( membershipServiceImpl_SPtr && !membershipServiceImpl_SPtr->isClosed() )
			{
				if (membershipServiceImpl_SPtr->isFirstViewDelivered())
				{
					external = true;
				}
				else
				{
					Trace_Debug(this,"notifyChangeOfMetadata()", "Membership-service skipped, waiting for first view delivery");
				}
			}

			if (internal || external)
			{
				std::pair<SCViewMap_SPtr, event::ViewMap_SPtr > partial_view =
						attributeManager.prepareDifferentialNotification(internal, external);

				if (ScTraceBuffer::isDebugEnabled(tc_))
				{
					ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this,"notifyChangeOfMetadata()", "Deliver event");
					buffer->addProperty<bool>("int", internal);
					buffer->addProperty<bool>("ext", external);
					buffer->invoke();
				}

				if (partial_view.first && partial_view.first->size() > 0)
				{
					SCMembershipEvent cm(SCMembershipEvent::Change_of_Metadata, partial_view.first);

					TRACE_DEBUG4(this,"notifyChangeOfMetadata()", "Deliver internal event",
								"event", cm.toString());


					for (int i = 0; i < MembershipManager::IntMemConsumer_Max; ++i)
					{
						if (intMemConsumer_Vec[i])
						{
							intMemConsumer_Vec[i]->onMembershipEvent(cm);
						}
					}
				}
				else
				{
					Trace_Debug(this,"notifyChangeOfMetadata()", "Empty internal notification, skipping event delivery");
				}

				if (partial_view.second && partial_view.second->size() > 0)
				{
					event::MembershipEvent_SPtr cm_SPtr(new event::ChangeOfMetaDataEvent(partial_view.second));

					TRACE_DEBUG4(this,"notifyChangeOfMetadata()", "Enqueue external event", "event", cm_SPtr->toString());

					membershipServiceImpl_SPtr->queueForDelivery(cm_SPtr);
				}
				else
				{
					Trace_Debug(this,"notifyChangeOfMetadata()", "Empty external notification, skipping event delivery");
				}
			}
			else
			{
				Trace_Debug(this,"notifyChangeOfMetadata()", "No consumers");
				attributeManager.resetNotifyTaskScheduled();
			}
		}
		else
		{
			Trace_Debug(this,"notifyChangeOfMetadata()", "closed, ignoring");
		}
	}

	Trace_Exit(this,"notifyChangeOfMetadata()");
}

void MembershipManagerImpl::notifyViewChange()
{
	using namespace event;
	Trace_Entry(this, "notifyViewChange()");

	{
		boost::recursive_mutex::scoped_lock lock(membership_mutex);

		if (!_closed)
		{
			bool intView = false;
			bool intDiffNotify = false;
			bool extView = false;
			bool extDiffNotify = false;

			for (int i = 0; i < MembershipManager::IntMemConsumer_Max; ++i)
			{
				if (intMemConsumer_Vec[i])
				{
					if (intMemConsumer_FirstViewDelivered) //first view delivered
					{
						intDiffNotify = true;
					}
					else
					{
						intView = true;
					}
					break;
				}
			}

			if ( membershipServiceImpl_SPtr && !membershipServiceImpl_SPtr->isClosed() )
			{
				if (membershipServiceImpl_SPtr->isFirstViewDelivered())
				{
					extDiffNotify = true;
				}
				else
				{
					extView = true;
				}
			}


			if (intView || extView)
			{
				std::pair<SCViewMap_SPtr, event::ViewMap_SPtr > views = attributeManager.prepareFullViewNotification(
						intView, intDiffNotify, extView, extDiffNotify);

				if (intView && extView)
				{
					//Two full views
					if (views.first && views.first->size() > 0)
					{
						SCMembershipEvent vc(SCMembershipEvent::View_Change, views.first);

						for (int i = 0; i < MembershipManager::IntMemConsumer_Max; ++i)
						{
							if (intMemConsumer_Vec[i])
							{
								intMemConsumer_Vec[i]->onMembershipEvent(vc);
							}
						}
					}

					if (views.second && views.second->size() > 0)
					{
						event::MembershipEvent_SPtr vc_SPtr(new event::ViewChangeEvent(views.second));
						membershipServiceImpl_SPtr->queueForDelivery(vc_SPtr);
					}
				}
				else if (intView)
				{
					//internal full view, external diff
					if (views.first && views.first->size() > 0)
					{
						SCMembershipEvent vc(SCMembershipEvent::View_Change, views.first);

						for (int i = 0; i < MembershipManager::IntMemConsumer_Max; ++i)
						{
							if (intMemConsumer_Vec[i])
							{
								intMemConsumer_Vec[i]->onMembershipEvent(vc);
							}
						}
					}

					if (views.second && views.second->size() > 0)
					{
						event::MembershipEvent_SPtr cm_SPtr(new event::ChangeOfMetaDataEvent(views.second));
						membershipServiceImpl_SPtr->queueForDelivery(cm_SPtr);
					}
				}
				else if (extView)
				{
					//Internal diff, external full view
					if (views.first && views.first->size() > 0)
					{
						SCMembershipEvent vc(SCMembershipEvent::Change_of_Metadata, views.first);

						for (int i = 0; i < MembershipManager::IntMemConsumer_Max; ++i)
						{
							if (intMemConsumer_Vec[i])
							{
								intMemConsumer_Vec[i]->onMembershipEvent(vc);
							}
						}
					}

					if (views.second && views.second->size() > 0)
					{
						event::MembershipEvent_SPtr vc_SPtr(new event::ViewChangeEvent(views.second));
						membershipServiceImpl_SPtr->queueForDelivery(vc_SPtr);
					}
				}
			}

			if (intView)
			{
				intMemConsumer_FirstViewDelivered = true;
			}
		}
		else
		{
			Trace_Debug(this,"notifyViewChange()", "closed, ignoring");
		}
	}
	Trace_Exit(this, "notifyViewChange()");
}

/*
 * @see spdr::MembershipManager
 */
void MembershipManagerImpl::notifyForeignZoneMembership(
	const int64_t requestID, const String& zoneBusName,
	boost::shared_ptr<event::ViewMap > view, const bool lastEvent)
{
	using namespace event;

	if (ScTraceBuffer::isEntryEnabled(tc_))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::entry(this, "notifyForeignZoneMembership()");
		buffer->addProperty<int64_t>("reqID", requestID);
		buffer->addProperty("zone", zoneBusName);
		buffer->addProperty<bool>("last", lastEvent);
		buffer->invoke();
	}

	{
		boost::recursive_mutex::scoped_lock lock(membership_mutex);

		if (!_closed )
		{
			//Not delivered to internal consumers
			//Membership-service
			if (membershipServiceImpl_SPtr && !membershipServiceImpl_SPtr->isClosed())
			{
				MembershipEvent_SPtr fzm_SPtr(new ForeignZoneMembershipEvent(requestID,zoneBusName,view,lastEvent));
				Trace_Debug(this,"notifyForeignZoneMembership()","success", "reqID", ScTraceBuffer::stringValueOf(requestID));
				membershipServiceImpl_SPtr->queueForDelivery(fzm_SPtr);
			}
		}
		else
		{
			Trace_Debug(this,"notifyForeignZoneMembership()", "closed, ignoring");
		}
	}

	Trace_Exit(this, "notifyForeignZoneMembership()");
}


/*
 * @see spdr::MembershipManager
 */
void MembershipManagerImpl::notifyForeignZoneMembership(
	const int64_t requestID, const String& zoneBusName,
	event::ErrorCode errorCode, const String& errorMessage, bool lastEvent)
{
	using namespace event;

	if (ScTraceBuffer::isEntryEnabled(tc_))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::entry(this, "notifyForeignZoneMembership()");
		buffer->addProperty<int64_t>("reqID", requestID);
		buffer->addProperty("Zone", zoneBusName);
		buffer->addProperty("Error", event::SpiderCastEvent::errorCodeName[errorCode]);
		buffer->addProperty("Message", errorMessage);
		buffer->addProperty<bool>("last", lastEvent);
		buffer->invoke();
	}

	{
		boost::recursive_mutex::scoped_lock lock(membership_mutex);

		if (!_closed )
		{
			//Not delivered to internal consumers
			//Membership-service
			if (membershipServiceImpl_SPtr && !membershipServiceImpl_SPtr->isClosed())
			{
				MembershipEvent_SPtr fzm_SPtr(new ForeignZoneMembershipEvent(requestID,zoneBusName,errorCode,errorMessage,lastEvent));
				Trace_Debug(this,"notifyForeignZoneMembership()","failure", "reqID", ScTraceBuffer::stringValueOf(requestID));
				membershipServiceImpl_SPtr->queueForDelivery(fzm_SPtr);
			}
		}
		else
		{
			Trace_Debug(this,"notifyForeignZoneMembership()", "closed, ignoring");
		}
	}

	Trace_Exit(this, "notifyForeignZoneMembership()");
}

/*
 * @see spdr::MembershipManager
 */
void MembershipManagerImpl::notifyZoneCensus(
	const int64_t requestID, event::ZoneCensus_SPtr census, const bool full)
{
	using namespace event;
	if (ScTraceBuffer::isEntryEnabled(tc_))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::entry(this, "notifyZoneCensus()");
		buffer->addProperty<int64_t>("reqID", requestID);
		buffer->addProperty<bool>("full", full);
		buffer->invoke();
	}

	{
		boost::recursive_mutex::scoped_lock lock(membership_mutex);

		if (!_closed )
		{
			//Not delivered to internal consumers
			//Membership-service
			if (membershipServiceImpl_SPtr && !membershipServiceImpl_SPtr->isClosed())
			{
				MembershipEvent_SPtr zc_SPtr(new ZoneCensusEvent(requestID,census,full));
				Trace_Debug(this,"notifyZoneCensus()","enqueue", "reqID", ScTraceBuffer::stringValueOf(requestID));
				membershipServiceImpl_SPtr->queueForDelivery(zc_SPtr);
			}
		}
		else
		{
			Trace_Debug(this,"notifyZoneCensus()", "closed, ignoring");
		}
	}

	Trace_Exit(this, "notifyZoneCensus()");
}



//=== process messages ===
bool MembershipManagerImpl::processMsgLeave(StringSPtr name, const NodeVersion& ver, spdr::event::NodeStatus node_status)
{
	Trace_Entry(this, "processMsgLeave()");

	Trace_Event(this, "processMsgLeave()",
			"name", *name,
			"version", ver.toString(),
			"status", spdr::stringValueOf(node_status));

	bool view_changed = false;

	NodeIDImpl_SPtr id = _nodeIDCache.getOrCreate(*name);

	if (*name != myID->getNodeName())
	{
		MembershipViewMap::iterator pos =  viewMap.find(id);
		if (pos != viewMap.end())
		{
			if (pos->second.nodeVersion <= ver)
			{
				Trace_Event(this, "processMsgLeave()", "in view, version number not stale");
				//leave, add to left-set only if in view, to avoid cycles
				viewRemoveNode(id, ver, node_status);
				updateDB.addToLeft(id->getNodeName(),ver, node_status);
				view_changed = true;
			}
			else
			{
				//old-leave, ignore
				Trace_Event(this, "processMsgLeave()", "version number is stale, old leave, ignored");
			}
		}
		else
		{
			Trace_Event(this, "processMsgLeave()", "not in view, update history");

			//not in view, update history to record maximal version number and update status
			//if status changes from:
			//Suspect to Leave; or Suspect to Removed; deliver a leave message
			std::pair<bool,bool> newInfo = nodeHistorySet.updateVer(
					id, ver, node_status,
					boost::posix_time::microsec_clock::universal_time());

			if (newInfo.first) //updated
			{
				updateDB.addToLeft(id->getNodeName(), ver, node_status);
				std::pair<NodeInfo,bool> historyInfo = nodeHistorySet.getNodeInfo(id);
				notifyLeave(id,ver,node_status,historyInfo.first.attributeTable);
			}

			if (!newInfo.second) //not retained
			{
				_nodeIDCache.remove(id);
			}
		}
	}
	else
	{
		if (ver.getIncarnationNumber() >= myVersion.getIncarnationNumber()) //SplitBrain
		{
			if (node_status == event::STATUS_SUSPECT_DUPLICATE_NODE)
			{
				std::ostringstream errMsg;
				errMsg << "Duplicate node (AKA 'Split Brain') was detected with a Leave message."
						<< " Another node with the same name but a higher incarnation number was detected."
						<< " This node is shutting down, the other node will probably continue.";
				Trace_Error(this,"processMsgLeave()",errMsg.str());
				_coreInterface.componentFailure(errMsg.str(), event::Duplicate_Local_Node_Detected);
			}
			else
			{
				//This should not normally happen
				std::ostringstream errMsg;
				errMsg << "Encountered a leave message on my self, with a higher incarnation number."
						<< " Duplicate node (AKA 'Split Brain') is suspected."
						<< " Another node with the same name but a higher incarnation has just left."
						<< " This node is shutting down.";
				Trace_Error(this,"processMsgLeave()",errMsg.str());
				_coreInterface.componentFailure(errMsg.str(), event::Duplicate_Local_Node_Detected);

//				String what("Error: Encountered a leave message on my self");
//				Trace_Error(this, "processMsgLeave()", what);
//				throw SpiderCastRuntimeError(what);
			}
		}
		else
		{
			Trace_Event(this, "processMsgLeave()", "Encountered a leave message on myself, old incarnation, ignored");
		}
	}

	Trace_Exit<bool>(this, "processMsgLeave()",view_changed);

	return view_changed;
}

bool MembershipManagerImpl::processMsgNodeUpdate(SCMessage_SPtr membershipMsg)
{
	Trace_Entry(this, "processMsgNodeUpdate()");

	bool view_changed = false;
	boost::shared_ptr<ByteBuffer> bb_SPtr = membershipMsg->getBuffer();

	const int left_num = (*bb_SPtr).readInt();
	Trace_Debug(this, "processMsgNodeUpdate()","Left",
			"num",boost::lexical_cast<std::string>(left_num),
			"buff-pos",boost::lexical_cast<std::string>(bb_SPtr->getPosition()));

	for (int k=0; k<left_num; k++)
	{
		StringSPtr name = (*bb_SPtr).readStringSPtr();
		NodeVersion ver = (*membershipMsg).readNodeVersion();
		spdr::event::NodeStatus node_status = static_cast<spdr::event::NodeStatus>(bb_SPtr->readInt());
		Trace_Debug(this, "processMsgNodeUpdate()", "Left-item",
				"node", *name,
				"version", ver.toString(),
				"status", boost::lexical_cast<std::string>(node_status));

		//processing a leave item is the same as a leave message.
		bool res = processMsgLeave(name,ver, node_status);
		view_changed = view_changed || res;
	}

	const int alive_num = (*bb_SPtr).readInt();
	Trace_Debug(this, "processMsgNodeUpdate()","Alive","num",boost::lexical_cast<std::string>(alive_num),
			"buff-pos",boost::lexical_cast<std::string>(bb_SPtr->getPosition()));
	for (int k=0; k<alive_num; k++)
	{
		NodeIDImpl_SPtr id = (*membershipMsg).readNodeID();
		NodeVersion ver = (*membershipMsg).readNodeVersion();
		Trace_Debug(this, "processMsgNodeUpdate()", "Alive-item",
						"node", NodeIDImpl::stringValueOf(id),
						"version", ver.toString());

		//process alive item, same as in discovery
		bool added = viewMergeAliveNode(id,ver);
		view_changed = view_changed || added;
	}

	const int suspicion_num = (*bb_SPtr).readInt();
	Trace_Debug(this, "processMsgNodeUpdate()","Susp","num",boost::lexical_cast<std::string>(suspicion_num),
			"buff-pos",boost::lexical_cast<std::string>(bb_SPtr->getPosition()));
	for (int k=0; k<suspicion_num; k++)
	{
		StringSPtr reportingName = (*bb_SPtr).readStringSPtr();
		StringSPtr suspectName = (*bb_SPtr).readStringSPtr();
		NodeVersion suspectVer = (*membershipMsg).readNodeVersion();
		Trace_Debug(this, "processMsgNodeUpdate()", "Suspicion-item",
						"reporting", *reportingName,
						"suspect", *suspectName,
						"suspect-version", suspectVer.toString());

		if (*suspectName == myID->getNodeName())
		{
			//TODO SplitBrain handle split brain - if incarnation of suspect is higher than myVer.inc, another node with a higher version exists
			if (suspectVer.getIncarnationNumber() > myVersion.getIncarnationNumber())
			{
				std::ostringstream errMsg;
				errMsg << "Duplicate node (AKA 'Split Brain') was detected by a Suspect message."
						<< " Another node with the same name but a higher incarnation number was detected."
						<< " This node is shutting down, the other node will probably continue.";
				Trace_Error(this,"processMsgNodeUpdate()",errMsg.str(),
						"local-inc", boost::lexical_cast<std::string>(myVersion.getIncarnationNumber()),
						"remote-inc", boost::lexical_cast<std::string>(suspectVer.getIncarnationNumber()));
				_coreInterface.componentFailure(errMsg.str(), event::Duplicate_Local_Node_Detected);
			}
			else
			{
				myVersion.addToMinorVersion(1);
				updateDB.addToAlive(myID,myVersion);
				viewMap[myID].nodeVersion = myVersion;
				attributeManager.writeMyRebuttalKey();

				//TODO schedule an urgent rebuttal task
				Trace_Event(this, "processMsgNodeUpdate()",
						"Suspicion on me!, increased my version, written attribute-rebuttal-key & propagate",
						"version", myVersion.toString());
			}
		}
		else
		{
			bool res = viewProcessSuspicion(reportingName,suspectName,suspectVer);
			view_changed = view_changed || res;
		}
	}

	//RetainAtt
	const int retained_num = bb_SPtr->readInt();
	Trace_Debug(this, "processMsgNodeUpdate()","Retained","num",boost::lexical_cast<std::string>(retained_num),
			"buff-pos",boost::lexical_cast<std::string>(bb_SPtr->getPosition()));
	if (_config.isRetainAttributesOnSuspectNodesEnabled())
	{
		for (int k=0; k<retained_num; ++k)
		{
			NodeIDImpl_SPtr id = membershipMsg->readNodeID();
			NodeVersion ver = membershipMsg->readNodeVersion();
			spdr::event::NodeStatus status =
					static_cast<spdr::event::NodeStatus>(bb_SPtr->readInt());

			bool res = historyProcessRetain(id,ver,status);
			view_changed = view_changed | res;
		}
	}

	if (ScTraceBuffer::isEventEnabled(tc_))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::event(this, "processMsgNodeUpdate()", "Summary");
		buffer->addProperty("sender", membershipMsg->getSender()->getNodeName() );
		buffer->addProperty<int>("#left", left_num);
		buffer->addProperty<int>("#alive", alive_num);
		buffer->addProperty<int>("#susp", suspicion_num);
		buffer->addProperty<int>("#ret", retained_num);
		buffer->addProperty<bool>("view-changed", view_changed);
		buffer->addProperty("buff-pos",bb_SPtr->getPosition());
		buffer->invoke();
	}

	Trace_Exit(this, "processMsgNodeUpdate()");
	return view_changed;
}

//=== view manipulation ===

/**
 * Called when a suspicion on another node (not current node) is encountered, via a gossip message.
 *
 * Threading: MemTopoThread.
 *
 * @param reportingName
 * @param suspectName
 * @param suspectVer
 * @return true if the view size changed, false otherwise
 */
bool MembershipManagerImpl::viewProcessSuspicion(
		StringSPtr reportingName,
		StringSPtr suspectName,
		NodeVersion suspectVer)
{
	Trace_Entry(this, "viewProcessSuspicion()");

	Trace_Event(this, "viewProcessSuspicion()", "suspicion:",
			"reporting", *reportingName,
			"suspect", *suspectName,
			"version", suspectVer.toString());

	bool view_changed = false;

	NodeIDImpl_SPtr tmp_id = _nodeIDCache.getOrCreate(*suspectName);
	MembershipViewMap::iterator pos = viewMap.find(tmp_id);
	if (pos != viewMap.end())
	{
		if (pos->second.nodeVersion <= suspectVer)
		{
			bool new_suspicion = pos->second.suspicionList.add(reportingName, suspectVer);
			if (new_suspicion)
			{
				updateDB.addToSuspect(reportingName,suspectName,suspectVer);
			}

			//update suspicionThreshold for small views
			int suspicionThreshold = _config.getSuspicionThreshold();
			if ((int) viewMap.size() <= suspicionThreshold + 1)
			{
				suspicionThreshold = 1;
			}

			if (pos->second.suspicionList.size() >= suspicionThreshold )
			{
				Trace_Event(this, "viewProcessSuspicion()", "removing suspect from view",
						"current-version", pos->second.nodeVersion.toString());
				viewRemoveNode(tmp_id, suspectVer, spdr::event::STATUS_SUSPECT);
				view_changed = true;
			}
		}
		else
		{
			Trace_Event(this, "viewProcessSuspicion()", "stale suspicion, ignoring",
					"current-version", pos->second.nodeVersion.toString());
		}
	}
	else
	{
		//check history to see if I need to propagate it? (No)
		Trace_Event(this, "viewProcessSuspicion()", "suspect not in view, ignoring");
	}

	Trace_Exit<bool>(this, "viewProcessSuspicion()", view_changed);
	return view_changed;
}

/**
 * Called when a node-liveness indication is received, via discovery or membership message.
 *
 * Threading: MemTopoThread.
 *
 * @param id
 * @param ver
 * @return true if the view size changed, false otherwise
 */
bool MembershipManagerImpl::viewMergeAliveNode(NodeIDImpl_SPtr id, const NodeVersion& ver)
{
	Trace_Entry(this,"viewMergeAliveNode()");

	//SplitBrain detect a duplicate node: if (id == myID) and (ver.inc > myVer.inc) another node with a higher incarnation number is there.
	if (*id == *myID)
	{
		Trace_Debug(this, "viewMergeAliveNode()", "On myNodeID",
				"local-ver", myVersion.toString(), "remote-ver", ver.toString());

		if (ver.getIncarnationNumber() > myVersion.getIncarnationNumber())
		{
			std::ostringstream errMsg;
			errMsg << "Duplicate node (AKA 'Split Brain') was detected by an Alive message."
					<< " Another node with the same name but a higher incarnation number was detected."
					<< " This node is shutting down, the other node will probably continue.";
			Trace_Error(this,"viewMergeAliveNode()",errMsg.str(),
					"local-inc", boost::lexical_cast<std::string>(myVersion.getIncarnationNumber()),
					"remote-inc", boost::lexical_cast<std::string>(ver.getIncarnationNumber()));
			_coreInterface.componentFailure(errMsg.str(), event::Duplicate_Local_Node_Detected);
		}
		else if (ver > myVersion) //equal incarnation
		{
			std::string errMsg("Error: Alive message on this node has a version number higher then the local version.");
			Trace_Error(this, "viewMergeAliveNode()",errMsg,
					"local-ver", myVersion.toString(), "remote-ver", ver.toString());
			throw SpiderCastRuntimeError(errMsg);
		}

		return false;
		Trace_Exit<bool>(this,"viewMergeAliveNode()",false);
	}


	bool view_change = false;
	MembershipViewMap::iterator info_iter =  viewMap.find(id);

	if (info_iter == viewMap.end()) //not in view
	{
		if (!nodeHistorySet.containsVerGreaterEqual(id,ver))
		{
			updateDB.addToAlive(id, ver);
			bool added = viewAddNode(id,ver);
			if (!added)
			{
				Trace_Error(this,"viewMergeAliveNode()","Error: failed to add node to view",
						"node", id->toString(), "ver", ver.toString());
				throw SpiderCastRuntimeError("Error: failed to add node to view");
				//assert(added);
			}
			view_change = true;
			Trace_Event(this,"viewMergeAliveNode()", "added to view"
					"node", NodeIDImpl::stringValueOf(id),
					"version", ver.toString());
		}
		else
		{
			Trace_Event(this,"viewMergeAliveNode()",
					"history contains higher or equal version, recently dead, ignoring"
					"node", NodeIDImpl::stringValueOf(id),
					"version", ver.toString());
		}
	}
	else if (info_iter->second.nodeVersion < ver) //in-view
	{
		updateDB.addToAlive(id, ver);
		//a newer version, check the incarnation number and minor version
		if (info_iter->second.nodeVersion.getIncarnationNumber() == ver.getIncarnationNumber())
		{
			//update the minor version
			info_iter->second.nodeVersion = ver;
			info_iter->second.suspicionList.deleteOlder(ver);
			Trace_Debug(this,"viewMergeAliveNode()",
					"already in view, minor version updated",
					"node", NodeIDImpl::stringValueOf(id),
					"version", ver.toString());
		}
		else if (info_iter->second.nodeVersion.getIncarnationNumber() < ver.getIncarnationNumber())
		{
			Trace_Event(this,"viewMergeAliveNode()",
					"already in view, incarnation number increased"
					"node", NodeIDImpl::stringValueOf(id),
					"version", ver.toString());

			//it is a new incarnation, meaning node left and joined
			NodeVersion old_ver = info_iter->second.nodeVersion;

			bool ok = false;

			//leave
			ok = viewRemoveNode(id, old_ver, spdr::event::STATUS_SUSPECT);
			if (!ok)
			{
				std::ostringstream oss;
				oss << "ID=" << NodeIDImpl::stringValueOf(id)
					<< " Ver=" << old_ver.toString() << " not removed from view (not found)" << std::endl;
				ScTraceBufferAPtr buffer = ScTraceBuffer::event(this, "viewMergeAliveNode()", oss.str());
				buffer->invoke();
				throw SpiderCastRuntimeError(oss.str());
			}

			//join
			ok = viewAddNode(id,ver);
			if (!ok)
			{
				std::ostringstream oss;
				oss << "ID=" << NodeIDImpl::stringValueOf(id)
				<< " Ver=" << ver.toString() << " not added to view (already exists)" << std::endl;
				ScTraceBufferAPtr buffer = ScTraceBuffer::event(this, "viewMergeAliveNode()", oss.str());
				buffer->invoke();
				throw SpiderCastRuntimeError(oss.str());
			}

			view_change = true; //Trigger a refresh of the successor list
		}
	}
	else
	{
		Trace_Debug(this,"viewMergeAliveNode()",
				"already in view with higher or equal version, ignoring",
				"node", NodeIDImpl::stringValueOf(id),
				"version", ver.toString());
	}

	Trace_Exit<bool>(this,"viewMergeAliveNode()",view_change);
	return view_change;
}

/*
 * Assumes node IS NOT in the view map.
 * Does not overwrite.
 * Insert to view-map, ring-set, mark bootstrap, remove from history, add to node-ID-cache, notify app
 */
bool MembershipManagerImpl::viewAddNode(NodeIDImpl_SPtr id, const NodeVersion& ver)
{
	Trace_Entry(this, "viewAddNode()");

	bool added = false;
	NodeInfo info;
	info.nodeVersion = ver;
	std::pair<MembershipViewMap::iterator, bool> result = viewMap.insert(std::make_pair(id,info));
	if (result.second)
	{
		util::VirtualID_SPtr vid = _nodeVirtualIDCache.get(id->getNodeName());
		std::pair<MembershipRing::iterator, bool> res = ringSet.insert(std::make_pair(vid,id));
		Trace_Debug(this, "viewAddNode()", "ring", "added", ScTraceBuffer::stringValueOf(res.second));

		//bootstrapSet.setInView(id, true);
		bootstrap->setInView(id, true);

		//RetainAtt
		bool retained = false;
		if (_config.isRetainAttributesOnSuspectNodesEnabled())
		{
			pair<NodeInfo, bool> retainedInfo = nodeHistorySet.getNodeInfo(id);
			if (retainedInfo.second
					&& (result.first->second.nodeVersion.getIncarnationNumber()
							== retainedInfo.first.nodeVersion.getIncarnationNumber())
					&& retainedInfo.first.attributeTable)
			{
				result.first->second.attributeTable =
						retainedInfo.first.attributeTable;
				retained = true;
			}
		}
		nodeHistorySet.remove(id);
		_nodeIDCache.put(id);

		//Notify also the attributes, in case retained where transfered from history to view
		event::AttributeMap_SPtr attrMap;
		if (retained)
		{
			attrMap = result.first->second.attributeTable->getAttributeMap4Notification();
		}
		notifyJoin(id, info.nodeVersion.getIncarnationNumber(), attrMap);
		added = true;

		Trace_Event(this, "viewAddNode()", "Node added to view",
				"node", NodeIDImpl::stringValueOf(id),
				"version", ver.toString(),
				"added", ScTraceBuffer::stringValueOf(added),
				"retained", ScTraceBuffer::stringValueOf(retained));
	}
	else
	{
		added = false;

		Trace_Event(this, "viewAddNode()", "Warning: node already in view, not added",
					"node", NodeIDImpl::stringValueOf(id),
					"version", ver.toString(),
					"added", ScTraceBuffer::stringValueOf(added));
	}


	Trace_Exit<bool>(this, "viewAddNode()",added);
	return added;
}

/*
 * Assume node IS in the view map.
 * Assumes the version in the map is <= then the version parameter.
 * Remove from view-map, ring-set, mark bootstrap, add to history, remove from node-ID-cache, notify app
 */
bool MembershipManagerImpl::viewRemoveNode(NodeIDImpl_SPtr id, const NodeVersion& ver, spdr::event::NodeStatus status)
{
	Trace_Entry(this, "viewRemoveNode()");

	bool removed = false;
	bool retained = false;
	MembershipViewMap::iterator pos =  viewMap.find(id);
	if (pos != viewMap.end())
	{
		NodeInfo erased_node_info = pos->second;

		if (erased_node_info.nodeVersion > ver)
		{
			Trace_Event(this, "viewRemoveNode()", "Node version in map is higher then parameter.",
					"version", ver.toString(), "erased-version", erased_node_info.nodeVersion.toString());
			String what("viewRemoveNode - Node version in map is higher then parameter.");
			throw SpiderCastRuntimeError(what);
		}

		viewMap.erase(pos);
		util::VirtualID_SPtr vid = _nodeVirtualIDCache.get(id->getNodeName());
		ringSet.erase(vid);
		//bootstrapSet.setInView(id, false);
		bootstrap->setInView(id, false);

		NodeInfo historyInfo(ver, status, boost::posix_time::microsec_clock::universal_time());
		if (_config.isRetainAttributesOnSuspectNodesEnabled() && status != spdr::event::STATUS_REMOVE)
		{
			historyInfo.attributeTable = erased_node_info.attributeTable;
			retained = true;
		}

		nodeHistorySet.add(id, historyInfo);

		if (!retained)
		{
			_nodeIDCache.remove(id);
		}

		notifyLeave(id, ver, status, historyInfo.attributeTable);
		removed = true;
	}
	else
	{
		removed = false;
	}

	Trace_Event(this, "viewRemoveNode()", "",
			"node", NodeIDImpl::stringValueOf(id),
			"version", ver.toString(),
			"removed", ScTraceBuffer::stringValueOf(removed),
			"retained", ScTraceBuffer::stringValueOf(retained));

	Trace_Exit<bool>(this, "viewRemoveNode()",removed);
	return removed;
}

bool MembershipManagerImpl::historyProcessRetain(
		NodeIDImpl_SPtr id, NodeVersion ver, spdr::event::NodeStatus status)
{
	Trace_Entry(this, "historyProcessRetain()");

	bool change = false;

	MembershipViewMap::iterator view_info_iter = viewMap.find(id);
	if (view_info_iter == viewMap.end())
	{
		if (!nodeHistorySet.contains(id))
		{
			_nodeIDCache.put(id);

			NodeInfo historyInfo(
					ver, status,
					boost::posix_time::microsec_clock::universal_time());
			if (status != spdr::event::STATUS_REMOVE)
			{
				historyInfo.attributeTable.reset(new AttributeTable);
			}

			change = nodeHistorySet.add(id, historyInfo);
			Trace_Debug(this, "historyProcessRetain()",
					(change ? "Ret-item, added to history" : "Ret-item, not added to history"),
					"node", NodeIDImpl::stringValueOf(id),
					"version", ver.toString(),
					"status", ScTraceBuffer::stringValueOf(status));
			if (change)
			{
				updateDB.addToRetained(id,ver,status);
				notifyLeave(id,ver,status,historyInfo.attributeTable);
			}
		}
		else
		{
			std::pair<bool,bool> update = nodeHistorySet.updateVer(id,ver,status,boost::posix_time::microsec_clock::universal_time());
			change = update.first;
			if (change)
			{
				std::pair<NodeInfo,bool> historyInfo = nodeHistorySet.getNodeInfo(id);
				updateDB.addToRetained(id,ver,status);
				notifyLeave(id,ver,status,historyInfo.first.attributeTable);
			}
		}

	}
	else
	{
		if (*id != *myID)
		{
			Trace_Debug(this, "historyProcessRetain()",
					"Ret-item, in view, ignored",
					"node",	NodeIDImpl::stringValueOf(id),
					"version", ver.toString(),
					"status", ScTraceBuffer::stringValueOf(status));
		}
		else
		{
			myVersion.addToMinorVersion(1);
			updateDB.addToAlive(myID,myVersion);
			viewMap[myID].nodeVersion = myVersion;
			attributeManager.writeMyRebuttalKey();

			//TODO schedule an urgent rebuttal task
			Trace_Event(this, "historyProcessRetain()",
					"Retained item on me!, increased my version, written attribute-rebuttal key & propagate",
					"version", myVersion.toString());
		}
	}

	Trace_Exit<bool>(this, "historyProcessRetain()",change);
	return change;
}

//=== successor list ===
void MembershipManagerImpl::refreshSuccessorList()
{
	Trace_Entry(this, "refreshSuccessorList()");

	//TODO implement successor list
	if (ringSet.size() > 1)
	{
		MembershipRing::const_iterator pos = ringSet.find(myVID);
		if (pos == ringSet.end())
		{
			String what("Cannot find my NodeID in ring-set, ");
			what.append(myID->getNodeName());
			Trace_Exit(this, "refreshSuccessorList()","SpiderCastRuntimeError", what);

			throw SpiderCastRuntimeError(what);
		}

		MembershipRing::const_reverse_iterator rpos(pos);


		//successor
		++pos;
		if (pos == ringSet.end())
		{
			pos = ringSet.begin();
		}

		//predecessor
		if (rpos == ringSet.rend())
		{
			rpos = ringSet.rbegin();
		}

		Trace_Event(this, "refreshSuccessorList()",
				"successor", toString<NodeIDImpl>(pos->second),
				"predecessor", toString<NodeIDImpl>(rpos->second));
		topoMgr_SPtr->setSuccessor(pos->second, rpos->second); //FIXME NLP after terminate from same thread
	}
	else
	{
		//when the view is shrinking we need to set the
		//successor to null if the view size goes to 1.
		Trace_Event(this, "refreshSuccessorList()", "view-size=1, successor is null");
		topoMgr_SPtr->setSuccessor(NodeIDImpl_SPtr(), NodeIDImpl_SPtr());
	}

	Trace_Exit(this, "refreshSuccessorList()");
}

//=== sending messages ===

void MembershipManagerImpl::sendLeaveMsg(int32_t exitCode)
{
	Trace_Entry(this, "sendLeaveMsg()");

	prepareLeaveMsg(outgoingMemMessage, exitCode);
	//ignore send failures, we're leaving
	neighborTable_SPtr->sendToAllNeighbors(outgoingMemMessage);

	Trace_Exit(this, "sendLeaveMsg()");
}

void MembershipManagerImpl::sendLeaveAckMsg(NodeIDImpl_SPtr target)
{
	Trace_Entry(this, "sendLeaveAckMsg()");

	ByteBuffer& bb = *(outgoingMemMessage->getBuffer());
	outgoingMemMessage->writeH1Header(SCMessage::Type_Mem_Node_Leave_Ack);
	bb.writeString(target->getNodeName());
	outgoingMemMessage->updateTotalLength();
	if (_config.isCRCMemTopoMsgEnabled())
	{
		outgoingMemMessage->writeCRCchecksum();
	}

	neighborTable_SPtr->sendToNeighbor(target, outgoingMemMessage);

	Trace_Exit(this, "sendLeaveAckMsg()");
}

void MembershipManagerImpl::prepareFullViewMsg(SCMessage_SPtr msg)
{
	Trace_Entry(this, "prepareFullViewMsg()");

	std::ostringstream oss;

	(*msg).writeH1Header(SCMessage::Type_Mem_Node_Update);
	ByteBuffer& bb = *((*msg).getBuffer());

	bb.writeInt(0); //#left

	bb.writeInt(viewMap.size()); //#Alive

	if (ScTraceBuffer::isEventEnabled(tc_))
	{
		oss << "#Alive=" << viewMap.size() << ", ";
	}

	for (MembershipViewMap::iterator it = viewMap.begin(); it != viewMap.end(); ++it)
	{
		(*msg).writeNodeID(it->first);
		(*msg).writeNodeVersion(it->second.nodeVersion);

		if (ScTraceBuffer::isDebugEnabled(tc_))
		{
				oss << it->first->getNodeName() << " "
					<< it->second.nodeVersion.toString() <<", ";
		}
	}

	std::ostringstream oss_suspicions;
	int num_suspicions_pos = bb.getPosition();
	bb.writeInt(0); //#suspect skip
	int num_suspicions = 0;
	for (MembershipViewMap::iterator it = viewMap.begin(); it != viewMap.end(); ++it)
	{
		it->second.suspicionList.writeToMessage(it->first->getNodeName(), *msg);
		int list_size = it->second.suspicionList.size();
		num_suspicions += list_size;

		if (ScTraceBuffer::isDebugEnabled(tc_) && list_size > 0)
		{
			oss_suspicions << "Sus=" << it->first->getNodeName() << " "
					<< it->second.suspicionList.toString() <<", ";
		}
	}
	int end_pos = bb.getPosition();
	bb.setPosition(num_suspicions_pos);
	bb.writeInt(num_suspicions);
	bb.setPosition(end_pos);

	//AttRetain write retained history nodes
	std::ostringstream oss_retained;
	int num_retained = 0;
	int num_retained_pos = bb.getPosition();
	bb.writeInt(num_retained);
	if (_config.isRetainAttributesOnSuspectNodesEnabled())
	{
		NodeHistoryMap& historyMap = nodeHistorySet.getNodeHistoryMap();

		for (NodeHistoryMap::const_iterator it = historyMap.begin(); it != historyMap.end(); ++it)
		{
			(*msg).writeNodeID(it->first);
			(*msg).writeNodeVersion(it->second.nodeVersion);
			bb.writeInt(it->second.status);
			++num_retained;

			if (ScTraceBuffer::isDebugEnabled(tc_))
			{
				oss_retained << "Ret=" << it->first->getNodeName() << " "
						<< it->second.nodeVersion.toString() << " " << it->second.status <<", ";
			}
		}

		if (num_retained > 0)
		{
			end_pos = bb.getPosition();
			bb.setPosition(num_retained_pos);
			bb.writeInt(num_retained);
			bb.setPosition(end_pos);
		}
	}

	(*msg).updateTotalLength();
	if (_config.isCRCMemTopoMsgEnabled())
	{
		(*msg).writeCRCchecksum();
	}

	if (ScTraceBuffer::isEventEnabled(tc_))
	{
		oss << " #Suspicions=" << num_suspicions << ", " << oss_suspicions.str();
	}


	if (ScTraceBuffer::isEventEnabled(tc_))
	{
		oss << " #Retained=" << num_retained << ", " << oss_retained.str();
	}

	if (ScTraceBuffer::isEventEnabled(tc_))
	{
	    ScTraceBufferAPtr buffer = ScTraceBuffer::event(this, "prepareFullViewMsg()");
	    buffer->addProperty("message", msg->toString());
	    buffer->addProperty("content", oss.str());
	    buffer->invoke();
	}

	Trace_Exit(this, "prepareFullViewMsg()");
}

void MembershipManagerImpl::prepareLeaveMsg(SCMessage_SPtr msg, int32_t exitCode)
{
	Trace_Entry(this, "prepareLeaveMsg()");

	ByteBuffer& bb = *(msg->getBuffer());
	msg->writeH1Header(SCMessage::Type_Mem_Node_Leave);
	bb.writeString(myID->getNodeName());
	msg->writeNodeVersion(myVersion);
	bb.writeInt(exitCode);

	msg->updateTotalLength();
	if (_config.isCRCMemTopoMsgEnabled())
	{
		msg->writeCRCchecksum();
	}

	Trace_Exit(this, "prepareLeaveMsg()");
}

void MembershipManagerImpl::prepareUpdateViewMsg(SCMessage_SPtr msg)
{
	Trace_Entry(this, "prepareUpdateViewMsg()");

	(*msg).writeH1Header(SCMessage::Type_Mem_Node_Update);
	updateDB.writeToMessage(msg);

	Trace_Exit(this, "prepareUpdateViewMsg()");
}

void MembershipManagerImpl::verifyCRC(SCMessage_SPtr msg, const String& method, const String& desc)
{
	if (_config.isCRCMemTopoMsgEnabled())
	{
		try
		{
			msg->verifyCRCchecksum();
		}
		catch (MessageUnmarshlingException& e)
		{
			e.printStackTrace();
			ScTraceBufferAPtr buffer = ScTraceBuffer::event(this, method, desc);
			buffer->addProperty("msg", msg->toString());
			buffer->addProperty("sender", toString<NodeIDImpl>(msg->getSender()));
			buffer->addProperty<uint64_t>("sid", msg->getStreamId());
			buffer->addProperty("what", e.what());
			buffer->invoke();
			throw;
		}
	}
}

}//namespace spdr

