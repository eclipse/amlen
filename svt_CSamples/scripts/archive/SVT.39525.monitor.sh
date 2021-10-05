while ((1)); do
  sdata=`tac log.mdbmessages.0 | grep -E 'SUB' | head -1`
  pdata=`tac log.mdbmessages.0 | grep -E 'PUB' | head -1`
  scount=`echo $sdata  | grep -oE 'Msgs.*' | awk '{print $2}'`
  pcount=`echo $pdata  | grep -oE 'Msgs.*' | awk '{print $2}'`
  if  echo $scount | grep -E '[0-9]+' > /dev/null ; then
    if  echo $pcount | grep -E '[0-9]+' > /dev/null ; then
      if [ -n "$pcount" -a -n "$scount" ] ; then
        let delta=($pcount-$scount)
        if (($delta>1000)); then
            echo -n "E"
            echo "`date`" |  mail -s "it looks hung" someuser@some.company.com
        fi
      fi
    fi  
  fi
            echo -n "."
  sleep 60
done


