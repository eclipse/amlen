#!/bin/bash 
#
if [ $# != 0 ]; then
   IN_FILE=$1
   RECV_M_FILE="${IN_FILE}.Received_m.log"
   RECV_D_FILE="${IN_FILE}.Received_D.log"
   RECV_R_FILE="${IN_FILE}.Received_R.log"

   OUT_FILE="${IN_FILE}.sorted"
   RECV_M_SORT="${RECV_M_FILE}.sorted"
   RECV_D_SORT="${RECV_D_FILE}.sorted"
   RECV_R_SORT="${RECV_R_FILE}.sorted"

#  Received message
   `grep "Received m" ${IN_FILE} > ${RECV_M_FILE}  `
#  Received Duplicate
   `grep "Received D" ${IN_FILE} > ${RECV_D_FILE}  `
#  Received Redelivered
   `grep "Received R" ${IN_FILE} > ${RECV_R_FILE}  `

   awk '{print $(NF-1),$0}' ${IN_FILE}     | sort -n | cut -f2- -d' '    > ${OUT_FILE}
   awk '{print $(NF-1),$0}' ${RECV_M_FILE} | sort -n | cut -f2- -d' '    > ${RECV_M_SORT}
   awk '{print $(NF-1),$0}' ${RECV_D_FILE} | sort -n | cut -f2- -d' '    > ${RECV_D_SORT}
   awk '{print $(NF-1),$0}' ${RECV_R_FILE} | sort -n | cut -f2- -d' '    > ${RECV_R_SORT}
   
#   `awk -F '\:' '{print $2}' ${APIKEY_FILE} | uniq | wc -l`
    m_count=`awk '{print}' ${RECV_M_SORT} | wc -l`
    m_unique=`awk -F ' ' '{print $12}' ${RECV_M_SORT} | uniq | wc -l`
    m_dups=`  awk -F ' ' '{print $12}' ${RECV_M_SORT} | uniq -cd  | wc -l`

    d_count=`awk '{print}' ${RECV_D_SORT} | wc -l`
    d_unique=`awk -F ' ' '{print $12}' ${RECV_D_SORT} | uniq | wc -l`
    d_dups=`  awk -F ' ' '{print $12}' ${RECV_D_SORT} | uniq -cd  | wc -l`

    r_count=`awk '{print}' ${RECV_R_SORT} | wc -l`
    r_unique=`awk -F ' ' '{print $12}' ${RECV_R_SORT} | uniq | wc -l`
    r_dups=`  awk -F ' ' '{print $12}' ${RECV_R_SORT} | uniq -cd  | wc -l`
   
    echo "The File, ${RECV_M_SORT} and has  ${m_count}  lines of which  ${m_unique}  are unique and  ${m_dups}  are not."    
    echo "The File, ${RECV_D_SORT} and has  ${d_count}  lines of which  ${d_unique}  are unique and  ${m_dups}  are not."
    echo "The File, ${RECV_R_SORT} and has  ${r_count}  lines of which  ${r_unique}  are unique and  ${m_dups}  are not."

   if [ "${m_dups}" != "0" ]; then
      echo "Want to see the duplicates "Received Messages"? (Y|n)"
      read y 
      if [ "${y}" = "Y" ]; then
         awk -F ' ' '{print $12}' ${RECV_M_SORT} | uniq -cd
      fi
    fi

   if [ "${d_dups}" != "0" ]; then
      echo "Want to see the duplicates for "Received Duplicate Messages"? (Y|n)"
      read y 
      if [ "${y}" = "Y" ]; then
         awk -F ' ' '{print $12}' ${RECV_D_SORT} | uniq -cd
      fi
    fi

   if [ "${r_dups}" != "0" ]; then
      echo "Want to see the duplicates "Redelivered Messages"? (Y|n)"
      read y 
      if [ "${y}" = "Y" ]; then
         awk -F ' ' '{print $12}' ${RECV_R_SORT} | uniq -cd
      fi
    fi

else
   echo "You forgot to say what logfile !.  RETRY AGAIN with a logfile as param #1"
fi
