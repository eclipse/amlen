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
#include "cphUtil.h"
#include "cphLog.h"
#include "cphStringBuffer.h"
#include "cphArrayList.h"
#include "cphListIterator.h"
#include "cphBundle.h"

#include <string.h>
#include <stdio.h>

/*
** Method: cphConfigIni
**
** This method creates and initialises a new trace control block. This is used to store the 
** default values for all options, the values for options specified on the command line, and 
** various other configuration parameters
**
** Input Parameters: pTrc - a pointer to the cph trace control block
**
** Output parameters: ppConfig - a new double pointer to the newly created control block
**
*/
void cphConfigIni(CPH_CONFIG **ppConfig, CPH_TRACE *pTrc) {
    CPH_CONFIG *pConfig;

    #define CPH_ROUTINE "cphConfigIni"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );

    pConfig = malloc(sizeof(CPH_CONFIG));
    if (NULL != pConfig) {
        pConfig->pTrc = pTrc;
 	    cphArrayListIni(&pConfig->bundles);
        pConfig->defaultOptions = NULL;
        pConfig->options = NULL;
        pConfig->inValid = CPHFALSE;
        cphLogIni(&pConfig->pLog, pTrc);
        pConfig->cphInstallDir[0] = '\0';

        /* Get the value of the CPH_INSTALLDIR environment variable */
        cphGetEnv(CPH_INSTALLDIR, pConfig->cphInstallDir, sizeof(pConfig->cphInstallDir));
        if (0 < strlen(pConfig->cphInstallDir)) {
            CPHTRACEMSG(pTrc, CPH_ROUTINE, "CPH_INSTALLDIR is: %s", pConfig->cphInstallDir); 
        } else {
            CPHTRACEMSG(pTrc, CPH_ROUTINE, "CPH_INSTALLDIR was not set"); 
        }
    }

    *ppConfig = pConfig;
    CPHTRACEEXIT(pConfig->pTrc, CPH_ROUTINE );
#undef CPH_ROUTINE

}

/*
** Method: cphConfigFree
**
** Dispose of the configuration control block
**
** Input/Output Parameters: ppConfig - a double pointer to the configuration control block to be freed
**                                     This is set to NULL on successful deletion
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
void cphConfigFree(CPH_CONFIG **ppConfig) {
#define CPH_ROUTINE "cphConfigFree"
    CPHTRACEENTRY((*ppConfig)->pTrc, CPH_ROUTINE );

    if (NULL != *ppConfig) {
        if (NULL != (*ppConfig)->pLog) cphLogFree((*ppConfig)->pLog);
        if (NULL != (*ppConfig)->defaultOptions) cphNameValFree(&(*ppConfig)->defaultOptions);
        if (NULL != (*ppConfig)->options) cphNameValFree(&(*ppConfig)->options);
        if (NULL != (*ppConfig)->bundles) {
            cphConfigFreeBundles(*ppConfig);
            cphArrayListFree(&(*ppConfig)->bundles);
        }
    }

    CPHTRACEEXIT((*ppConfig)->pTrc, CPH_ROUTINE );
    if (NULL != *ppConfig) {
        free(*ppConfig);
        *ppConfig = NULL;
    }
#undef CPH_ROUTINE

}

/*
** Method: cphConfigReadCommandLine
**
** This function builds a linked list of the specified command line options
**
** Input Parameters: pConfig - a pointer to the configuration control block
**                   argc    - the number of arguments on the command line (inc the prog name)
**                   argv    - pointer to an array of character strings representing the arguments
**
*/
void cphConfigReadCommandLine(CPH_CONFIG * pConfig, int argc, char *argv[]) {
    int i = 0;
    char key[80] = { '\0' };
    char value[80] = CPHCONFIG_UNSPECIFIED;
    char cmdLine[2048];

#define CPH_ROUTINE "cphConfigReadCommandLine"
    CPHTRACEENTRY(pConfig->pTrc, CPH_ROUTINE );

    strcpy(cmdLine, "Command line: ");
    strcat(cmdLine, argv[0]);
    strcat(cmdLine, " ");
    for (i=1; i < argc; i++) {
        strcat(cmdLine, argv[i]);
        strcat(cmdLine, " ");
        if (argv[i][0] == '-') {
            if ('\0' != key[0]) {
                cphNameValAdd(&pConfig->options, key, value);
                *key = 0;
            }
            strcpy(key, &argv[i][1]);
            strcpy(value, CPHCONFIG_UNSPECIFIED);
        } else {
            if (0 == strcmp(value, CPHCONFIG_UNSPECIFIED)) {
                strcpy(value, argv[i]);
            } else {
                /*this else allows us to have spaces in the key values i.e.
                the values will span
                multiple commandline arguements. */
                strcat(value, " ");
                strcat(value, argv[i]);
            }
        }
    }

    if (0 != *key) {
        cphNameValAdd(&pConfig->options, key, value);
    }

	/* configState = LOADING; */
			
    cphLogPrintLn(pConfig->pLog, LOGINFO, cmdLine );


    /* cphConfigListNameVal(pConfig->options); */
    CPHTRACEEXIT(pConfig->pTrc, CPH_ROUTINE );
#undef CPH_ROUTINE
}

/*
** Method: cphConfigNextNameVal
**
** Given a pointer to a CPH_NAMEVAL entry, set that pointer to the next entry in the list 
**
** Input/output Parameters: ppList - double pointer to a CPH_NAMEVAL entry. This is set to the
**                                   the next value in the list if there is one
**
** Returns: CPHTRUE on successful exection, CPHFALSE if there is no next value or on other error 
**
*/
int cphConfigNextNameVal(CPH_NAMEVAL **ppList) {

    if (NULL == *ppList) return CPHFALSE;
    if (NULL == (*ppList)->next) return CPHFALSE;

    *ppList = (*ppList)->next;

    return CPHTRUE;
}

/*
** Method: cphConfigContainsKey
**
** Scans down the CPH_NAMVAL linked list of configuration options for the presence of a given
** name.
**
** Input Parameters: pConfig - a pointer to the configuration control block
**                   name - the name to search for (character string)
**
** Returns: CPHTRUE on successful exection, CPHFALSE if the name was not found in the list or on other error 
**
*/
int cphConfigContainsKey(CPH_CONFIG *pConfig, char *name) {
    int status = CPHFALSE;
    CPH_NAMEVAL *pList;
    CPH_NAMEVAL *nv;

    #define CPH_ROUTINE "cphConfigContainsKey"
    CPHTRACEENTRY(pConfig->pTrc, CPH_ROUTINE );

    pList = pConfig->options;

    if (pList != NULL) {
        nv = pList;
        while (1)
        {
            if (0 == strcmp(nv->name, name)) {
                status = CPHTRUE;
                break;
            }
            if (NULL == nv->next) break;
            nv = nv->next;
        }
    }

    CPHTRACEEXIT(pConfig->pTrc, CPH_ROUTINE);
    #undef CPH_ROUTINE
    return status;
}

/*
** Method: cphConfigGetString
**
** Look up a given configuration option and return the result as a string. The list of command line
** options are scanned first and if the option was not found there, the list of default options is
** scanned.
**
** Input Parameters: pConfig - a pointer to the configuration control block
**                   in - the option name to search for (character string)
**
** Output parameters: res - if found in the list, the value of the configuration option
**
** Returns: CPHTRUE on successful exection, CPHFALSE if the name was not found in the options
**          or on other error
**
*/
int cphConfigGetString(CPH_CONFIG *pConfig, char *res, char *in) {

    char errorString[512]= {'\0'};
    int status=CPHFALSE;

    #define CPH_ROUTINE "cphConfigGetString"
    CPHTRACEENTRY(pConfig->pTrc, CPH_ROUTINE );

    if (CPHTRUE == cphNameValGet(pConfig->options, in, res))
        status = CPHTRUE; 
    else {
        if (CPHTRUE == cphNameValGet(pConfig->defaultOptions, in, res)) 
           status = CPHTRUE;
    }

    if (CPHTRUE == status) {

       if (0 == strcmp(res, "UNSPECIFIED")) { 
           strcpy(res, "");
       }
    } else {
       strcpy(errorString, "Unknown argument [");
       strcat(errorString, in);
       strcat(errorString, "]");
       cphConfigInvalidParameter(pConfig, errorString);
    }

    CPHTRACEEXIT(pConfig->pTrc, CPH_ROUTINE);
    #undef CPH_ROUTINE

    return(status);
}

/*
** Method: cphConfigGetBoolean
**
** Look up a given boolean configuration option and returns the result as an integer CPHTRUE or CPHFALSE. 
** The function calls cphConfigGetString to first extract the value as a string "true" or "false" and then 
** if found in the list converts it to the integer representation.
**
** Input Parameters: pConfig - a pointer to the configuration control block
**                   in - the option name to search for (character string)
**
** Output parameters: res - if found in the list, the integer value of the configuration option. If the 
**                          the value does not parse as "true" or "false" the contents of res are not altered.
**
** Returns: CPHTRUE on successful exection, CPHFALSE if the name was not found in the options
**          or on other error
**
*/
int  cphConfigGetBoolean(CPH_CONFIG *pConfig, int *res, char *in) {
    char sVal[80] = {'\0'};
    int status = CPHTRUE;

#define CPH_ROUTINE "cphConfigGetBoolean"
    CPHTRACEENTRY(pConfig->pTrc, CPH_ROUTINE );

    if (CPHTRUE == cphConfigGetString(pConfig, sVal, in)) {
        /* poor checking but allows UNSPECIFIED ("") to return true */
        if (0 == strcmp(sVal, "false"))
            *res = CPHFALSE;
        else 
            *res = CPHTRUE;
    }
    else
        status = CPHFALSE;

    CPHTRACEEXIT(pConfig->pTrc, CPH_ROUTINE );
#undef CPH_ROUTINE
    return(status);
}

/*
** Method: cphConfigStringToBoolean
**
** Convert a "true" or "false" string representation of a boolean value to integer CPHTRUE or CPHFALSE.
**
** Input Parameters: aString - the string to convert 
**
** Output Parameters: res - a pointer to an integer in which the result is set
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphConfigStringToBoolean(int *res, char *aString) {
    int status = CPHTRUE;
    if (0 == strcmp(aString, "false")) *res = CPHFALSE;
    else if (0 == strcmp(aString, "true"))  *res = CPHTRUE;
    else status = CPHFALSE;

    return(status);
}

/*
** Method: cphConfigGetInt
**
** Look up a given configuration option and return the result as an integer. The function calls cphConfigGetString
** to first extract the value as a string and then if found converts it to an int. 
**
** Input Parameters: pConfig - a pointer to the configuration control block
**                   name - the option name to search for (character string)
**
** Output parameters: res - if found in the list, the integer value of the configuration option
**
** Returns: CPHTRUE on successful exection, CPHFALSE if the name was not found in the options
**          or on other error
**
*/
int cphConfigGetInt(CPH_CONFIG *pConfig, int *res, char *name)
{

    char errorString[512] = {'\0'};
    char sVal[80] = {'\0'};
    int status=CPHFALSE;

#define CPH_ROUTINE "cphConfigGetInt"
    CPHTRACEENTRY(pConfig->pTrc, CPH_ROUTINE );

    if ( (CPHTRUE == cphConfigGetString(pConfig, sVal, name)) && 
        (1 == sscanf(sVal, "%d", res)) ) 
        status = CPHTRUE;
    else {
        strcpy(errorString, "Error parsing [");
        strcat(errorString, name);
        strcat(errorString, "=");
        strcat(errorString, sVal);
        strcat(errorString, "] ensure the value is a valid int value");
        cphConfigInvalidParameter(pConfig, errorString);
        }

    CPHTRACEEXIT(pConfig->pTrc, CPH_ROUTINE );
#undef CPH_ROUTINE

    return(status);
}


/*
** Method: cphConfigInvalidParameter
**
** This method is called when an invalid option or option setting is specified. It prints
** an error message through the logging mechanism and sets the "invalid" member of the
** configuration control block. This value is checked and if set, cph will abort before
** the worker threads start executing.
**
** Input Parameters: pConfig - a pointer to the configuration control block
**                   error - the character string to be be written to the log
**
*/
void cphConfigInvalidParameter(CPH_CONFIG *pConfig, char* error) {
    char errorString[512]={'\0'};

#define CPH_ROUTINE "cphConfigInvalidParameter"
    CPHTRACEENTRY(pConfig->pTrc, CPH_ROUTINE );

    strcpy(errorString, "Invalid parameter: ");
    strcat(errorString, error);
    cphLogPrintLn(pConfig->pLog, LOGERROR, errorString );

    pConfig->inValid = CPHTRUE;

    CPHTRACEEXIT(pConfig->pTrc, CPH_ROUTINE );
#undef CPH_ROUTINE
}

/*
** Method: cphConfigIsInvalid
**
** Return true or false depending on whether the configuration options have been
** marked as invalid
**
** Input Parameters: pConfig - a pointer to the configuration control block
**
** Returns: CPHTRUE if the options are valid
**          CPHFALSE if the options are not valid
**
*/
int cphConfigIsInvalid(CPH_CONFIG *pConfig) {

    #define CPH_ROUTINE "cphConfigIsInvalid"
    CPHTRACEENTRY(pConfig->pTrc, CPH_ROUTINE );

    CPHTRACEEXIT(pConfig->pTrc, CPH_ROUTINE );
    #undef CPH_ROUTINE

    return pConfig->inValid;
}

/*
** Method: cphConfigMarkLoaded
**
** This routine checks that the set of command line options is not empty, scans for
** any unrecognised command line options and displays help text if any of the -h or -v
** options were used
**
** Input Parameters: pConfig - a pointer to the configuration control block
**
*/
void cphConfigMarkLoaded(CPH_CONFIG *pConfig) {
    CPH_NAMEVAL *setOptions;
    char aValue[80];
    char errorString[512];

    #define CPH_ROUTINE "cphConfigMarkLoaded"
    CPHTRACEENTRY(pConfig->pTrc, CPH_ROUTINE );

    /* 		configState = LOADED; */

    if ( CPHTRUE == cphConfigIsEmpty(pConfig) ) {
        cphConfigInvalidParameter(pConfig, "No arguments specified!" );
    } else {

        setOptions = pConfig->options;
        do  {
            /* Look up the next option given on the command line in the list of default options */
            if (CPHFALSE == cphNameValGet(pConfig->defaultOptions, setOptions->name, aValue)) {
                strcpy(errorString, "Parameter [");
                strcat(errorString, setOptions->name);
                strcat(errorString, "] is not known/valid in this configuration" ); 
                cphConfigInvalidParameter(pConfig, errorString );
            }
        } while (CPHTRUE == cphConfigNextNameVal(&setOptions));

    } /* end if args set */


    /* Display help if any of the -h options or the -v option were used */
    cphDisplayHelpIfNeeded(pConfig);
    CPHTRACEEXIT(pConfig->pTrc, CPH_ROUTINE );
#undef CPH_ROUTINE
}

/*
** Method: cphDisplayHelpIfNeeded
**
** Look at the command line help options "v", "h", "hf" and "hm" and gives the appropriate help 
** as requested. If help was requested, this function then terminates the program after displaying it.
**
** Input Parameters: pConfig - a pointer to the configuration control block
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphDisplayHelpIfNeeded(CPH_CONFIG *pConfig) {

        int status = CPHTRUE;
        int i = 0;
        int isBool = CPHFALSE;
        int doExit = CPHFALSE;
		char module[80] = {'\0'};
        char versionString[80] = {'\0'};

        #define CPH_ROUTINE "cphDisplayHelpIfNeeded"
        CPHTRACEENTRY(pConfig->pTrc, CPH_ROUTINE );

        if (CPHTRUE != cphConfigGetBoolean(pConfig, &isBool, "h")) return(CPHFALSE);
        if (CPHTRUE == isBool) i = 1;

        if (CPHTRUE != cphConfigGetBoolean(pConfig, &isBool, "v")) return(CPHFALSE);
        if (CPHTRUE == isBool) i = 2;

        if (CPHTRUE != cphConfigGetBoolean(pConfig, &isBool, "hf")) return(CPHFALSE);
        if (CPHTRUE == isBool) i = 3;
        else {
            if (CPHTRUE != cphConfigGetString(pConfig, module, "hm")) return(CPHFALSE);
            if ( (0 < strlen(module)) && (0 != strcmp(module, "none")) ) i =5;
        }

		switch (i) {
			case 1 : /* -h */

                cphLogPrintLn(pConfig->pLog, LOGINFO, cphConfigGetVersion(pConfig, versionString)); 
				cphLogPrintLn(pConfig->pLog, LOGINFO, "(use -hf to see more help options)" );
				cphLogPrintLn(pConfig->pLog, LOGINFO, cphConfigDescribe(pConfig, CPHFALSE) );
				break;
			case 2 : /* -v */
                cphLogPrintLn(pConfig->pLog, LOGINFO, cphConfigGetVersion(pConfig, versionString));
				break;
			case 3 : /* -hf */
                cphLogPrintLn(pConfig->pLog, LOGINFO, cphConfigGetVersion(pConfig, versionString));
				cphLogPrintLn(pConfig->pLog, LOGINFO, cphConfigDescribe(pConfig, CPHTRUE) );
				break;
			case 5 : /* -hm */
                cphLogPrintLn(pConfig->pLog, LOGINFO, cphConfigGetVersion(pConfig, versionString));
				cphLogPrintLn(pConfig->pLog, LOGINFO, cphConfigDescribeModules(pConfig, module, CPHTRUE) );
				break;
		} /* end switch */

		if ( i>0 ) doExit = CPHTRUE;

    CPHTRACEEXIT(pConfig->pTrc, CPH_ROUTINE );
    #undef CPH_ROUTINE

    if (CPHTRUE == doExit) exit(0);
    return status;
}

/*
** Method: cphConfigIsEmpty
**
** Detect whether no configuration option have been given
**
** Input Parameters: pConfig - a pointer to the configuration control block
**
** Returns: CPHTRUE if there are no configuration options
**          CPHFALSE if there are configuration options
**
*/
int cphConfigIsEmpty(CPH_CONFIG *pConfig) {
    int res;

#define CPH_ROUTINE "cphConfigIsEmpty"
    CPHTRACEENTRY(pConfig->pTrc, CPH_ROUTINE );

    res = (NULL == pConfig->options);

    CPHTRACEEXIT(pConfig->pTrc, CPH_ROUTINE );
#undef CPH_ROUTINE

    return (res);
}

/*
** Method: cphConfigValidateHelpOptions
**
** Validate the help options "h", "hm", "hf" and "v". If more than one of these have
** been specified, return an error
**
** Input Parameters: pConfig - a pointer to the configuration control block
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int	cphConfigValidateHelpOptions(CPH_CONFIG *pConfig) {

		int count = 0;
        int status = CPHTRUE;

#define CPH_ROUTINE "cphConfigRegisterConfig"
    CPHTRACEENTRY(pConfig->pTrc, CPH_ROUTINE );


        if (CPHTRUE == cphConfigContainsKey(pConfig, "h")) count++;
        if (CPHTRUE == cphConfigContainsKey(pConfig, "hm")) count++;
        if (CPHTRUE == cphConfigContainsKey(pConfig, "hf")) count++;
        if (CPHTRUE == cphConfigContainsKey(pConfig, "v")) count++;

		if ( count > 1 ) {
            cphConfigInvalidParameter(pConfig, "Cannot specify more than one of -h, -hm, -hf or -v");
            status = CPHFALSE;
		}
		
    CPHTRACEEXIT(pConfig->pTrc, CPH_ROUTINE );
#undef CPH_ROUTINE

    return(status);
}

/*
** Method: cphConfigGetVersion
**
** Return the cph version string. This is retrieved from the tool.version name/value pair
** from the "Config" property bundle.
**
** Input Parameters: pConfig - a pointer to the configuration control block
**
** Returns: a character string describing the cph version
**
*/
char *cphConfigGetVersion(CPH_CONFIG *pConfig, char *versionString) {
    CPH_LISTITERATOR *pIterator;
    CPH_ARRAYLISTITEM *pItem;
    CPH_BUNDLE *pBundle;
    char sKeyVal[512];

    #define CPH_ROUTINE "cphConfigGetVersion"
    CPHTRACEENTRY(pConfig->pTrc, CPH_ROUTINE );

    strcpy(sKeyVal, "NO VERSION INFO.\n");
    pIterator = cphArrayListListIterator(pConfig->bundles);
    do {
       pItem = cphListIteratorNext(pIterator);

       pBundle = pItem->item;
       if (0 == strcmp("Config", pBundle->clazzName))
       {
          if (CPHTRUE == cphNameValGet(pBundle->pNameValList, "tool.version", sKeyVal)) {
          }
          break;
       }

    } while (cphListIteratorHasNext(pIterator));
    cphListIteratorFree(&pIterator);

    strcpy(versionString, sKeyVal);

    CPHTRACEEXIT(pConfig->pTrc, CPH_ROUTINE );
    #undef CPH_ROUTINE
 
    return versionString;
}

/*
** Method: cphConfigDescribe
**
** 
**
** Input Parameters: pConfig - a pointer to the configuration control block
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
char *cphConfigDescribe(CPH_CONFIG *pConfig, int inFull) {
    CPH_STRINGBUFFER *pSb;
    CPH_LISTITERATOR *pIterator;
    CPH_ARRAYLISTITEM * pItem;
    CPH_BUNDLE *pBundle;

    char *resString;

    #define CPH_ROUTINE "cphConfigDescribe"
    CPHTRACEENTRY(pConfig->pTrc, CPH_ROUTINE );

    cphStringBufferIni(&pSb);

    pIterator = cphArrayListListIterator(pConfig->bundles);
    do {
       pItem = cphListIteratorNext(pIterator);

       pBundle = pItem->item;

       cphConfigDescribeModuleAsText(pConfig, pSb, pBundle, inFull);

    } while (cphListIteratorHasNext(pIterator));
    cphListIteratorFree(&pIterator);

    resString = (char *) malloc(1 + cphStringBufferGetLength(pSb));
    strcpy(resString, cphStringBufferToString(pSb));
    cphStringBufferFree(&pSb);

    CPHTRACEEXIT(pConfig->pTrc, CPH_ROUTINE );
    #undef CPH_ROUTINE

    return(resString);
}

/*
** Method: cphConfigDescribeModules
**
** 
**
** Input Parameters: pConfig - a pointer to the configuration control block
**
** Returns: a character string of help text
**
*/
char *cphConfigDescribeModules(CPH_CONFIG *pConfig, char *module, int inFull) {
    CPH_STRINGBUFFER *pSb;
    char *resString;
    CPH_BUNDLE *pBundle;
    CPH_LISTITERATOR *pIterator;
    CPH_ARRAYLISTITEM * pItem;
    int moduleFound = CPHFALSE;

    #define CPH_ROUTINE "cphConfigDescribeModules"
    CPHTRACEENTRY(pConfig->pTrc, CPH_ROUTINE );

    cphStringBufferIni(&pSb);

    pIterator = cphArrayListListIterator(pConfig->bundles);
    do {
       pItem = cphListIteratorNext(pIterator);

       pBundle = pItem->item;
       if (0 == strcmp(module, pBundle->clazzName))
       {
          moduleFound = CPHTRUE;
          cphConfigDescribeModuleAsText(pConfig, pSb, pBundle, inFull);
          break;
       }

    } while (cphListIteratorHasNext(pIterator));
    cphListIteratorFree(&pIterator);

    if (CPHFALSE == moduleFound) {
       cphStringBufferAppend(pSb, "UNKNOWN MODULE [");
       cphStringBufferAppend(pSb, module);
       cphStringBufferAppend(pSb, "]\n");
	}

    resString = (char *) malloc(cphStringBufferGetLength(pSb));
    strcpy(resString, cphStringBufferToString(pSb));

    cphStringBufferFree(&pSb);

    CPHTRACEEXIT(pConfig->pTrc, CPH_ROUTINE );
    #undef CPH_ROUTINE

    return(resString);
}

/*
** Method: cphConfigDescribeModuleAsText
**
** Given a CPH_BUNDLE for a module, populate a CPH_STRINGBUFFER with help text about
** the module. This help comes from the information stored in the CPH_BUNDLE that was
** read in from the module's property file.
**
** Input Parameters: pConfig - a pointer to the configuration control block
**                   ptr - a pointer to the CPH_BUNDLE for the module we are describing
**                   inFull - whether or not we give a basic or detailed description
**
** Output Parameters: pSb - pointer to a CPH_STRINGBUFFER containing the help text
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
/* NB: It would be nice to have CPH_BUNDLE* here and not void * for the bundle pointer */
int cphConfigDescribeModuleAsText(CPH_CONFIG *pConfig, CPH_STRINGBUFFER *pSb, void *ptr, int inFull) {
    CPH_BUNDLE *pBundle;
    char sKeyName[80];
    char sKeyVal[2048];
    char *module;
    char keyBase[8];
    CPH_NAMEVAL *pNameVal;
    int hidden;

    #define CPH_ROUTINE "cphConfigDescribeModulesAsText"
    CPHTRACEENTRY(pConfig->pTrc, CPH_ROUTINE );

    pBundle = ptr;

		if ( pBundle==NULL ) { 
			cphStringBufferAppend(pSb, "UNKNOWN MODULE []");
			return(CPHFALSE);
		}
		
        cphStringBufferAppend(pSb, pBundle->clazzName);
        cphStringBufferAppend(pSb, ":\n");

        if (CPHTRUE == inFull) {
            strcpy(sKeyName, pBundle->clazzName);
            strcat(sKeyName, ".desc");

            if (CPHTRUE == cphNameValGet(pBundle->pNameValList, sKeyName, sKeyVal)) {

               cphStringBufferAppend(pSb, cphUtilstrcrlf(sKeyVal));
               cphStringBufferAppend(pSb, "\n\n");
            }
        }

        /* Walk through the properties for this bundle */
        pNameVal = pBundle->pNameValList;
        do  {

             if (cphUtilStringEndsWith(pNameVal->name, ".dflt")) {
               strncpy(keyBase, pNameVal->name, strlen(pNameVal->name) - 5);
               keyBase[ strlen(pNameVal->name) - 5 ] = '\0';

               hidden = CPHFALSE;
               strcpy(sKeyName, keyBase);
               strcat(sKeyName, ".hide");
               if (CPHTRUE == cphNameValGet(pBundle->pNameValList, sKeyName, sKeyVal)) {
                   cphConfigStringToBoolean(&hidden, sKeyVal);
               }

               if (CPHTRUE == inFull || CPHFALSE == hidden ) {
                   strcpy(sKeyName, keyBase);
                   strcat(sKeyName, ".desc");

                   /* Get the key description */
                   if (CPHTRUE == cphNameValGet(pBundle->pNameValList, sKeyName, sKeyVal)) {
                            cphStringBufferAppend(pSb, "  ");
                            cphStringBufferAppend(pSb, keyBase);
                            cphStringBufferAppend(pSb, "\t");
                            cphStringBufferAppend(pSb, sKeyVal);
   
                            /* Get the Default value */
                            strcpy(sKeyName, keyBase);
                            strcat(sKeyName, ".dflt");
                            if (CPHTRUE == cphNameValGet(pBundle->pNameValList, sKeyName, sKeyVal)) {
                                cphStringBufferAppend(pSb, " (default: ");
                                cphStringBufferAppend(pSb, sKeyVal);
                                cphStringBufferAppend(pSb, ")");
                            }

                            cphStringBufferAppend(pSb, "\n");

                            if (CPHTRUE == inFull) {
                                strcpy(sKeyName, keyBase);
                                strcat(sKeyName, ".xtra");
                                if (CPHTRUE == cphNameValGet(pBundle->pNameValList, sKeyName, sKeyVal)) {
                            
                                   cphStringBufferAppend(pSb, "\t");
                                   cphStringBufferAppend(pSb, cphUtilstrcrlfTotabcrlf(sKeyVal));
                                   cphStringBufferAppend(pSb, "\n");

                                   strcpy(sKeyName, keyBase);
                                   strcat(sKeyName, ".modules");
                                   if (CPHTRUE == cphNameValGet(pBundle->pNameValList, sKeyName, sKeyVal)) {
                                       if (CPHTRUE == inFull) {
                                           module = strtok(sKeyVal, " ");
                                           while (NULL != module) {
                                              cphStringBufferAppend(pSb, "\t\t");
                                              cphStringBufferAppend(pSb, module);
                                              cphStringBufferAppend(pSb, "\n");
                                              module = strtok(NULL, " ");
                                           }
                                       }
                                       else {
                                          cphStringBufferAppend(pSb, "\t\tFor full module list use -hm ");
                                          cphStringBufferAppend(pSb, pBundle->clazzName);
                                          cphStringBufferAppend(pSb, "\n");
                                       }

                                   }
                                }
                            }
                   }
               }
            }
        } while (CPHTRUE == cphConfigNextNameVal(&pNameVal));

    CPHTRACEEXIT(pConfig->pTrc, CPH_ROUTINE );
    #undef CPH_ROUTINE

    return(CPHTRUE);
}

/*
** Method: cphConfigRegisterBaseModules
**
** This function process the property files for the "fixed" modules (i.e. everything other than the -tc module)
** It does this by calling cphConfigRegisterModule for each module.
**
** Input Parameters: pConfig - a pointer to the configuration control block
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphConfigRegisterBaseModules(CPH_CONFIG *pConfig) {

    int status = CPHTRUE;

    #define CPH_ROUTINE "cphConfigRegisterBaseModules"
    CPHTRACEENTRY(pConfig->pTrc, CPH_ROUTINE );

    if ( (CPHTRUE != cphConfigRegisterModule(pConfig, "Config")) ||
         (CPHTRUE != cphConfigRegisterModule(pConfig, "Log")) ||
         (CPHTRUE != cphConfigRegisterModule(pConfig, "ControlThread")) ||
         (CPHTRUE != cphConfigRegisterModule(pConfig, "WorkerThread")) ||
         (CPHTRUE != cphConfigRegisterModule(pConfig, "DestinationFactory"))
       ) {
        status = CPHFALSE;
    }

    CPHTRACEEXIT(pConfig->pTrc, CPH_ROUTINE );
    #undef CPH_ROUTINE

    return(status);
}

/*
** Method: cphConfigRegisterTCModule
**
** This function processes the property file for the -tc module (i.e. the Publisher, Subscriber or Dummy module)
** It does this by calling cphConfigRegisterModule for the -tc module module as set in pConfig->clazzName
**
** Input Parameters: pConfig - a pointer to the configuration control block
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphConfigRegisterTCModule(CPH_CONFIG *pConfig) {
    int status = CPHTRUE;

    #define CPH_ROUTINE "cphConfigRegisterTCModule"
    CPHTRACEENTRY(pConfig->pTrc, CPH_ROUTINE );

    if ( CPHTRUE != cphConfigRegisterModule(pConfig, pConfig->clazzName)
       ) {
        status = CPHFALSE;
    }

    CPHTRACEEXIT(pConfig->pTrc, CPH_ROUTINE );
    #undef CPH_ROUTINE

    return(status);
}

/*
** Method: cphConfigRegisterModule
**
** This function:
**
** - reads in the required property file for the specified module name 
** - creates a CPH_BUNDLE for the contents of the property file
** - calls cphConfigImportResources to add the default options from the bundle to the CPH_NAMEVAL
**   list of default options
**
** Input Parameters: pConfig - a pointer to the configuration control block
**                   moduleName - the name of the module
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphConfigRegisterModule(CPH_CONFIG *pConfig, char *moduleName) {
    int status = CPHTRUE;
    CPH_BUNDLE *pBundle;

    #define CPH_ROUTINE "cphConfigRegisterModule"
    CPHTRACEENTRY(pConfig->pTrc, CPH_ROUTINE );

      if (NULL != (pBundle = cphBundleLoad(pConfig, moduleName)))
        status = cphConfigImportResources(pConfig, pBundle);
      else 
        status = CPHFALSE;

    CPHTRACEEXIT(pConfig->pTrc, CPH_ROUTINE );
    #undef CPH_ROUTINE

    return(status);
}

/*
** Method: cphConfigImportResources
**
** This function processes a CPH_BUNDLE, which represents the contents of a property file, and
** extracts the default options for each property and adds that to the CPH_NAMEVAL list of 
** defaultOptions that are stored in the configuration control block.
**
** Input Parameters: pConfig - a pointer to the configuration control block
**                   ptr - a pointer to the CPH_BUNDLE from which we are extracting default options
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphConfigImportResources(CPH_CONFIG *pConfig, void *ptr) {
    CPH_BUNDLE *pBundle;
    CPH_NAMEVAL *bundleNameVal;
    char keyBase[8];
    char res[512];
    char errorString[512];
    int status = CPHTRUE;

    #define CPH_ROUTINE "cphConfigImportResources"
    CPHTRACEENTRY(pConfig->pTrc, CPH_ROUTINE );

    pBundle = ptr;

    bundleNameVal = pBundle->pNameValList;

    do  {

        if (cphUtilStringEndsWith(bundleNameVal->name, ".dflt")) {

            strncpy(keyBase, bundleNameVal->name, strlen(bundleNameVal->name) - 5);
            keyBase[ strlen(bundleNameVal->name) - 5 ] = '\0';

            /* Has this property already been registered? */
            /* NB - we treat this as a fatal error, the original Java gives the message but continues */
            if (CPHTRUE == cphNameValGet(pConfig->defaultOptions, keyBase, res)) {

               strcpy(errorString, "Property [");
               strcat(errorString, keyBase);
               strcat(errorString,"] exists twice in ResourceBundles");
               cphConfigInvalidParameter(pConfig, errorString);
               status = CPHFALSE;
               break;
            }

            if (CPHTRUE != cphNameValAdd(&pConfig->defaultOptions, keyBase, bundleNameVal->value)) {
               cphLogPrintLn(pConfig->pLog, LOGERROR, "ERROR setting default option.");
               status = CPHFALSE;
               break;
            }

        }

    } while (CPHTRUE == cphConfigNextNameVal(&bundleNameVal));

    CPHTRACEEXIT(pConfig->pTrc, CPH_ROUTINE );
    #undef CPH_ROUTINE

    return(status);
}

/*
** Method: cphConfigStartTrace
**
** If requested from the command line option "tr", enable tracing by setting the trace flag
** in the configuration control block
**
** Input Parameters: pConfig - pointer to the configuration control block
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphConfigStartTrace(CPH_CONFIG *pConfig) {
    int doTrace = CPHFALSE;

    if (CPHTRUE != cphConfigGetBoolean(pConfig, &doTrace, "tr")) return(CPHFALSE);
    if (CPHTRUE == doTrace) 
    {
        CPHTRACEOPENTRACEFILE(pConfig->pTrc);
        CPHTRACEON(pConfig->pTrc);
    }

    return(CPHTRUE);
}

/*
** Method: cphConfigFreeBundles
**
** Dispose of each CPH_BUNDLE registered with this configuration
**
** Input Parameters: pConfig - pointer to the configuration control block
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphConfigFreeBundles(CPH_CONFIG *pConfig) {
    CPH_TRACE *pTrc;
    CPH_LISTITERATOR *pIterator;
    CPH_ARRAYLISTITEM * pItem;
    CPH_BUNDLE *pBundle;

    int status = CPHTRUE;

    pTrc = pConfig->pTrc;

#define CPH_ROUTINE "cphConfigFreeBundles"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );

    if (NULL != pConfig->bundles) {
        pIterator = cphArrayListListIterator( pConfig->bundles);
        do {
            pItem = cphListIteratorNext(pIterator);
            pBundle = pItem->item;
            cphBundleFree(&pBundle);
        } while (cphListIteratorHasNext(pIterator));
        cphListIteratorFree(&pIterator);
    }

    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
#undef CPH_ROUTINE

    return(status);
}
