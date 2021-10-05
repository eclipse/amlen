/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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
/*
 * RemoteServerRecord.cpp
 *  Created on: Feb 29, 2016
 */

#include <RemoteServerRecord.h>
#include <sstream>

namespace mcp
{

RemoteServerRecord::RemoteServerRecord(
        const std::string& uid,
        const std::string& name,
        int64_t incarnation) :
                serverUID(uid),
                serverName(name),
                incarnationNumber(incarnation)
{
}

RemoteServerRecord::~RemoteServerRecord()
{
}

bool RemoteServerRecord::operator<(const RemoteServerRecord& other) const
{
    return serverUID < other.serverUID;
}

bool RemoteServerRecord::operator>(const RemoteServerRecord& other) const
{
    return serverUID > other.serverUID;
}

bool RemoteServerRecord::operator==(const RemoteServerRecord& other) const
{
    return serverUID == other.serverUID;
}

bool RemoteServerRecord::operator!=(const RemoteServerRecord& other) const
{
    return serverUID != other.serverUID;
}

std::string RemoteServerRecord::toString() const
{
    std::ostringstream s;
    s << "UID=" << serverUID << " Name=" << serverName << " Inc=" << incarnationNumber;
    return s.str();
}

} /* namespace mcp */
