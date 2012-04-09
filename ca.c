/**
 *       @file  ca.cpp
 *      @brief  TODO
 *
 * Detailed description starts here.
 *
 *     @author  Bc. Jaroslav Sendler (xsendl00), xsendl00@stud.fit.vutbr.cz
 *
 *   @internal
 *     Created  04/09/2012
 *     Company  FIT-VUT, Brno
 *   Copyright  Copyright (c) 2012, Bc. Jaroslav Sendler
 *
 * =====================================================================================
 */

#include <mpi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>


#define TAG 0

int main(int argc, char *argv[]) {
   int numprocs;                                                  /* pocet procesoru */
   int myid;                                                      /* muj rank */
   int column;                                                    /* pocet sloupcu */
   int step;                                                      /* pocet kroku provadenych v algoritmu */
   char *row;                                                      /* jeden radek hraci desky */
   size_t result;                                                 /* pomocna hodnota, obsah navratu funkci */
   MPI_Status stat;                                               /* struct- obsahuje kod- source, tag, error */

   /* MPI INIT */
   MPI_Init(&argc,&argv);                                         /* inicializace MPI */
   MPI_Comm_size(MPI_COMM_WORLD, &numprocs);                      /* pocet bezicich procesu */
   MPI_Comm_rank(MPI_COMM_WORLD, &myid);                          /* id meho procesu */

   /* Nacteni argumentu*/
   step = strtol(argv[1], &argv[1], 10);                          /* prevod vstupniho argumentu na cislo */
   column = strtol(argv[2], &argv[2], 10);                        /* prevod vzstupniho argumentu na cislo */

   /* Otevreni souboru */
   char input[] = "lattice";
   FILE *file;
   if((file = fopen(input, "r")) == NULL ) {
      perror("CHYBA: Nepovedlo se otevrit soubor.");
      MPI_Finalize();
      return errno;
   }

   /* Alokace pole o velikosti jednoho radku */
   if((row = (char *) malloc(sizeof(char) * column)) == NULL ) {
      perror("CHYBA: Nepovedla se alokace pole o velikosti jednoho radku.");
      fclose(file);
      return -1;
   }

   /* Vymazani alokavona pameti */
   int i;
   for(i = 0; i <= column; i++) {
      row[i] = '\0';
   }

   /* Nastaveni spravne pozice v souboru */
   if(fseek(file, myid*column+myid, SEEK_SET) != 0) {                  /* posun kurzoru v souboru na zacatek radku */
      perror("CHYBA: Nepovedla se operace pri pohybu v souboru.");
      fclose(file);
      free(row);
      MPI_Finalize();
      return errno;
   }

   /* Cteni dat - cte se jen jeden radek */
   result = fread(row, 1, column, file);
   if(result != sizeof(row)) {
      printf("CHYBA: Nepovedlo se nacist cely radek ze souboru.");
      fclose(file);
      free(file);
      MPI_Finalize();
      return -1;
   }
   printf("(%d):%s\n", myid, row);




   free(row);                                                     /* uvolenni dynamicke pameti */
   if(fclose(file) == EOF) {                                      /* uzavreni souboru */
      perror("CHYBA: Nepovedlo se korektnr uzavrit soubor.");
   }
   MPI_Finalize(); 
   return 0;

 }//main
