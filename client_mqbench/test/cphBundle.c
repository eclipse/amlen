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

/* This module provides a set of functions that are used to read in the contents of a
   property file and store them in a CPH_BUNDLE structure. This contains the contents of the 
   property file as a linked list of name/value pairs
*/

#include <string.h>

#include "cphUtil.h"
#include "cphBundle.h"

/*
** Method: cphBundleLoad
**
** This function reads a property file and stores the contents in a CPH_BUNDLE structure. This contains
** each line of the property file as a name/value list.
**
** Input Parameters: pConfig - a pointer to the configuration control block
**                   moduleName - the name of the module for which we wish to read the associated property file
**
** Returns: a pointer to the CPH_BUNDLE on successful exection and NULL otherwise
**
*/
void *cphBundleLoad(CPH_CONFIG *pConfig, char *moduleName) {
    FILE *fp;
    int loadedOK = CPHTRUE;
    char propFileName[512];
    size_t dirLen;
    char aLine[2048];
    char name[80];
    char value[1024];
    char *token;
    char errorString[1024];

    CPH_BUNDLE *pBundle = NULL;

    #define CPH_ROUTINE "cphConfigGetBundle"
    CPHTRACEENTRY(pConfig->pTrc, CPH_ROUTINE );

    /* Initiliase a new CPH_BUNDLE structure into which we read all the lines in the property file
       as name/value pairs */
    cphBundleIni(&pBundle, pConfig, moduleName);

    /* If we have a CPH_INSTALLDIR env var prefix prepend that to the property file name */
    dirLen = strlen(pConfig->cphInstallDir);
    if (0 < dirLen) {
        strcpy(propFileName, pConfig->cphInstallDir);
        if (pConfig->cphInstallDir[ dirLen - 1 ] != CPH_DIRSEP) {
            propFileName[dirLen] = CPH_DIRSEP;
            propFileName[dirLen+1] = '\0';
        }
        strcat(propFileName, moduleName);
    }
    else
        strcpy(propFileName, moduleName);

    strcat(propFileName, ".properties");

    fp = fopen(propFileName, "rt");
    if (NULL != fp) {
        while ( NULL != fgets(aLine, 1023, fp)) {

            /* If the line was just white space or empty ignore it */
            if (0 == strlen(aLine)) continue;

            /* If it was a comment ignore it */
            if ('#' == aLine[0]) continue;

            /* Remove any CRLF */
            if (aLine[strlen(aLine)-1] == '\n') aLine[strlen(aLine)-1] = '\0';

            /* If the line is now just white space or empty ignore it */
            if (0 == strlen(aLine)) continue;


            /* Take off any terminating and leading white space */
            cphUtilTrim(aLine);

            /* Append any continuation lines   */
            while (aLine[strlen(aLine) - 1] == '\\') {
                if ( NULL != fgets(aLine + strlen(aLine) -1, 255, fp) )
                {
                    /* Remove any CRLF */
                    if (aLine[strlen(aLine)-1] == '\n') aLine[strlen(aLine)-1] = '\0';
                }
            }

            /* Parse out the line into its name and value components */
            strcpy(name, strtok(aLine, "="));

            /* The value may or may not be present */
            token = strtok(NULL, "=");
            if (NULL != token) 
                strcpy(value, token);
            else
                *value = '\0';

            /* Don't be sensitive to white space around the "=" character */
            cphUtilRTrim(name);
            cphUtilLTrim(value);

            /* Store the name value pair into the given CPH_NAMEVAL structure */
            if (CPHTRUE != cphNameValAdd(&(pBundle->pNameValList), name, value)) {
                cphLogPrintLn( pConfig->pLog, LOGERROR, "ERROR storing name/value pair." );
                loadedOK = CPHFALSE;
                break;
            } 
        }
        fclose(fp);
    }
    else {
        sprintf(errorString, "ERROR couldn't open properties file: %s.", propFileName);
        cphLogPrintLn( pConfig->pLog, LOGERROR, errorString );
        loadedOK = CPHFALSE;
    }

    /* Add the pointer to this bundle into the array of resource bundle pointers */ 
    cphArrayListAdd(pConfig->bundles, pBundle);

    CPHTRACEEXIT(pConfig->pTrc, CPH_ROUTINE );
    #undef CPH_ROUTINE

    if (CPHTRUE == loadedOK) return(pBundle);
    return(NULL);
}


/*
** Method: cphBundleIni
**
** Initialise a CPH_BUNDLE
**
** Input Parameters: pConfig - pointer to the configuration control block
**                   moduleName - the name of the module which this CPH_BUNDLE relates to
**
** Output Parameters: ppBundle - double pointer to the CPH_BUNDLE to be allocated
**
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
void cphBundleIni(CPH_BUNDLE **ppBundle, CPH_CONFIG *pConfig, char *moduleName) {
   CPH_BUNDLE *pBundle;

   #define CPH_ROUTINE "cphBundleIni"
   CPHTRACEENTRY(pConfig->pTrc, CPH_ROUTINE );

   pBundle = malloc(sizeof(CPH_BUNDLE));
   if (NULL != pBundle) {
       strcpy(pBundle->clazzName, moduleName);
       pBundle->pConfig = pConfig;
       pBundle->pNameValList = NULL;
   }

   *ppBundle = pBundle;
    
   CPHTRACEEXIT(pConfig->pTrc, CPH_ROUTINE);
    #undef CPH_ROUTINE
}

/*
** Method: cphBundleFree
**
** This function disposes of a CPH_BUNDLE
**
** Input/Output Parameters: ppBundle - double pointer to the CPH_BUNDLE. The pointer
**                          is set to NULL when it is successfully disposed of.
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int  cphBundleFree(CPH_BUNDLE **ppBundle) {

    CPH_TRACE *pTrc;
    int status = CPHFALSE;;

    pTrc = (*ppBundle)->pConfig->pTrc;
    #define CPH_ROUTINE "cphBundleFree"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );

	if (NULL != *ppBundle) {

        /* Free the CPH_NAMEVAL list of name/value pairs that were read in from the property file */
        cphNameValFree(&(*ppBundle)->pNameValList);

		free(*ppBundle);
		*ppBundle = NULL;
		status = CPHTRUE;
	}

    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
    #undef CPH_ROUTINE

    return(status);
}


