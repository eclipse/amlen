/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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

#ifndef __ISMREGEX_DEFINED
#define __ISMREGEX_DEFINED

typedef struct ism_regex_t * ism_regex_t;

typedef unsigned int ism_regex_offset_t;

typedef struct ism_regex_matches_t {
    ism_regex_offset_t startOffset;
    ism_regex_offset_t endOffset;
} ism_regex_matches_t;

/*
 * Compile a regular expression.
 *
 * @param  pregex     (output) The location to return the compiled regular expression.
 * @param  regex_str  A regular expression in string form
 * @return A regex specific return code
 */
int ism_regex_compile(ism_regex_t * pregex, const char * regex_str);

/*
 * Compile a regular expression (extended syntax and sub expression matching).
 *
 * @param  pregex       (output) The location to return the compiled regular expression.
 * @param  psubexprcnt  (output) The number of subexpressions contained in the regular expression
 * @param  regex_str    A regular expression in string form
 * @return A regex specific return code
 */
int ism_regex_compile_subexpr(ism_regex_t * pregex, int *psubexprcnt, const char * regex_str);

/*
 * Return the error string for a regex error
 *
 * @param rc   A return code from ism_regex_compile() or ism_regex_match()
 * @param buf  A buffer in which to return the string
 * @param len  The length of the buffer
 * @return The return error string
 */
const char * ism_regex_getError(int rc, char * buf, int len, ism_regex_t regex);

/*
 * Match a regular expression.
 *
 * @param regex  A previously compiled regular expression
 * @param str    An arbitrary string to match
 * @return A return code 0=match, 1=not matched, other=error
 */
int ism_regex_match(ism_regex_t regex, const char * str);

/*
 * Match a regular expression.
 *
 * @param regex       A previously compiled regular expression
 * @param str         An arbitrary string to match
 * @param maxMatches  max number of matches caller wants
 * @param matches     array of results of size maxMatches
 * @return A return code 0=match, 1=not matched, other=error
 */
int ism_regex_match_subexpr(ism_regex_t regex, const char * str, int maxMatches, ism_regex_matches_t *matches);

/*
 * Free a regular expression
 * @pararm regex  A compiled regular expression
 */
void ism_regex_free(ism_regex_t regex);

#endif


