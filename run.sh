#!/bin/bash
function rand_usr(){
    echo $(cat /dev/urandom | head -n 10 | md5sum | head -c 10  )
}

function rand_num(){
    min=$1  
    max=$(($2-$min+1))  
    num=$(cat /proc/sys/kernel/random/uuid | cksum | awk -F ' ' '{print $1}')  
    echo $(($num%$max+$min)) 
}

for((i=0;i<$1;i++))
do 
	gnome-terminal -x ./auto $(rand_usr) $(rand_num $2 $3) $4 $5
	
wait
done

exit 0
