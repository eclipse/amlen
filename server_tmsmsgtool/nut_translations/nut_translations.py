#!/usr/bin/python3
# Copyright (c) 2021-2022 Contributors to the Eclipse Foundation
# 
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
# 
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0
# 
# SPDX-License-Identifier: EPL-2.0
#
#
# Nut (Nu-TMS) is a python script that aims to process the translations
# in the same way as the closed-source TMS utilities
#

import logging
import logging.handlers
import xml.etree.ElementTree as ET
import json
import re
import argparse
import html
import os
import datetime
import sys
print (sys.argv[1:]);


#Our sub modules:
import nut_utils
import nut_genICU
import nut_genLRB
import nut_genPRB
import nut_genHTML
import nut_genTOC
import nut_genPseudo
import nut_genDITA
import nut_noop
import nut_msgtoolwrapper
import nut_xsltprocwrapper
import nut_checkjstrans
import nut_fixclass

LOG_FILENAME = "nut_translations.log"

logger = logging.getLogger("nut_translations")
logger.setLevel(logging.DEBUG)
logfileformatter = logging.Formatter('%(asctime)-25s %(name)-30s ' + ' %(levelname)-8s %(message)s')
handler = logging.handlers.RotatingFileHandler(LOG_FILENAME, maxBytes=10485760, backupCount=1) # max size of 10Mb and 1 backup
handler.setFormatter(logfileformatter)
logger.addHandler(handler)

consoleHandler = logging.StreamHandler()
consoleHandler.setLevel(logging.INFO)
consoleFormatter = logging.Formatter('%(name)-30s ' + ' %(levelname)-8s %(message)s')
consoleHandler.setFormatter(consoleFormatter)
logger.addHandler(consoleHandler)

def parseArguments():
    parser = argparse.ArgumentParser()
    
    args_ok = True
    
    parser.add_argument('-i', '--input', required=False, action='append', help="XML input file to process - can be specified multiple times")
    parser.add_argument('-o', '--outputfile', required=False, help="Output file to write")
    parser.add_argument('-c', '--container', required=False, help="Name of container in output (for ICU format, the class in LRB)")
    parser.add_argument('-p', '--package', required=False, help="Package output is part of (for LRB format)")
    parser.add_argument('-b', '--outputbasedir',  required=False, help="Root of output tree (for LRB/HTML/dita/msgtoolwrapper and optionally pseudotranslation format)")
    parser.add_argument('-z', '--zipoutput',  required=False, help="zip (and remove) outputdir (DITA format only)")
    parser.add_argument('-x', '--inputdir',  required=False, help="Directory to look for input files (with .xml extension for DITA format only) (with.js/.java for pseudotranslation)")
    parser.add_argument('-l', '--language', required=False, help="Language of files (from DITA format only)")
    parser.add_argument('-s', '--langlist', required=False, action='append', help="Languages to generate (only some modes support this) can be specified multiple times")
    parser.add_argument('-d', '--langsubdir', required=False, action='store_true', help="Create a subdir per lang in the output base dir (only for pseudotranslation)")
    parser.add_argument('--msgtoolbindir',required=False, help="Bin directory (and working dir) for java msgtool which validates+combines tms files")
    parser.add_argument('--msgtoolclasspath',required=False, help="classpath containing msgtool class")
    parser.add_argument('--msgtoolclass',required=False, help="Class name of main class for java msgtool which validates+combines tms files")
    parser.add_argument('--xsl',required=False, help="Path to xsl file in xsltprocwrapper mode")
    parser.add_argument('--extraargs',required=False, help="Extra args to pass to the ISMMsgTool/xsltproc")
    parser.add_argument('--translationrootdir',required=False, help="Where to look for the translations to check in checkjstrans mode")
    parser.add_argument('-r', '--replace_filename_vars', required=False, action='store_true', help="In Container/Input/Output filename replace %%LANG%% with language and %%lang%% with lowercase (and _ -> -)")
    parser.add_argument('-m', '--mode', type=str, default='icu', help="Output format", choices=['icu','lrb','prb','html','toc','dita','pseudotranslation','msgtoolwrapper','xsltprocwrapper','checkjstrans','fixclassname','noop'])

    args, unknown = parser.parse_known_args()
    
    if not args.input:
       if args.mode == 'dita':
            if not args.inputdir:
                print("-i/--input OR -x/--inputdir must be supplied in DITA mode")
                args_ok=False
       elif args.mode == 'pseudotranslation':
            if not args.inputdir:
                print("-i/--input OR -x/--inputdir must be supplied in PseudoTranslation mode")
                args_ok=False
       else:
           print("-i/--input must be supplied in "+args.mode+" mode")
           args_ok=False

    if args.mode == 'icu':
        if not args.outputfile:
            print("-o/--outputfile must be supplied in ICU mode")
            args_ok=False
        if not args.container:
            print("-c/--container must be supplied in ICU mode as the label on the stanza containing the messages.")
            args_ok=False
    elif args.mode == 'lrb':
        if not args.package:
            print("-p/--package must be supplied in LRB mode")
            args_ok=False
        if not args.outputbasedir:
            print("-b/--outputbasedir must be supplied in LRB mode")
            args_ok=False
        if not args.container:
            print("-c/--container must be supplied in LRB mode as the class containing the messages.")
            args_ok=False
    elif args.mode == 'prb':
        if not args.outputfile:
            print("-o/--outputfile must be supplied in PRB mode")
            args_ok=False
    elif args.mode == 'html':
        if not args.outputbasedir:
            print("-b/--outputbasedir must be supplied in HTML mode")
            args_ok=False
    elif args.mode == 'toc':
        if not args.outputbasedir:
            print("-b/--outputbasedir must be supplied in TOC mode")
            args_ok=False
    elif args.mode == 'dita':
        if not args.outputbasedir:
            print("-b/--outputbasedir must be supplied in DITA mode")
            args_ok=False
        if not args.language:
            print("-l/--language must be supplied in DITA mode")
            args_ok=False
    elif args.mode == 'pseudotranslation':
        if not args.langlist:
            print("-s/--langlist must be supplied in pseudotranslation mode")
            args_ok=False
    elif args.mode == 'msgtoolwrapper':
        if not args.langlist:
            print("-s/--langlist must be supplied in msgtoolwrapper mode")
            args_ok=False
        if not args.msgtoolbindir:
            print("--msgtoolbindir must be supplied in msgtoolwrapper mode")
            args_ok=False
        if not args.msgtoolclass:
            print("--msgtoolclass must be supplied in msgtoolwrapper mode")
            args_ok=False
        if not args.msgtoolclasspath:
            print("--msgtoolclasspath must be supplied in msgtoolwrapper mode")
            args_ok=False
        if not args.extraargs:
            print("--extraargs must be supplied in msgtoolwrapper mode")
            args_ok=False
    elif args.mode == 'xsltprocwrapper':
        if not args.langlist:
            print("-s/--langlist must be supplied in xsltprocwrapper mode")
            args_ok=False
        if not args.xsl:
            print("--xsl must be supplied in xsltprocwrapper mode")
            args_ok=False
        if not args.outputfile:
            print("--outputfile/-o must be supplied in xsltprocwrapper mode")
            args_ok=False
    elif args.mode == 'checkjstrans':
        if not args.translationrootdir:
            print("--translationrootdir must be supplied in checkjstrans mode")
            args_ok=False 
    elif args.mode == 'noop':
        if not args.langlist:
            print("-s/--langlist must be supplied in noop mode")
            args_ok=False
        if not args.outputfile:
            print("-o/--outputfile must be supplied in PRB mode")
            args_ok=False
    if unknown:
        print('Unexpected args: '+' '.join(map(str, unknown)))
        args_ok=False
     
    if not args_ok:
        exit(10)

    return args

if __name__ == "__main__":
    
    args = parseArguments()
    
    if args.input:
        #Some modes expect the list of files all in one go, other are called separately for each file
        if args.mode == 'icu':
            nut_genICU.outputICU(logger, args.langlist, args.replace_filename_vars, args.input, args.outputfile, args.container)
        if args.mode == 'noop':
            nut_noop.copyLangFiles(logger,args.langlist, args.replace_filename_vars, args.input, args.outputfile)
        elif args.mode =='msgtoolwrapper':
            nut_msgtoolwrapper.runMsgTool(logger,args.langlist, args.replace_filename_vars, args.input, args.outputbasedir,
                                                        args.msgtoolbindir, args.msgtoolclasspath, args.msgtoolclass, args.extraargs)
        elif args.mode =='xsltprocwrapper':
            nut_xsltprocwrapper.runXSLTProc(logger,args.langlist, args.replace_filename_vars, args.input, args.outputfile,
                                                        args.xsl, args.extraargs)
        elif args.mode =='checkjstrans':
            nut_checkjstrans.checkJSTrans(logger, args.input, args.translationrootdir)
        elif args.mode == 'dita':
            nut_genDITA.outputDITA(logger, args.langlist, args.replace_filename_vars, args.language, args.input, args.inputdir, args.outputbasedir, args.zipoutput)
        else:
            for infile in args.input:
                if args.mode == 'lrb':
                    nut_genLRB.outputLRB(logger, args.replace_filename_vars, infile, args.langlist, args.outputbasedir, args.package, args.container)
                elif args.mode == 'prb':
                    nut_genPRB.outputPRB(logger, args.replace_filename_vars, infile, args.langlist, args.outputfile)
                elif args.mode == 'html':
                    inputxmlroot = nut_utils.parseFile(infile)
                    nut_genHTML.outputHTML(logger, infile, inputxmlroot, args.outputbasedir)
                elif args.mode == 'toc':
                    inputxmlroot = nut_utils.parseFile(infile)
                    nut_genTOC.outputTOC(logger, infile, inputxmlroot, args.outputbasedir)
                elif args.mode == 'pseudotranslation':
                    nut_genPseudo.outputPseudoTranslation(logger, args.langlist, infile, args.inputdir, args.outputfile, args.outputbasedir, args.langsubdir)
                elif args.mode == 'fixclassname':
                    nut_fixclass.fixClassName(logger,infile)
    else:
        if args.mode == 'dita':
            nut_genDITA.outputDITA(logger, args.langlist, args.replace_filename_vars, args.language, None, args.inputdir, args.outputbasedir, args.zipoutput)
        if args.mode == 'pseudotranslation':
            nut_genPseudo.outputPseudoTranslation(logger, args.langlist, None, args.inputdir, args.outputfile, args.outputbasedir, args.langsubdir)















 
