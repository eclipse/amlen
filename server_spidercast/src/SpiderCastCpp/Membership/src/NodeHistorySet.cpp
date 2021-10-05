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
 * NodeHistorySet.cpp
 *
 *  Created on: 18/04/2010
 */

#include "NodeHistorySet.h"

namespace spdr
{

NodeHistorySet::NodeHistorySet() :
	validIterator_(false)
{
}

NodeHistorySet::~NodeHistorySet()
{
}


std::pair<NodeInfo,bool> NodeHistorySet::getNodeInfo(NodeIDImpl_SPtr node) const
{
	NodeHistoryMap::const_iterator it = historyMap_.find(node);
	if (it != historyMap_.end())
	{
		return std::make_pair(it->second, true);
	}
	else
	{
		return std::make_pair(NodeInfo(NodeVersion(),spdr::event::STATUS_ALIVE,boost::date_time::not_a_date_time), false);
	}
}


bool NodeHistorySet::remove(NodeIDImpl_SPtr node)
{
	validIterator_ = false;
	return (historyMap_.erase(node) > 0);
}

bool NodeHistorySet::add(NodeIDImpl_SPtr node, const NodeInfo& info)
{

	NodeHistoryMap::iterator pos = historyMap_.find(node);
	if (pos != historyMap_.end())
	{
		//over-write the version, only if version is higher
		if (pos->second.nodeVersion < info.nodeVersion)
		{
			pos->second = info;
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		validIterator_ = false;
		historyMap_[node] = info;
		return true;
	}
}

std::pair<bool,bool> NodeHistorySet::updateVer(NodeIDImpl_SPtr node, NodeVersion ver, spdr::event::NodeStatus status,
		boost::posix_time::ptime time)
{
	bool updated = false;
	bool retained = false;

	NodeHistoryMap::iterator pos = historyMap_.find(node);
	if (pos != historyMap_.end())
	{
		if (pos->second.nodeVersion <= ver)
		{
			//removal of a retained attribute
			if ((status == spdr::event::STATUS_REMOVE) && pos->second.attributeTable)
			{
				pos->second.status = spdr::event::STATUS_REMOVE;
				pos->second.attributeTable.reset();
				updated = true;
			}

			//version progress
			if (pos->second.nodeVersion < ver)
			{
				pos->second.nodeVersion = ver;
				pos->second.status = status;
				updated = true;
			}

			//status change
			if (pos->second.status == spdr::event::STATUS_SUSPECT && (status == spdr::event::STATUS_LEAVE || status == spdr::event::STATUS_REMOVE))
			{
				pos->second.status = status;
				updated = true;
			}

			if (updated)
			{
				pos->second.timeStamp = time;
			}
		}

		if (pos->second.attributeTable)
		{
			retained = true;
		}
	}

	return std::make_pair(updated,retained);
}

bool NodeHistorySet::containsVerGreaterEqual(
		NodeIDImpl_SPtr node, NodeVersion ver) const
{

	NodeHistoryMap::const_iterator pos = historyMap_.find(node);
	if (pos != historyMap_.end())
	{
		//version is higher or equal
		if (pos->second.nodeVersion >= ver)
		{
			return true;
		}
		else
			return false;
	}
	else
	{
		return false;
	}
}

bool NodeHistorySet::forceRemoveRetained(NodeIDImpl_SPtr node, int64_t incarnation)
{
	NodeHistoryMap::iterator pos = historyMap_.find(node);
	if (pos != historyMap_.end())
	{
		if ((pos->second.nodeVersion.getIncarnationNumber() <= incarnation)
				&& (pos->second.status != spdr::event::STATUS_REMOVE)
				&& pos->second.attributeTable)
		{
			pos->second.status = spdr::event::STATUS_REMOVE;
			pos->second.attributeTable.reset();
			return true;
		}
	}

	return false;
}

bool NodeHistorySet::contains(
		NodeIDImpl_SPtr node) const
{
	return (historyMap_.count(node) > 0);
}

NodeIDImpl_SPtr NodeHistorySet::getNextNode()
{
	NodeIDImpl_SPtr next; //null pointer

	if (!validIterator_)
	{
		//place iterator at a random starting point
		iterator_ = historyMap_.begin();
		if (iterator_ != historyMap_.end())
		{
			int r = rand() % historyMap_.size();
			for (int i=0; i<r; ++i)
				++iterator_;
		}
		validIterator_ = true;
	}

	if (historyMap_.size() == 0)
	{
		return next;
	}
	else
	{
		next = iterator_->first;
		++iterator_;
		if (iterator_ == historyMap_.end())
		{
			iterator_ = historyMap_.begin();
		}
	}

	return next;
}

int NodeHistorySet::size() const
{
	return historyMap_.size();
}

void NodeHistorySet::clear()
{
	historyMap_.clear();
}

int NodeHistorySet::prune(boost::posix_time::ptime retention_threshold)
{
	NodeHistoryMap::iterator pos = historyMap_.begin();
	int num = 0;
	while ( pos !=historyMap_.end() )
	{
		if ((pos->second.timeStamp <= retention_threshold) && (!pos->second.attributeTable))
		{
			//the iterator position given to erase is a copy,
			//therefore it does not invalidate the iterator
			historyMap_.erase(pos++);
			num++;
		}
		else
		{
			++pos;
		}
	}

	return num;
}

}
