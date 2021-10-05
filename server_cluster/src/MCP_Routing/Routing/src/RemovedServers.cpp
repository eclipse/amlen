/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
 * 
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 * 
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0
 * 
 * SPDX-License-Identifier: EPL-2.0
 */

#include <sstream>
#include <RemovedServers.h>

namespace mcp
{

RemovedServers::RemovedServers()
{
}

RemovedServers::~RemovedServers()
{
}

bool RemovedServers::add(const std::string& uid, int64_t incNum)
{
    bool changed = true;

    RemoteServerRecord_SPtr server(new RemoteServerRecord(uid,"",incNum));
    std::pair<RemovedServersSet::iterator,bool> res = set_.insert(server);
    if (!res.second)
    {
        if (incNum > (*res.first)->incarnationNumber)
        {
            set_.erase(res.first);
            set_.insert(server);
        }
        else
        {
            changed = false;
        }
    }

    return changed;
}

bool RemovedServers::add(RemoteServerRecord_SPtr server)
{
    bool changed = true;

    std::pair<RemovedServersSet::iterator,bool> res = set_.insert(server);
    if (!res.second)
    {
        if (server->incarnationNumber > (*res.first)->incarnationNumber)
        {
            set_.erase(res.first);
            set_.insert(server);
        }
        else
        {
            changed = false;
        }
    }

    return changed;
}

void RemovedServers::write(const uint32_t wireFormatVer, ByteBuffer_SPtr buffer) const
{
    buffer->writeInt(static_cast<int32_t>(set_.size()));
    for (RemovedServersSet::const_iterator it = set_.begin(); it != set_.end(); ++it)
    {
        buffer->writeString((*it)->serverUID);
        buffer->writeLong((*it)->incarnationNumber);
    }
}

bool RemovedServers::readAdd(const uint32_t wireFormatVer, ByteBufferReadOnlyWrapper& buffer)
{
    bool changed = false;
    int32_t num_records = buffer.readInt();
    while (num_records-- > 0)
    {
        std::string uid = buffer.readString();
        int64_t inc = buffer.readLong();
        bool changed1 = this->add(uid,inc);
        changed |= changed1;
    }
    return changed;
}

bool RemovedServers::readMerge(const uint32_t wireFormatVer, ByteBufferReadOnlyWrapper& buffer, RemoteServerVector& newServers)
{
    bool changed = false;
    int32_t num_records = buffer.readInt();
    while (num_records-- > 0)
    {
        std::string uid = buffer.readString();
        int64_t inc = buffer.readLong();
        RemoteServerRecord_SPtr server(new RemoteServerRecord(uid,"",inc));
        if (set_.count(server) == 0)
        {
            newServers.push_back(server);
        }

        bool changed1 = this->add(server);
        changed |= changed1;
    }

    return changed;
}

void RemovedServers::exportTo(RemoteServerVector& serverVector) const
{
    for (RemovedServersSet::const_iterator it = set_.begin(); it != set_.end(); ++it)
    {
        serverVector.push_back(*it);
    }
}

bool RemovedServers::contains(const std::string& uid)
{
    RemoteServerRecord_SPtr server(new RemoteServerRecord(uid,"",0));
    return (set_.count(server)>0);
}

void RemovedServers::clear()
{
    set_.clear();
}

std::size_t RemovedServers::size() const
{
    return set_.size();
}

std::string RemovedServers::toString() const
{
    std::ostringstream s;
    s << "RemovedServers(#" << set_.size() << ")={";
    for (RemovedServersSet::const_iterator it = set_.begin(); it != set_.end();)
    {
        s << "(" << (*it)->serverUID << " " << (*it)->incarnationNumber << ")";
        if (++it != set_.end())
            s << ",";
    }
    s<< "}";
    return s.str();
}
} /* namespace mcp */
