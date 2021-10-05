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

#ifndef REQUESTADMINMAINTENANCEMODETASK_H_
#define REQUESTADMINMAINTENANCEMODETASK_H_

#include <AbstractTask.h>
#include "admin.h"
#include "ControlManager.h"

namespace mcp
{

class RequestAdminMaintenanceModeTask : public mcp::AbstractTask
{
public:
    RequestAdminMaintenanceModeTask(ControlManager& controlManager, int errorRC, int restartFlag);
    virtual ~RequestAdminMaintenanceModeTask();

    void run()
    {
        controlManager_.executeRequestAdminMaintenanceModeTask(errorRC_, restartFlag_);
    }

private:
    ControlManager& controlManager_;
    int errorRC_;
    int restartFlag_;
};

} /* namespace mcp */

#endif /* REQUESTADMINMAINTENANCEMODETASK_H_ */
