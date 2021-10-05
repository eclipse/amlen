#!/bin/bash 

for i in $(eval echo {1..${M_COUNT}}); do
    HOST=$(eval echo \$\{M${i}_HOST\})

    if [ "${HOST}" == "10.10.177.17" ]; then
      eval M${i}_COUNT=$((10*500))
      MYCOUNT[$i]=6000
      MYBATCH[$i]=1000
    elif [ "${HOST}" == "10.10.179.131" ]; then
      eval M${i}_COUNT=$((18*500))
      MYCOUNT[$i]=7000
      MYBATCH[$i]=1000
    elif [ "${HOST}" == "10.10.179.77" ]; then
      eval M${i}_COUNT=$((16*500))
      MYCOUNT[$i]=8000
      MYBATCH[$i]=1000
    elif [ "${HOST}" == "10.10.179.78" ]; then
      eval M${i}_COUNT=$((30*500))
      MYCOUNT[$i]=17000
      MYBATCH[$i]=1000
    elif [ "${HOST}" == "10.10.179.176" ]; then
      eval M${i}_COUNT=$((16*500))
      MYCOUNT[$i]=7000
      MYBATCH[$i]=1000
    elif [ "${HOST}" == "10.10.177.161" ]; then
      eval M${i}_COUNT=$((23*500))
      MYCOUNT[$i]=9000
      MYBATCH[$i]=1000
    elif [ "${HOST}" == "10.10.177.162" ]; then
      eval M${i}_COUNT=$((23*500))
      MYCOUNT[$i]=9000
      MYBATCH[$i]=1000
    elif [ "${HOST}" == "10.10.177.163" ]; then
      eval M${i}_COUNT=$((24*500))
      MYCOUNT[$i]=9000
      MYBATCH[$i]=1000
    elif [ "${HOST}" == "10.10.179.51" ]; then
      eval M${i}_COUNT=$((6*500))
      MYCOUNT[$i]=2000
      MYBATCH[$i]=500
    elif [ "${HOST}" == "10.10.1710.108" ]; then
      eval M${i}_COUNT=$((4*500))
      MYCOUNT[$i]=2000
      MYBATCH[$i]=1000
    elif [ "${HOST}" == "10.10.179.227" ]; then
      eval M${i}_COUNT=$((5*500))
      MYBATCH[$i]=1000
      MYCOUNT[$i]=4000
    elif [ "${HOST}" == "10.10.177.78" ]; then
      eval M${i}_COUNT=$((7*500))
      MYCOUNT[$i]=3000
      MYBATCH[$i]=1000
    elif [ "${HOST}" == "10.10.179.144" ]; then
      eval M${i}_COUNT=$((5*500))
      MYCOUNT[$i]=3000
      MYBATCH[$i]=1000
    elif [ "${HOST}" == "10.10.179.195" ]; then
      eval M${i}_COUNT=$((5*500))
      MYCOUNT[$i]=2000
      MYBATCH[$i]=500
    elif [ "${HOST}" == "10.10.177.73" ]; then
      eval M${i}_COUNT=$((5*500))
      MYCOUNT[$i]=3000
      MYBATCH[$i]=1000
    elif [ "${HOST}" == "10.10.177.76" ]; then
      eval M${i}_COUNT=$((5*500))
      MYCOUNT[$i]=2500
      MYBATCH[$i]=500
    elif [ "${HOST}" == "10.10.179.146" ]; then
      eval M${i}_COUNT=$((4*500))
      MYCOUNT[$i]=2500
      MYBATCH[$i]=500
    elif [ "${HOST}" == "10.10.177.59" ]; then
      eval M${i}_COUNT=$((4*500))
      MYCOUNT[$i]=2000
      MYBATCH[$i]=500
    elif [ "${HOST}" == "10.10.177.103" ]; then
      MYCOUNT[$i]=6000
      MYBATCH[$i]=1000
    elif [ "${HOST}" == "10.10.177.104" ]; then
      MYCOUNT[$i]=6000
      MYBATCH[$i]=1000
    elif [ "${HOST}" == "10.10.177.105" ]; then
      MYCOUNT[$i]=6000
      MYBATCH[$i]=1000
    elif [ "${HOST}" == "10.10.177.106" ]; then
      MYCOUNT[$i]=6000
      MYBATCH[$i]=1000
    elif [ "${HOST}" == "10.10.177.107" ]; then
      MYCOUNT[$i]=6000
      MYBATCH[$i]=1000
    elif [ "${HOST}" == "10.10.177.108" ]; then
      MYCOUNT[$i]=6000
      MYBATCH[$i]=1000
    else
      eval M${i}_COUNT=$((4*500))
      MYCOUNT[$i]=2000
      MYBATCH[$i]=500
    fi
    MYCOUNT[$i]=2000
#   MYBATCH[$i]=500
done

for i in $(eval echo {2..${M_COUNT}}); do
  CLIENTS=$(eval echo \$\{MYCOUNT[$i]\})
  BATCH=$(eval echo \$\{MYBATCH[$i]\})

  DIV[$i]=$((${CLIENTS}/${BATCH}))
  REM[$i]=$((${CLIENTS}%${BATCH}))

  TOTAL=$((${TOTAL}+(${CLIENTS}/${BATCH})))
  if [ $(($CLIENTS%${BATCH})) -gt 0 ]; then
     TOTAL=$((${TOTAL}+1))
  fi
done


declare TWHERE=2
i=1
while [ $i -le ${TOTAL} ]; do
  if [ $(eval echo \$\{DIV[${TWHERE}]\}) -gt 0 ]; then
    HOWMANY[$i]=${MYBATCH[$TWHERE]}
    WHERE[$i]=m${TWHERE}
    DIV[$TWHERE]=$(($(eval echo \$\{DIV[$TWHERE]\})-1))
    ((i++))
  elif [ $(eval echo \$\{REM[${TWHERE}]\}) -gt 0 ]; then
    HOWMANY[$i]=${REM[$TWHERE]}
    WHERE[$i]=m${TWHERE}
    REM[$TWHERE]=0
    ((i++))
  fi

  TWHERE=$((${TWHERE}+1))
  if [ ${TWHERE} -gt ${M_COUNT} ]; then
     TWHERE=2
  fi
done


# for i in $(eval echo {1..${TOTAL}}); do
#     echo ${HOWMANY[$i]} to run on ${WHERE[$i]}
# done



