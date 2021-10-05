#!/usr/bin/python3
# Copyright (c) 2021 Contributors to the Eclipse Foundation
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
import pathlib
import datetime

#Our sub modules:
import nut_parse
import nut_genICU
import nut_genLRB
import nut_genPRB
import nut_genHTML
import nut_genTOC
import nut_genPseudo
import nut_genDITA

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

def createOutputDir(outpath):
    pathlib.Path(os.path.dirname(outpath)).mkdir(parents=True, exist_ok=True)

def parseArguments():
    parser = argparse.ArgumentParser()
    
    args_ok = True
    
    parser.add_argument('-i', '--input', required=False, action='append', help="XML input file to process - can be specified multiple times")
    parser.add_argument('-o', '--outputfile', required=False, help="Output file to write")
    parser.add_argument('-c', '--container', required=False, help="Name of container in output (for ICU format, the class in LRB)")
    parser.add_argument('-p', '--package', required=False, help="Package output is part of (for LRB format)")
    parser.add_argument('-b', '--outputbasedir',  required=False, help="Root of output tree (for LRB/HTML and optionally pseudotranslation format)")
    parser.add_argument('-x', '--inputdir',  required=False, help="Directory to look for input files (with .xml extension for DITA format only) (with.js/.java for pseudotranslation)")
    parser.add_argument('-l', '--language', required=False, help="Language of files (from DITA format only)")
    parser.add_argument('-s', '--pseudolanglist', required=False, action='append', help="Languages to generate (in pseudo translation  mode) can be specified multiple times")
    parser.add_argument('-d', '--langsubdir',required=False, action='store_true', help="Create a subdir per lang in the output base dir (only for pseudotranslation)")
    parser.add_argument('-m', '--mode', type=str, default='icu', help="Output format", choices=['icu','lrb','prb','html','toc','dita','pseudotranslation'])

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
        if not args.pseudolanglist:
            print("-s/--pseudolanglist must be supplied in pseudotranslation mode")
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
        for infile in args.input:    
            if args.mode == 'icu':
                createOutputDir(args.outputfile)
                inputxmlroot = nut_parse.parseFile(infile)
                nut_genICU.outputICU(logger, infile, inputxmlroot, args.outputfile, args.container)
            elif args.mode == 'lrb':
                inputxmlroot = nut_parse.parseFile(infile)
                nut_genLRB.outputLRB(logger, infile, inputxmlroot, args.outputbasedir, args.package, args.container)
            elif args.mode == 'prb':
                createOutputDir(args.outputfile)
                inputxmlroot = nut_parse.parseFile(infile)
                nut_genPRB.outputPRB(logger, infile, inputxmlroot, args.outputfile)
            elif args.mode == 'html':
                inputxmlroot = nut_parse.parseFile(infile)
                nut_genHTML.outputHTML(logger, infile, inputxmlroot, args.outputbasedir)
            elif args.mode == 'toc':
                inputxmlroot = nut_parse.parseFile(infile)
                nut_genTOC.outputTOC(logger, infile, inputxmlroot, args.outputbasedir)
            elif args.mode == 'pseudotranslation':
                nut_genPseudo.outputPseudoTranslation(logger, args.pseudolanglist, infile, args.inputdir, args.outputfile, args.outputbasedir, args.langsubdir)
            elif args.mode == 'dita':
                nut_genDITA.outputDITA(logger, args.language, infile, args.inputdir, args.outputbasedir)
    else:
        if args.mode == 'dita':
            nut_genDITA.outputDITA(logger, args.language, None, args.inputdir, args.outputbasedir)
        if args.mode == 'pseudotranslation':
            nut_genPseudo.outputPseudoTranslation(logger, args.pseudolanglist, None, args.inputdir, args.outputfile, args.outputbasedir, args.langsubdir)
















 
