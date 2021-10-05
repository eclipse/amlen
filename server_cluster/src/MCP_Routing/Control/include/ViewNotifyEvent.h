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

#ifndef MCP_VIEWNOTIFYEVENT_H_
#define MCP_VIEWNOTIFYEVENT_H_

#include <boost/shared_ptr.hpp>
#include "cluster.h"
#include "ViewKeeper.h"

namespace mcp
{

class ViewNotifyEvent;
typedef boost::shared_ptr<ViewNotifyEvent> ViewNotifyEvent_SPtr;

class ViewNotifyEvent
{
public:
    enum Type
    {
        NONE                              = 0,
        INCOMING_PROTOCOL_RS_CONNECTED    = 1,
        INCOMING_PROTOCOL_RS_DISCONNECTED = 2
    };

    static ViewNotifyEvent_SPtr createInProtoConnected(
            const ismCluster_RemoteServerHandle_t phServerHandle,
            boost::shared_ptr<ViewKeeper> viewKeeper);
    static ViewNotifyEvent_SPtr createInProtoDisconnected(const ismCluster_RemoteServerHandle_t phServerHandle,
            boost::shared_ptr<ViewKeeper> viewKeeper);

    virtual ~ViewNotifyEvent();

    Type getType() const;
    const ismCluster_RemoteServerHandle_t getCluster_RemoteServerHandle() const;

    int deliver();

    std::string toString() const;

private:
    ViewNotifyEvent();
    explicit ViewNotifyEvent(Type type, const ismCluster_RemoteServerHandle_t phServerHandle, boost::shared_ptr<ViewKeeper> viewKeeper);

    Type type_;
    const ismCluster_RemoteServerHandle_t phServerHandle_;
    boost::shared_ptr<ViewKeeper> viewKeeper_;
};

} /* namespace mcp */

#endif /* MCP_VIEWNOTIFYEVENT_H_ */
