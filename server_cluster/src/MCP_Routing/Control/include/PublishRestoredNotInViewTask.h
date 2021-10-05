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

#ifndef PUBLISHRESTOREDNOTINVIEWTASK_H_
#define PUBLISHRESTOREDNOTINVIEWTASK_H_

#include <AbstractTask.h>
#include "ControlManager.h"

namespace mcp
{

class PublishRestoredNotInViewTask : public mcp::AbstractTask
{
public:
    PublishRestoredNotInViewTask(ControlManager& controlManager);
    virtual ~PublishRestoredNotInViewTask();

    void run()
    {
        controlManager_.executePublishRestoredNotInViewTask();
    }

private:
    ControlManager& controlManager_;
};

} /* namespace mcp */

#endif /* PUBLISHRESTOREDNOTINVIEWTASK_H_ */
