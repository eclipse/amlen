#echo `echo \"ls -al /var/mqm/trace/*.TRC \" | ssh ${A1_USER}@${A1_HOST} busybox`
echo \\"ls -al /var/mqm/trace/ \\" | ssh ${A1_USER}@${A1_HOST} busybox
