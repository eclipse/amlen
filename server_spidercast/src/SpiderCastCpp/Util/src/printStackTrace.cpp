// Copyright (c) 2010-2021 Contributors to the Eclipse Foundation
// 
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// 
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0
// 
// SPDX-License-Identifier: EPL-2.0
//
/*
 * printStackTrace.cpp
 *
 *  Created on: May 26, 2010
 *      Author:
 *
 * Version     : $Revision: 1.2 $
 *-----------------------------------------------------------------
 * $Log: printStackTrace.cpp,v $
 * Revision 1.2  2015/11/18 08:36:56 
 * Update copyright headers
 *
 * Revision 1.1  2015/03/22 12:29:20 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.5  2012/10/25 10:11:15 
 * Added copyright and proprietary notice
 *
 * Revision 1.4  2010/07/20 14:30:38 
 * *** empty log message ***
 *
 * Revision 1.3  2010/07/15 12:58:11 
 * Make it applicable only on Windows
 *
 * Revision 1.2  2010/07/15 12:46:02 
 * Make it applicable only on Windows
 *
 * Revision 1.1  2010/07/15 09:10:16 
 * Initial version
 *
 */

using namespace std;
#include <iostream>

#ifdef SPDR_WINDOWS_DEBUG_TOOLS
#ifdef WIN32
#include <windows.h>
#include "DebugTools.h"

void printStackTrace()
{
	cerr	<< "Going to dump threads. DebugServices must be started before in order to get dump."
			<< endl;
#ifndef SPDR_DEBUG_TOOLS_DYNAMIC_LOAD
	DumpAllThreads();
	STACK_TRACE stack = GetStackTrace();
	cerr << "The stackTrace is: " << endl << stack << endl;
	FreeStackTrace(stack);
#else
	xDumpAllThreads();
	STACK_TRACE stack = xGetStackTrace();
	cerr << "The stackTrace is: " << endl << stack << endl;
	xFreeStackTrace(stack);
#endif
}
#else // WIN32
void printStackTrace()
{
	cerr << "Not supported..." << endl;
}
#endif // WIN32
#endif // SPDR_WINDOWS_DEBUG_TOOLS

void printStackTrace()
{
	cerr << "Not supported..." << endl;
}

