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
 * RemoteServerRecord.h
 *  Created on: Feb 29, 2016
 */

#ifndef REMOTESERVERRECORD_H_
#define REMOTESERVERRECORD_H_

#include <string>
#include <vector>
#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>

namespace mcp
{

class RemoteServerRecord
{
public:
    const std::string serverUID;
    const std::string serverName;
    const int64_t incarnationNumber;

    RemoteServerRecord(
            const std::string& uid,
            const std::string& name,
            int64_t incarnation);
    virtual ~RemoteServerRecord();

    /*
     * Relational operators <,>,==,!= use only the serverUID
     */

    bool operator<(const RemoteServerRecord& other) const;
    bool operator>(const RemoteServerRecord& other) const;
    bool operator==(const RemoteServerRecord& other) const;
    bool operator!=(const RemoteServerRecord& other) const;

    std::string toString() const;
};

typedef boost::shared_ptr<RemoteServerRecord> RemoteServerRecord_SPtr;
typedef std::vector<RemoteServerRecord_SPtr> RemoteServerVector;

} /* namespace mcp */

#endif /* REMOTESERVERRECORD_H_ */
