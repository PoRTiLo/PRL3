/**
 *       @file  ca.c
 *      @brief  Pomoci knihovny Open MPI je implementovan celularni automat, který vyuziva
 *      paralelniho prostredi pro urychleni vypoctu. Celularni automat implementuje 
 *      pravidla hry Game of life.
 *
 * Detailed description starts here.
 *
 *     @author  Bc. Jaroslav Sendler (xsendl00), xsendl00@stud.fit.vutbr.cz
 *
 *   @internal
 *     Created  04/11/2012
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
#include <string.h>


#define TAG 0
#define DEAD 48                                                   /* nula v ASCII hodnote */
#define LIVE 49                                                   /* jednicka v ASCII hodnote */

/**
 * @brief   Zmena stavu bunky. Plati nasledujici pravidla:
 * Kazda ziva bunka s mene nez dvema zivymi sousedy umira. 
 * Kazda ziva bunka se dvema nebo tremi zivymi sousedy zustava zit. 
 * Kazda ziva bunka s vice nez tremi zivymi sousedy umira. 
 * Kazda mrtva bunka s prave tremi zivymi sousedy oziva.
 * @param   cell aktualni stav bunky
 * @param   live pocet zivych okolnich bunek
 * @return  vysledny stav bunky
 */
char liveDead(char cell, int live) {
   if(cell == LIVE) {                                             /* bunka je ziva */
      if(live < 2 || live > 3) {                                  /* v okoli je vice jak 3 zive bunky nebo mene nez 2 */
         cell = DEAD;                                             /* bunka umira */
      }
   }
   else {                                                         /* mrtva bunka */
      if(live == 3) {                                             /* v okoli jsou tri zive bunky */
         cell = LIVE;                                             /* bunka oziva */
      }
   }
   return cell;
}

int main(int argc, char *argv[]) {
   double startT, endT;
   startT = MPI_Wtime();
   int numprocs;                                                  /* pocet procesoru */
   int myid;                                                      /* muj rank */
   unsigned int column;                                           /* pocet sloupcu */
   int step;                                                      /* pocet kroku provadenych v algoritmu */
   char *row;                                                     /* jeden radek hraci desky */
   char *rowNew;                                                  /* nove vytvoreny radek, pomocne pole */
   char *rowDown;                                                 /* obsah spodniho radku */
   char *rowUp;                                                   /* obsah vrchniho radku */
   size_t result;                                                 /* pomocna hodnota, obsah navratu funkci */
   int lastId;
   MPI_Status stat;                                               /* struct- obsahuje kod- source, tag, error */

   /* MPI INIT */
   MPI_Init(&argc,&argv);                                         /* inicializace MPI */
   MPI_Comm_size(MPI_COMM_WORLD, &numprocs);                      /* pocet bezicich procesu */
   lastId = numprocs -1;
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
   if((row = (char *) malloc(sizeof(row) * column)) == NULL ) {
      perror("CHYBA: Nepovedla se alokace pole o velikosti jednoho radku.");
      fclose(file);
      return -1;
   }

   /* Alokace pole o velikosti jednoho radku - pro zalohu noveho radku */
   if((rowNew = (char *) malloc(sizeof(rowNew) * column)) == NULL ) {
      perror("CHYBA: Nepovedla se alokace pole o velikosti jednoho radku.");
      fclose(file);
      return -1;
   }

   /* Alokace pole o velikosti jednoho radku - pro vrchni radek */
   if(myid != 0) {
      if((rowUp = (char *) malloc(sizeof(rowUp) * column)) == NULL ) {
         perror("CHYBA: Nepovedla se alokace pole o velikosti jednoho radku.");
         fclose(file);
         return -1;
      }
   }

   /* Alokace pole o velikosti jednoho radku - pro spodni radek radek */
   if(myid != lastId) {
      if((rowDown = (char *) malloc(sizeof(rowDown) * column)) == NULL ) {
         perror("CHYBA: Nepovedla se alokace pole o velikosti jednoho radku.");
         fclose(file);
         return -1;
      }
   }

   /* Vymazani alokavona pameti */
   unsigned int i;
/*   memset(row, '\0', column);
   memset(rowNew, '\0', column);
   if (myid != 0)
      memset(rowUp, '\0', column);
   if(myid != lastId)
      memset(rowDown, '\0', column);
      */
   for(i = 0; i <= column; i++) {
      row[i] = '\0';
      rowNew[i] = '\0';
      if(myid != lastId) {
         rowDown[i] = '\0';
      }
      if (myid != 0) {
         rowUp[i] = '\0';
      }
   }

   /* Nastaveni spravne pozice v souboru */
   if(fseek(file, myid*column+myid, SEEK_SET) != 0) {             /* posun kurzoru v souboru na zacatek radku */
      perror("CHYBA: Nepovedla se operace pri pohybu v souboru.");
      fclose(file);
      free(row);
      MPI_Finalize();
      return errno;
   }

   /* Cteni dat - cte se jen jeden radek */
   result = fread(row, 1, column, file);
   if(result != column) {
      perror("CHYBA: Nepovedlo se nacist cely radek ze souboru.");
      fclose(file);
      free(file);
      MPI_Finalize();
      return -1;
   }

   if(fclose(file) == EOF) {                                      /* uzavreni souboru */
      perror("CHYBA: Nepovedlo se korektne uzavrit soubor.");
   }

   /* Algortimus hry Game Of Life */
   int actStep;                                                   /* aktualni krok algoritmu */
   int live = 0;                                                  /* pocet zivych bunek v okoli */
   for(actStep = 0; actStep < step; actStep++) {                  /* provadim tolik kroku, kolik bylo zadano na vstupu */
      if(myid == 0) {                                             /* prvni radek, rozhoduje se jen podle spodniho */
         MPI_Send(row, column, MPI_CHAR, myid+1, TAG, MPI_COMM_WORLD);              /* posilam pole dolu */
/* printf("id(%d)->(%d):%s\n", myid, myid+1, row); */
         MPI_Recv(rowDown, column, MPI_CHAR, myid+1, TAG, MPI_COMM_WORLD, &stat);   /* prijimam pole ze spodu */
/* printf("id(%d)<-(%d):%s\n", myid, myid+1, rowDown); */
         /* reseni pro prvni bunku, prvniho radku*/
         if(row[1] == LIVE) live++;                               /* prava bunak je ziva */
         if(rowDown[1] == LIVE) live++;                           /* prava spodni bunka je ziva */
         if(rowDown[0] == LIVE) live++;                           /* spodni bunka je ziva */
         rowNew[0] = liveDead(row[0], live);                      /* vypocet noveho stavu bunky */
         live = 0;
         /* reseni pro stred, krome prvni a psledni bunky */
         for(i = 1; i < column-1; i++) {
            if(row[i+1] == LIVE) live++;
            if(rowDown[i+1] == LIVE) live++;
            if(rowDown[i] == LIVE) live++;
            if(rowDown[i-1] == LIVE) live++;
            if(row[i-1] == LIVE) live++;
            rowNew[i] = liveDead(row[i], live);
            live = 0;
         }
         /* reseni pro posledni bunku, prvniho radku*/
         if(row[column-2] == LIVE) live++;                        /* leva bunka je ziva */
         if(rowDown[column-2] == LIVE) live++;                    /* leva spodni bunka je ziva */
         if(rowDown[column-1] == LIVE) live++;                    /* spodni bunka je ziva */
         rowNew[column-1] = liveDead(row[column-1], live);        /* vypocet neveho stavu bunky */
      }
      else if(myid == lastId) {                                 /* posledni radek, roshoduje se jen podle predchoziho */
         MPI_Send(row, column, MPI_CHAR, myid-1, TAG, MPI_COMM_WORLD);              /* posilam pole nahoru */
/* printf("id(%d)->(%d):%s\n", myid, myid-1, row); */
         MPI_Recv(rowUp, column, MPI_CHAR, myid-1, TAG, MPI_COMM_WORLD, &stat);     /* prijimam pole z vrchu */
/* printf("id(%d)<-(%d):%s\n", myid, myid-1, rowUp); */
         /* reseni pro prvni bunku, prvniho radku*/
         if(row[1] == LIVE) live++;                               /* prava bunak je ziva */
         if(rowUp[1] == LIVE) live++;                             /* prava vrchni bunka je ziva */
         if(rowUp[0] == LIVE) live++;                             /* vrchni bunka je ziva */
         rowNew[0] = liveDead(row[0], live);                      /* vypocet noveho stavu bunky */
         live = 0;
         /* reseni pro stred, krome prvni a psledni bunky */
         for(i = 1; i < column-1; i++) {
            if(row[i+1] == LIVE) live++;                          /* prava ziva bunka */
            if(rowUp[i+1] == LIVE) live++;                        /* prava ziva vrchni bunka */
            if(rowUp[i] == LIVE) live++;                          /* vrchni ziva bunka */
            if(rowUp[i-1] == LIVE) live++;                        /* vrchni leva bunka */
            if(row[i-1] == LIVE) live++;                          /* leva ziva bunka */
            rowNew[i] = liveDead(row[i], live);
            live = 0;
         }
         /* reseni pro posledni bunku, prvniho radku*/
         if(row[column-2] == LIVE) live++;                        /* leva bunka je ziva */
         if(rowUp[column-2] == LIVE) live++;                      /* leva vrchni bunka je ziva */
         if(rowUp[column-1] == LIVE) live++;                      /* vrchni bunka je ziva */
         rowNew[column-1] = liveDead(row[column-1], live);        /* vypocet neveho stavu bunky */
      }
      else {                                                      /* zbyle radky, maji vrchniho i spodniho souseda */
         MPI_Send(row, column, MPI_CHAR, myid+1, TAG, MPI_COMM_WORLD);              /* posilam pole dolu */
/* printf("id(%d)->(%d):%s\n", myid, myid+1, row); */
         MPI_Send(row, column, MPI_CHAR, myid-1, TAG, MPI_COMM_WORLD);              /* posilam pole nahoru */
/* printf("id(%d)->(%d):%s\n", myid, myid-1, row); */
         MPI_Recv(rowUp, column, MPI_CHAR, myid+1, TAG, MPI_COMM_WORLD, &stat);     /* prijimam pole ze spodu */
/* printf("id(%d)<-(%d):%s\n", myid, myid+1, rowUp); */
         MPI_Recv(rowDown, column, MPI_CHAR, myid-1, TAG, MPI_COMM_WORLD, &stat);   /* prijimam pole z vrchu */
/* printf("id(%d)<-(%d):%s\n", myid, myid-1, rowDown); */
         /* prvni bunka - jen prave 5 okoli */
         if(rowUp[0] == LIVE) live++;                             /* vrchni bunka je ziva */
         if(rowUp[1] == LIVE) live++;                             /* prava vrchni bunak je ziva */
         if(row[1] == LIVE) live++;                               /* prava bunak je ziva */
         if(rowDown[1] == LIVE) live++;                           /* prava spodni bunka je ziva */
         if(rowDown[0] == LIVE) live++;                           /* spodni bunka je ziva */
         rowNew[0] = liveDead(row[0], live);                      /* vypocet noveho stavu bunky */
         live = 0;
         /* reseni pro stred, krome prvni a psledni bunky */
         for(i = 1; i < column-1; i++) {
            if(row[i+1] == LIVE) live++;                          /* prava ziva bunka */
            if(rowUp[i+1] == LIVE) live++;                        /* prava ziva vrchni bunka */
            if(rowUp[i] == LIVE) live++;                          /* vrchni ziva bunka */
            if(rowUp[i-1] == LIVE) live++;                        /* vrchni leva bunka */
            if(row[i-1] == LIVE) live++;                          /* leva ziva bunka */
            if(rowDown[i-1] == LIVE) live++;                      /* leva spodni ziva bunka */
            if(rowDown[i] == LIVE) live++;                        /* spodni ziva bunka */
            if(rowDown[i+1] == LIVE) live++;                      /* prava spodni ziva bunka */
            rowNew[i] = liveDead(row[i], live);
            live = 0;
         }
         /* posledni bunka - jen leve 5 okoli */
         if(rowUp[column-1] == LIVE) live++;                      /* vrchni bunka je ziva */
         if(rowUp[column-2] == LIVE) live++;                      /* leva vrchni bunka je ziva */
         if(row[column-2] == LIVE) live++;                        /* leva je ziva */
         if(rowDown[column-2] == LIVE) live++;                    /* leva spodni bunka je ziva */
         if(rowDown[column-1] == LIVE) live++;                    /* spodni bunka je ziva */
         rowNew[column-1] = liveDead(row[column-1], live);        /* vypocet neveho stavu bunky */
      } 
      live = 0;
      memcpy(row, rowNew, column);
   }

   /* Tisk hodnot po danem kroku na vystup */
   /*printf("%d:%s\n", myid, row);*/

   /* uklizeni po sobe */
   free(row);                                                     /* uvolenni dynamicke pameti */
   free(rowNew);                                                  /* uvolenni dynamicke pameti */
   if(myid != 0) {
      free(rowUp);                                                /* uvolenni dynamicke pameti */
   }
   if(myid != lastId) {
      free(rowDown);                                              /* uvolenni dynamicke pameti */
   }
   endT = MPI_Wtime();
   printf("(%d)start=%f, end=%f,  result=%f\n",myid, startT, endT, endT-startT);
   MPI_Finalize(); 
   return 0;

 }

