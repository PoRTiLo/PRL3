#!/bin/bash
if [ $# -lt 1 ];then 					#pocet kroku musi byt zadan, jinak ukoncim
    echo "Zadejte pocet kroku"
    exit
else
    step=$1;                                                                           # pocet kroku
fi;

if [ -e "lattice" ];then
   soubor=lattice
else
   echo "Soubor lattice neexsituje"
   exit

fi;
row=`wc -l < lattice`
row=$((--row))
column=`wc -L < lattice`
mpicc --prefix /usr/local/share/OpenMPI  -o ca ca.c 				                        #preklad c zdrojaku
mpirun --prefix /usr/local/share/OpenMPI -np $row ./ca $step $column | sort -nk1			#spusteni
#mpirun --prefix /usr/local/share/OpenMPI -np $row  valgrind --tool=callgrind ./ca $step $column | sort -nk1			#spusteni
#LD_PRELOAD=$VALGRIND_MPI_WRAPPER mpirun --prefix /usr/local/share/OpenMPI -np $row valgrind --log-file=valgrind.log ./ca $step $column | sort -nk1			#spusteni
#gprof -b ./ca
rm -f ca       					                                                         #uklid
