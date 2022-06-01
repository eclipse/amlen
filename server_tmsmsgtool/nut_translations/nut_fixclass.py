# Copyright (c) 2022 Contributors to the Eclipse Foundation
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

import glob
import datetime
import os.path
import re

import nut_parse

def fixClassName(logger, input):
    """Some of the translation java classes have a classname that doesn't include the
       language they relate to even though the filename does. This routine goes through
       the supplied files and changes the classname to match the file the class is in"""


    input_list = glob.glob(input, recursive=True)

    for inputfile in input_list:
        try:
            filenamewithoutext = os.path.splitext(os.path.basename(inputfile))[0]

            #The filename is of the format SomeClass_lang.java - we want just the "SomeClass bit"
            classNameWithoutLang=filenamewithoutext.rsplit('_',1)[0]

            if classNameWithoutLang != filenamewithoutext:
                #FileName is of the right format - let's make the change

                regex = re.compile(r"public class ([^\s]*)\s", re.MULTILINE)
                fixedstring="public class "+filenamewithoutext+" "

                with open(inputfile,'r') as file:
                    filedata = file.read()

                    (filedata, numchanges) = re.subn(regex, fixedstring, filedata)

                    if numchanges > 1:
                        logger.fatal("Aborting otherwise would have made TOO MANY changes in: "+inputfile)
                    if numchanges == 1:
                        logger.info("Changing Classname to "+filenamewithoutext+" in "+inputfile)
                    else:
                        logger.info("No classname found to change in "+inputfile)
                with open(inputfile,'w') as file:
                    file.write(filedata)
            else:
                timestr = datetime.datetime.now().strftime("%H:%M:%S on %d %B %Y")
                logger.info("Skipping fixing classname in "+inputfile+" as filename is not of expected format at "+timestr)

        except Exception as e:
            logger.error("Failed to correct classname in: %s - error %s" %
                             (inputfile, e))
            raise e 



