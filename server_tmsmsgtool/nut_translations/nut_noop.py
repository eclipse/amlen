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

import shutil
import datetime
import os.path

import nut_utils

def copyLangFiles(logger, langliststr, replace_filename_vars, input, output):

    langlist = nut_utils.parseLanguageLists(langliststr)

    for lang in langlist:
        for infile in input:
            if replace_filename_vars:
                infile = nut_utils.parseFileNameForLangVars(infile, lang)
                outfile = nut_utils.parseFileNameForLangVars(output, lang)
            else:
                outfile = output
    
            try:
                timestr = datetime.datetime.now().strftime("%H:%M:%S on %d %B %Y")
                os.makedirs(os.path.dirname(output), exist_ok=True)
                logger.info("Copy "+infile+" to output file: "+outfile+" at "+timestr)
                shutil.copyfile(infile, outfile)
            except Exception as e:
                logger.error("Failed to copy file: %s to %s - error %s" %
                             (infile, outfile, e))
                raise e 



