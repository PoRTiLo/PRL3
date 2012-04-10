#!/bin/bash
if [ $# -lt 1 ];then 					#pocet kroku musi byt zadan, jinak ukoncim
    echo "Zadejte pocet kroku"
    exit
else
    step=$1;                                                                           # pocet kroku
fi;

row=`wc -l < lattice`
row=$((--row))
column=`wc -L < lattice`
mpicc --prefix /usr/local/share/OpenMPI -o ca ca.c 				                        #preklad c zdrojaku
mpirun --prefix /usr/local/share/OpenMPI -np $row ./ca $step $column | sort -nk1			#spusteni
rm -f ca       					                                                         #uklid
