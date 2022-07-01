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
import sys
import os
import xml.etree.ElementTree as ET

#Import replacement vars code from ../../server_build
sys.path.append(os.path.join(os.path.dirname(os.path.dirname(sys.path[0])),'server_build'))
import path_parser

def parseFile(filepath):
    """returns the root node of the XML tree"""
    tree = parseFileToTree(filepath)
    root = tree.getroot()
    return root

def parseFileToTree(filepath):
    """returns the generated XML tree"""
    instring = ""

    with open(filepath, "r", encoding='utf-8') as infile:
        instring = infile.read()

    varreplacedstring = path_parser.parseString(instring)
    
    tree = ET.ElementTree(ET.fromstring(varreplacedstring))
    return tree

def parseFileNameForLangVars(infilename, lang):
    step1 = infilename.replace('%LANG%', lang)

    alteredlang = lang.lower().replace("_","-")

    return step1.replace('%lang%', alteredlang)

def parseLanguageLists(langlist):
    """We can be given multiple args in a list and each arg is a whitespace separated
       list of languages"""
    individuallangs = []

    for elem in langlist:
        individuallangs += elem.split()

    return individuallangs

def createOutputDir(outpath):
    pathlib.Path(os.path.dirname(outpath)).mkdir(parents=True, exist_ok=True)
