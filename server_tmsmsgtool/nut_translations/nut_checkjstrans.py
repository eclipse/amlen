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

#
# This looks for missing javascript translation lines and if any are missing
# updates the english version to record that. The later dojobuild stage will
# then not error out complaining of missing translations 
#

import datetime
import glob
import os.path

def checkJSTrans(logger, input, translationrootdir):
    for inputarg in input:
        inputfiles = glob.glob(inputarg)

        for inf in inputfiles:
            for potentialdir in glob.glob(translationrootdir+'/*'):
                if os.path.isdir(potentialdir):
                    translationdir = potentialdir

                    #Does there exist a translated version of english file for that translation dir
                    translationpath = translationdir+'/'+os.path.basename(inf)

                    if not os.path.exists(translationpath):
                        logger.info('No version of '+inf+' exists for translation: '+translationdir)

                        with open(inf,'r') as file:
                            filedata = file.read()

                            translationname = os.path.basename(translationdir)
                            stringtofind = '"'+translationname+'": true'
                            replacementstring = '"'+translationname+'": false'
                            logger.debug('replacing: <'+stringtofind+'> with <'+replacementstring+'> in '+inf)
                            filedata = filedata.replace(stringtofind,replacementstring)
                        with open(inf,'w') as file:
                            file.write(filedata)





