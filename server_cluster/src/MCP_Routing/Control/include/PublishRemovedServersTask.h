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

#ifndef PUBLISHREMOVEDSERVERSTASK_H_
#define PUBLISHREMOVEDSERVERSTASK_H_

#include <AbstractTask.h>
#include "ControlManager.h"

namespace mcp
{

class PublishRemovedServersTask : public mcp::AbstractTask
{
public:
    PublishRemovedServersTask(ControlManager& controlManager);
    virtual ~PublishRemovedServersTask();

    void run()
    {
        controlManager_.executePublishRemovedServersTask();
    }

private:
    ControlManager& controlManager_;
};

} /* namespace mcp */

#endif /* PUBLISHREMOVEDSERVERSTASK_H_ */
