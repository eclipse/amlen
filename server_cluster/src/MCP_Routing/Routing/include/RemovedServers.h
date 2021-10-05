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

#ifndef MCP_REMOVEDSERVERS_H_
#define MCP_REMOVEDSERVERS_H_

#include <string>
#include <map>
#include <set>
#include <boost/cstdint.hpp>
#include <ByteBuffer.h>
#include <Definitions.h>
#include <RemoteServerRecord.h>

namespace mcp
{

class RemovedServers
{
private:
//    typedef std::map<std::string,int64_t> RemovedServersMap;
//    RemovedServersMap map_;

    typedef std::set<RemoteServerRecord_SPtr, spdr::SPtr_Less<RemoteServerRecord> > RemovedServersSet;
    RemovedServersSet set_;

public:
    RemovedServers();
    virtual ~RemovedServers();

    bool add(const std::string& uid, int64_t incNum);

    bool add(RemoteServerRecord_SPtr server);

    void write(const uint32_t wireFormatVer, ByteBuffer_SPtr buffer) const;

    bool readAdd(const uint32_t wireFormatVer, ByteBufferReadOnlyWrapper& buffer);

    bool readMerge(const uint32_t wireFormatVer, ByteBufferReadOnlyWrapper& buffer, RemoteServerVector& newServers);

    void exportTo(RemoteServerVector& serverVector) const;

    bool contains(const std::string& uid);

    void clear();

    std::size_t size() const;

    std::string toString() const;

};

} /* namespace mcp */

#endif /* MCP_REMOVEDSERVERS_H_ */
