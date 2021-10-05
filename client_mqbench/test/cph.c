const static char sccsid[] = "%Z% %W% %I% %E% %U%";
/*
 * Copyright (c) 2007-2021 Contributors to the Eclipse Foundation
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
* Change Log:
* -------- ----------- ------------- ------------------------------------
* Reason:  Date:       Originator:   Comments:
* -------- ----------- ------------- ------------------------------------
*          10-04-07    Jerry Stevens File Creation
*/

#include "cphConfig.h"
#include "cphControlThread.h"
#include "cphWorkerThread.h"
#include "cphLog.h"
#include "cphUtil.h"

/*
** Method: main
**
** main program which: 
**
** - sets up the configuration control block
** - loads the property files
** - parses the command line options
** - initialises and starts the control thread
** - waits for the worker threads to finish
** - disposes of the various control blocks
**
** Input Parameters: argc - the number of arguments supplied on the command line
**                   argv - command line arguments as an array of pointers to character strings
**
** Returns: 0 on successful completion, non zero otherwise
**
*/
int main(int argc, char * argv[])
{

    CPH_CONFIG *myConfig;
    CPH_CONTROLTHREAD *pControlThread;
    CPH_TRACE *pTrc=NULL;

    /* Set up a trace structure if trace is compiled in */
    CPHTRACEINI(&pTrc);

    /* Set up the configuration control block */
    cphConfigIni(&myConfig, pTrc);

    /* Read the property files for the "fixed" modules */
    if (CPHTRUE == cphConfigRegisterBaseModules(myConfig)) {

        /* Read the command line arguments */
        cphConfigReadCommandLine(myConfig, argc, argv);

        /* Start trace if it has been requested */
        cphConfigStartTrace(myConfig);

        if (CPHTRUE == cphConfigValidateHelpOptions(myConfig)) {

            if (CPHTRUE == cphWorkerThreadValidate(myConfig)) {

                /* Read the property file for the "variable" modules - this is the module specified by the -tc parameter
                NB - We have to load this after cphWorkerThreadValidate has set up pConfig->clazzName from the -tc option */
                if (CPHTRUE == cphConfigRegisterTCModule(myConfig)) {

                    /* Initiliase the control thread structure */
                    cphControlThreadIni(&pControlThread, myConfig);
                    if (NULL != pControlThread) {

                        /* Remember the time the contol thread started for stats purposes */
                        cphControlThreadSetThreadStartTime(pControlThread);

                        /* Run the control thread - this actually runs in the current thread not another */
                        cphControlThreadRun(pControlThread);

                        /* Wait until all of the worker threads are complete */
                        while (cphControlThreadGetRunningWorkers(pControlThread) > 0) cphUtilSleep(1000);

                        /* Release the control thread structure */
                        cphControlThreadFree(&pControlThread);
                    }
                }
            }
        }
    }
    /* Relase the configuration control block */
    cphConfigFree(&myConfig);

    /* Dispose of the trace structure if trace was compiled in */
    CPHTRACEFREE(&pTrc);

    return 0;
}

