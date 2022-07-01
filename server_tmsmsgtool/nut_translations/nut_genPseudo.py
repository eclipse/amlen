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
import xml.etree.ElementTree as ET
import re
import os
import pathlib
import json

import nut_utils

def pseudoTranslateText(lang, intext):
    """We make non-empty strings longer (as English is often compact) and
       (for known langs) convert a few characters into common non low-ASCII
       characters""" 
    if intext:
        if intext.strip():
            prefixchars = '['+lang
            suffixchars = lang+']'

            if lang == 'de':
               prefixchars = '[ß'
               str1  = re.sub("a","ä", intext)
               str2  = re.sub("o","ö", str1)
               str3  = re.sub("u","ü", str2)
               str4  = re.sub("A","Ä", str3)
               str5  = re.sub("U","Ü", str4)
               str_stage1 = str5
            elif lang == 'fr':
               prefixchars = '[œÿ'
               str1  = re.sub("a","à", intext)
               str2  = re.sub("c","ç", str1)
               str3  = re.sub("e","é", str2)
               str4  = re.sub("i","î", str3)
               str5  = re.sub("o","ô", str4)
               str6  = re.sub("u","û", str5)
               str7  = re.sub("C","Ç", str6)
               str_stage1 = str7
            elif lang == 'ja':
               prefixchars = '[表竹～'
               str_stage1 = intext
            elif lang == 'zh':
               prefixchars = '[黒憒違'
               str_stage1 = intext
            elif lang == 'zh_TW':
               prefixchars = '[牾四亡'
               str_stage1 = intext
            else:
               str_stage1 = intext
            
            paddingstr = '~' * (0+int(0.25 *len(str_stage1)))
            str1 = re.sub("^",prefixchars,str_stage1,count=1)
            str2 = re.sub("(\s*)$",paddingstr+suffixchars+r'\1',str1,count=1)
            #print('Turned:\n'+intext+'\ninto:\n'+str2+'\n')
            return str2
        else:
            return intext
    
    return intext

def getTranslatedFileName(logger, lang, inputfile, outfile, outbasedir,mklangdir):
    """If we are given an output filename - just use it. Otherwise
       either keep the original filename but put it in a per-lang subdir or
       add a per-lang suffix to the filename (based on the mklangdir flag)""" 
    dirstomake = []

    if outfile:
        outpath = outfile
        dirstomake.append(os.path.dirname(outpath))
    else:
        if not outbasedir:
            outbasedir =  os.path.dirname(inputfile)
        
        #When we include the lang in XML file names or dirs, we lower case and replace _ with -

        if mklangdir or inputfile.endswith(".xml"):
            fnlang = re.sub('_','-',lang).lower()
        else:
            fnlang = lang
        
        if mklangdir:
            langdir = os.path.join(outbasedir, fnlang)
            outpath = os.path.join(langdir, os.path.basename(inputfile))
            dirstomake.append(langdir)
        else:
            basename = os.path.basename(inputfile)
            #Add _<lang> before the extension:
            newbasename = re.sub('\.([^.]*)$','_'+fnlang+r'.\1',basename)
            outpath = os.path.join(outbasedir, newbasename)
            dirstomake .append(outbasedir)

    for dirname in dirstomake:
        try:
            logger.debug('Making dir: '+dirname)
            os.makedirs(dirname, exist_ok=True)
        except Exception as e:
           logger.error("Failed to create directory %s - error %s" % (dirname,e))
           return False

    return outpath

def processLanguageList(pseudolanglist):
    """We can be given multiple args in a list and each arg is a whitespace separated
       list of languages"""
    individuallangs = []

    for elem in pseudolanglist:
        individuallangs += elem.split()

    return individuallangs

#Convert characters >128 in ascii to \uXXXX
def unicode_escaped(obj):
    res = json.dumps(obj)
    if res[0] == res[-1] == '"':
        res = res[1:-1]
    return res

def processSourceLine(lang, line, unicodeescape, in_string, current_string):
    processed_line = ""

    for ch in line:
        if ch == '"':
            if in_string:
                #Check if the last character in the current_string is an escape char
                if len(current_string) > 0 and current_string[-1] == '\\':
                    current_string += ch
                else:
                    in_string = False

                    if unicodeescape:
                        processed_line += unicode_escaped(pseudoTranslateText(lang, current_string))
                    else:
                        processed_line += pseudoTranslateText(lang, current_string)
                    processed_line += '"'
            else:
                processed_line += '"'
                if not "//" in processed_line:
                    in_string = True
            
            current_string = ""
        elif in_string:
            current_string += ch
        else:
            processed_line += ch
    
    return (processed_line, in_string, current_string)

def processJSFile(logger, lang, inputfile, outfilepath):
    """From reverse engineering RPX, inside a define({ it looks for a JSON entry called root, and
       only includes the value of that entry ignoring the rest of the JSON - this does the same
       to mimic the older program without properly parsing the JSON so will only work for input
       files in a very similar format to the existing files"""
    with open(inputfile) as inf:
        with open(outfilepath, "w", encoding="utf8") as outf:
            in_define = False
            in_root   = False
            in_string = False
            root_had_curly = False #Does the root element have a ( as well as a { enclosing it
            current_string = ""

            for line in inf:
                if in_root:
                    if (    (line.strip() == "})," and root_had_curly)
                         or (line.strip() == "}," and not root_had_curly)):
                        #ending the root entry
                        outf.write(line.replace(",", ""))
                        in_root = False
                    else:
                        (converted_line, in_string, current_string) = processSourceLine(lang, line, False, 
                                                                                        in_string, current_string)

                        outf.write(converted_line)
                elif in_define:
                    if line.strip().startswith("root :"):
                        outf.write(line.replace("root :", ""))
                        in_root = True
                        if "(" in line:
                            root_had_curly = True
                        else:
                            root_had_curly = False
                    elif line.strip().startswith("});"):
                        outf.write(line.replace("}", ""))
                        in_define = False
                else:
                    if line.strip().startswith("define({"):
                        outf.write(line.replace("{", ""))
                        in_define = True
                    else:
                        outf.write(line)

def processJavaFile(logger, lang, inputfile, outfilepath):
    """Changes the class name (assuming public class) to match the outfilename as well as pseudo translating the strings
       (string in between START NON-TRANSLATABLE  and END NON-TRANSLATABLE are not translated)"""
    
    with open(inputfile) as inf:
        with open(outfilepath, "w", encoding="utf8") as outf:
            translating = True
            in_string = False
            current_string = ""

            for line in inf:
                if "START NON-TRANSLATABLE" in line:
                    translating = False
                elif "END NON-TRANSLATABLE" in line:
                    translating = True

                if line.strip().startswith("public class"):
                    infilestem  = pathlib.Path(inputfile).stem
                    outfilestem = pathlib.Path(outfilepath).stem

                    outf.write(line.replace(infilestem, outfilestem))
                elif translating:
                    (converted_line, in_string, current_string) = processSourceLine(lang, line, True, 
                                                                                        in_string, current_string)

                    outf.write(converted_line)
                else:
                    outf.write(line)


def processFile(logger, pseudolanglist, inputfile, outfile, outbasedir, mklangdir):
    langs = processLanguageList(pseudolanglist)

    for lang in langs:
        outfilepath = getTranslatedFileName(logger, lang, inputfile, outfile, outbasedir,mklangdir)

        if inputfile.endswith('.xml'):
            xmltree = nut_utils.parseFileToTree(inputfile)
        
            for elem in xmltree.iter():
                elem.text = pseudoTranslateText(lang, elem.text)

            logger.info('Creating pseudotranslated file (lang='+lang+'): '+outfilepath)
            xmltree.write(outfilepath, encoding="UTF-8")
        elif inputfile.endswith('.js'):
            processJSFile(logger, lang, inputfile, outfilepath)
        elif inputfile.endswith('.java'):
            processJavaFile(logger, lang, inputfile, outfilepath)
        else:
            logger.error("Don't know how to pseudo translate file:  %s" % (inputfile))


def outputPseudoTranslation(logger, pseudolanglist, inputfile, inputdir, outfile, outbasedir, mklangdir):  
    if inputfile:
        processFile(logger, pseudolanglist, inputfile, outfile, outbasedir, mklangdir)

    if inputdir:
        for (root, dirs, files) in os.walk(inputdir):
            for fname in files:
                filepath = os.path.join(root, fname)

                processFile(logger, pseudolanglist, filepath, outfile, outbasedir, mklangdir)
