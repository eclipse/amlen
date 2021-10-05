#!/usr/bin/env python
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

# This is a quick little script which can be used
# to determine which messages are missing (and show the gaps)
# (made for testmqtt_slow08, you'll need to tweak the numbers for your test)

import sys
import re

matrix = [[ False for i in xrange(0, 100) ] for j in xrange(0, 3)]
received = 0
for line in sys.stdin:
    if 'Received Binary message, topic=' in line:
        match = re.search('text of m([\d]+), # ([\d]+)', line)
        m_class = int(match.group(1))
        m_num = int(match.group(2))
        matrix[m_class][m_num-1] = 'c: %d, n: %d' % (m_class, m_num)
        received += 1

print 'Num received: %d' % received

for c in xrange(0, 3):
    for i in xrange(len(matrix[c])):
        if matrix[c][i] == False:
            print 'Missing m%d # %d' % (c, i)
