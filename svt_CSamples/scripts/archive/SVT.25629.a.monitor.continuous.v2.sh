# note then you can gather connection data with:  
#       grep "Connection Time" log.messages  |grep log.s | sort -u | sort --key=13n
# note to only check final results grep for :1: which means result_available was set to 1
#       grep "Connection Time :1:" log.messages  |grep log.s | sort -u | sort --key=13n
#

while((1)); do ./RTC.25629.a.monitor.c.client.v2.sh | tee -a log.messages ; done

