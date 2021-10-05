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

import nut_parse
    
def writeDITAFile(logger, language, outbasedir, msgid, category, msgtext, explanationtext, operatorresponsetext):
    outpath = os.path.join(outbasedir, msgid+'.dita')
    try:
       timestr = datetime.datetime.now().strftime("%H:%M:%S on %d %B %Y")

       outfile = open(outpath, "w", encoding="utf8")
       
       outfile.write('<?xml version="1.0" encoding="UTF-8"?>\n\n')

       outfile.write('<!DOCTYPE msg\n')
       outfile.write('  PUBLIC "-//IBM//DTD DITA Message Reference//EN" "msgRef.dtd">\n')
       outfile.write('<msg xml:lang="'+language+'" id="'+msgid+'">\n')
       outfile.write('   <msgId outputclass="msgId">\n      \n')
      
       outfile.write('      <msgNumber>'+msgid+'</msgNumber>\n   \n')
   
       outfile.write('   </msgId>\n')
       outfile.write('   <titlealts>\n      \n')
      
       outfile.write('      <searchtitle>'+numbersOnly(msgid)+'</searchtitle>\n   \n')
   
       outfile.write('   </titlealts>\n')
       outfile.write('   <msgText>'+msgtext+'</msgText>\n')
       outfile.write('   <msgBody>\n')
       outfile.write('      <msgExplanation>\n')
       outfile.write('\n      \n         ')
       outfile.write(explanationtext+'\n')

       outfile.write('   \n')
       outfile.write('      </msgExplanation>\n')
       outfile.write('      <msgUserResponse>\n')
      
       outfile.write('\n      \n         ')
       outfile.write(operatorresponsetext+'\n')

       outfile.write('   \n')
       outfile.write('      </msgUserResponse>\n')
       
       if category:
           outfile.write('      <msgOther>\n')
           outfile.write('      \n         <p>\n         \n            <b>Category: </b>')
           outfile.write(category)
           outfile.write('</p>\n   \n')
           outfile.write('      </msgOther>\n')
       else:
           outfile.write('      <msgOther/>\n')
       outfile.write('   </msgBody>\n')
       outfile.write('</msg>\n')

       outfile.close()

    except Exception as e:
       logger.error("Failed to write DITA file: %s - error %s" %
                         (outpath,  e))
       raise e 

def msgTextNodeToDITAString(xmlnode):
    '''The node can contain innertext and tags like <q />'''
    strwithtags = ET.tostring(xmlnode).decode()
    
    #Remove starting tag
    str1  = re.sub("^<[^>]*>",'',strwithtags)
    #Remove ending tag
    str2  = re.sub("<\s*/[^>]*>\s*\n?\s*$",'',str1)
    
    #Add a bunch of weird whitespace around <ul> etc.    
    str3  = re.sub("<ul>",'\n         \n            <ul>\n',str2)
    str4  = re.sub("<ol>",'<ol>\n',str3)
    str5  = re.sub('<li>','            \n               <li>', str4)
    str6  = re.sub("</li>",'</li>\n',str5)
    str7  = re.sub("</ul>\n",'         \n            </ul>\n\n      \n         ',str6)
    str8  = re.sub("</ol>\n",'         \n            </ol>\n\n      \n         ',str7)
    
    #Paragraph tags if they are selfclosing have no interior whitespace
    str9  = re.sub("<\s*p\s*/\s*>",'<p/>',str8)
    
    #Replace quote tags
    str10 = re.sub("<\s*q[\s/]*>",'"',str9)
    
    #And <br> tags disappear...
    str11 = re.sub("<\s*br[\s/]*>",'',str10)
    
    #Remove newline at start
    str12 = re.sub("^\n","",str11)
    
    #Replace &gt; (or &gt without semicolon) and &lt; with %%%%gt; and %%%%lt; so they are not affected
    #by the unescaping in the next step
    str13 = re.sub("&gt;?", "%%%%gt;", str12)
    str14 = re.sub("&lt;?", "%%%%lt;", str13)
    
    #Replace entities like accented characters etc for the real thing
    str15 = html.unescape(str14)
    
    #Put the < and > back to xml escaped versions
    str16 =  re.sub("%%%%gt;", "&gt;", str15)
    str17 =  re.sub("%%%%lt;", "&lt;", str16)

    return str17

def mkDITAPath(outbasedir):
    pathlib.Path(outbasedir).mkdir(parents=True, exist_ok=True)
    
def numbersOnly(msgid):
    return re.search(r'\d+',msgid).group()
    
def processSingleFileToDITA(logger, language, inputfile, inputxmlroot, outbasedir):
    timestr = datetime.datetime.now().strftime("%H:%M:%S on %d %B %Y")
    
    logger.info("Converting "+inputfile+" to DITA output files in: "+outbasedir+" at "+timestr)

    try:

       for child in inputxmlroot:
           if child.tag == "Message":
               msgnode = child
               msgid = None
               category = None
               msgtext = None
               explanationtext = None
               operatorresponsetext = None

               if 'ID' in msgnode.attrib:
                   msgid = msgnode.attrib['ID']
               
               if 'category' in msgnode.attrib:
                   category = msgnode.attrib['category']

               for mchild in msgnode:
                   if mchild.tag == "MsgText":
                       msgtextnode = mchild
                       msgtext = msgTextNodeToDITAString(msgtextnode)
                   elif mchild.tag == "Explanation":
                       explanationtext = msgTextNodeToDITAString(mchild)
                   elif mchild.tag == "OperatorResponse":
                       operatorresponsetext =  msgTextNodeToDITAString(mchild)
               
               if msgtext and msgid:
                   writeDITAFile(logger, language, outbasedir, msgid, category, msgtext, explanationtext, operatorresponsetext)
               else:
                   logger.error("Malformed message")
                   if msgid:
                       logger.error("Maformed message ID is "+msgid)
                   else:
                       logger.error("Maformed message ID is MISSING")
                   if msgtext:
                       logger.error("Maformed message text is "+msgtext)
                   else:
                       logger.error("Maformed message text is MISSING")

    except Exception as e:
       logger.error("Failed to parse data for DITA files - error %s" %
                         (e))
       raise e


def outputDITA(logger, language, inputfile, inputxmldir,  outbasedir):  
    mkDITAPath(outbasedir)
    
    if inputfile:
        inputxmlroot = nut_parse.parseFile(inputfile)
        processSingleFileToDITA(logger, language, inputfile, inputxmlroot, outbasedir)
    if inputxmldir:
        for (root, dirs, files) in os.walk(inputxmldir):
            for fname in files:
                filepath = os.path.join(root, fname)

                if filepath.endswith('.xml') :
                    filexmlroot = nut_parse.parseFile(filepath)
                    processSingleFileToDITA(logger, language, filepath, filexmlroot, outbasedir)
