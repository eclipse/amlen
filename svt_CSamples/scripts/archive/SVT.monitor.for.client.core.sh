echo core > /proc/sys/kernel/core_pattern 

while ((1)); do
     echo `date` checking; ls -tlra * | grep -E 'core.[0-9]+'
     if ls -tlra * |grep -E 'core.[0-9]+' ; then echo "`date`" |  mail -s "it looks like there is a core.[0-9]+ on client" someuser@some.company.com ; break; fi;
     sleep 60 ;
done

