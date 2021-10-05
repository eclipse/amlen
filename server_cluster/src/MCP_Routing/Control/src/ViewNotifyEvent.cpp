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


#include <ViewNotifyEvent.h>
#include <ostream>

namespace mcp
{

ViewNotifyEvent::ViewNotifyEvent() :
        type_(NONE), phServerHandle_(NULL), viewKeeper_()
{
}

ViewNotifyEvent::ViewNotifyEvent(Type type,
        const ismCluster_RemoteServerHandle_t phServerHandle,
        boost::shared_ptr<ViewKeeper> viewKeeper) :
        type_(type), phServerHandle_(phServerHandle), viewKeeper_(viewKeeper)
{
}

ViewNotifyEvent::~ViewNotifyEvent()
{
}

ViewNotifyEvent::Type ViewNotifyEvent::getType() const
{
    return type_;
}

int ViewNotifyEvent::deliver()
{
    int rc = ISMRC_OK;

    switch (type_)
    {
    case INCOMING_PROTOCOL_RS_CONNECTED:
        if (viewKeeper_)
        {
            rc = viewKeeper_->nodeForwardingConnected(phServerHandle_);
        }
        else
        {
            rc = ISMRC_NullPointer;
        }

        break;

    case INCOMING_PROTOCOL_RS_DISCONNECTED:
        if (viewKeeper_)
        {
            rc = viewKeeper_->nodeForwardingDisconnected(phServerHandle_);
        }
        else
        {
            rc = ISMRC_NullPointer;
        }

        break;

    default:
        rc = ISMRC_Error;
    }

    return rc;
}

const ismCluster_RemoteServerHandle_t ViewNotifyEvent::getCluster_RemoteServerHandle() const
{
    return phServerHandle_;
}

boost::shared_ptr<ViewNotifyEvent> ViewNotifyEvent::createInProtoConnected(
        const ismCluster_RemoteServerHandle_t phServerHandle,
        boost::shared_ptr<ViewKeeper> viewKeeper)
{
    return ViewNotifyEvent_SPtr(
            new ViewNotifyEvent(INCOMING_PROTOCOL_RS_CONNECTED,
                    phServerHandle, viewKeeper));
}

boost::shared_ptr<ViewNotifyEvent> ViewNotifyEvent::createInProtoDisconnected(
        const ismCluster_RemoteServerHandle_t phServerHandle,
        boost::shared_ptr<ViewKeeper> viewKeeper)
{
    return ViewNotifyEvent_SPtr(
            new ViewNotifyEvent(INCOMING_PROTOCOL_RS_DISCONNECTED,
                    phServerHandle, viewKeeper));
}

std::string ViewNotifyEvent::toString() const
{
    std::ostringstream oss;
    oss << "ViewNotifyEvent: type=" << type_;
    return oss.str();
}

} /* namespace mcp */
