    { while read line ; do echo "$line" ; done ; } |
    awk '
    NR == 1 {sum=0; max=$1 ; min=$1} 
    $1 >= max {max = $1} 
    $1 <= min {min = $1} 
    { sum+=$1; array[NR]=$1; } 
    
    END {
    for(x=1;x<=NR;x++){
        sumsq+=((array[x]-(sum/NR))^2);
    }
    if (NR>0){
        print "Min: "min," Max: "max " Sum: "sum, " Number of lines: "NR, " Average: "sum/NR " Standard Deviation: "sqrt(sumsq/NR)
    } else {
        print "No data. "
    }
    
    }' ;
