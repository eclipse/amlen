while((1)); do 
    nc=`netstat -tnp |wc -l`; 
    mc=`ps -ef | grep mqttsample_array |wc -l`; 
    echo "`date` $nc $mc"; 
    sleep 1; 
done

