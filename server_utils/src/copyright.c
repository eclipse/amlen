/*
 * Copyright (c) 2013-2021 Contributors to the Eclipse Foundation
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

#ifndef XSTR
#define XSTR(s) STR(s)
#define STR(s) #s
#endif

#define xUNUSED __attribute__((__unused__))

/*
 * ISM_VERSION, BUILD_LABEL, ISMDATE and ISMTIME are defined in the makefile.
 * version_string must have ":" in it
 */
xUNUSED static char * version_string = IMA_PRODUCTNAME_FULL " version_string: " XSTR(ISM_VERSION) " " XSTR(BUILD_LABEL) " " XSTR(ISMDATE) " " XSTR(ISMTIME);
xUNUSED static const char * ima_static_copyright_string =
"" IMA_PRODUCTNAME_FULL ""
"(C) Copyright Contributors to the Eclipse Foundation 2012-2021. ";

//Deliberately not marking static so it can be printed in gdb
//(when it's static we need a frame in this source file)
//If this reliably doesn't get removed from binaries we can
//remove the static from the above strings as well
xUNUSED char *ima_copyright_sourcelevel_string = IMA_PRODUCTNAME_FULL " source level info: " XSTR(IMA_SOURCELEVEL_INFO);

/*
 * Return the version of the library
 */
xUNUSED const char * ima_libraryVersion(void) {
    return version_string;
}

/*
 * Return the static copyright string.
 * While never called, this function is needed
 * to assure the copyright string is embedded in
 * the binary.
 */
xUNUSED const char * ima_getCopyrightString(void) {
    return ima_static_copyright_string;
}

/*
 * Return the source level string. Never called but
 * helps to ensure not optimised out of binaries
 */
 xUNUSED const char * ima_getSourceLevelString(void) {
     return ima_copyright_sourcelevel_string;
 }
