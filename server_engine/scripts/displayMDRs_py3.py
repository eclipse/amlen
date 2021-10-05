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
import sys
import gdb

def displayMDRs(hMsgInfoExpr,outhdl=sys.stdout):
    
    hMsgInfo = gdb.parse_and_eval('(iecsMessageDeliveryInfoHandle_t)('+str(hMsgInfoExpr)+')')
    for chunkNum in range(0, hMsgInfo['MdrChunkCount']):
        if hMsgInfo['pChunk'][chunkNum]:
            print("Got chunk", chunkNum, file=outhdl)
            
            for slotNum in range (0, hMsgInfo['MdrChunkSize']):
                slot = hMsgInfo['pChunk'][chunkNum]['Slot'][slotNum]
                
                if slot['fSlotInUse']:
                    qHdl = slot['hQueue']
                    
                    multiConsumer = False
                    if qHdl:
                        if str(qHdl['QType']) == 'multiConsumer':
                            multiConsumer = True
                    
                    orderId = 0
                    
                    if multiConsumer:
                        node = gdb.parse_and_eval('(iemqQNode_t *)'+str(slot['hNode']))
                        orderId = node['orderId']
                    else:
                        node = gdb.parse_and_eval('(ieiqQNode_t *)'+str(slot['hNode']))
                        orderId = node['orderId']
                    print ( "Slot ", slotNum," DeliveryId ", slot['DeliveryId'], 'Q ',slot['hQueue'],'Node ',slot['hNode'], 'oId ', orderId,file=outhdl)
