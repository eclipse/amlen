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
import os
import re
import datetime
import xml.etree.ElementTree as ET
import html
import pathlib
    
def msgTextNodeToHTMLString(xmlnode):
    '''The node can contain innertext and tags like <q />'''
    strwithtags = ET.tostring(xmlnode).decode()

    #Remove starting tag
    str1  = re.sub("^<[^>]*>\n?",'',strwithtags)
    #Remove ending tag
    str2  = re.sub("<\s*/[^>]*>\s*\n?\s*$",'',str1)
    
    str3  = str2.strip()
    return str3

def mkHTMLPath(outbasedir):
    pathlib.Path(outbasedir).mkdir(parents=True, exist_ok=True)

def idsorter(messageidstring):
    numbers = messageidstring[5:]
    
    return int(numbers)

def outputTOC(logger, inputfile, inputxmlroot, outbasedir):
    timestr = datetime.datetime.now().strftime("%H:%M%:%S on %d %B %Y")
    logger.info("Converting "+inputfile+" to TOC output file in: "+outbasedir+" at "+timestr)
    
    mkHTMLPath(outbasedir)
    
    outpath = os.path.join(outbasedir, "toc.xml")
    
    try:
       outfile = open(outpath, "w", encoding="utf8")
       outfile.write('<?xml version="1.0" encoding="UTF-8"?>\n')
       outfile.write('<toc label="Messages">\n')
       outfile.write('  <topic label="CWLNA">\n')
       
       msgidlist = []

       for child in inputxmlroot:
           if child.tag == "Message":
               msgnode = child
               msgid = None

               if msgnode.attrib['ID']:
                   msgid = msgnode.attrib['ID']

               if msgid:
                   msgidlist.append(msgid)
               else:
                   logger.error("Malformed message")
            
       #Sort the list numerically
       msgidlist.sort(key=idsorter)
       
       for messageid in msgidlist:
           msgpath = os.path.join(outbasedir, messageid+".html")
           absmsgpath = os.path.abspath(msgpath)
           outfile.write('    <topic label="'+messageid+'" href="'+absmsgpath+'" />\n')
       
       outfile.write('  </topic>\n')
       outfile.write('</toc>\n\n')
       outfile.close()
                   

    except Exception as e:
       logger.error("Failed to parse data for TOC file - error %s" %
                         (e))
       raise e 
