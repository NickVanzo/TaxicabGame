#include "include_main.h"



/*
    funzione per la stampa in master delle statistiche della cella
    map_cell mappa[][] la mappa del gioco
    int ttigheMappa: il numero di righe nella mappa
    int colonneMappa, il numero di colonne nella mappa
    int statistiche[6]: array contenente le statistiche:
        0-> numero di corse completate con successo
        1-> numero di corse no completate
        2-> numero di corse abortite
        3-> pid processo con la somma di tempo di corsa maggiore tra tutte
        4-> pid processo che ha percorso più distanza
        5-> pid processo che ha servito più clienti
*/


void stampaStatistiche(map_cell **mappa, int righeMappa, int colonneMappa, int *statistiche){
    int i,j,k;
    char *stats[] = {
        " | Number of successfoul rides: ",
        " | Number of unsuccessfoul rides: ",
        " | Number of aborted rides: ",
        " | Cumulative longest driving taxi: ",
        " | Cumulative farthest driving taxi: ",
        " | Taxi with most succesfoul rides: "
    };

    char *strTmp = (char *)malloc(7);


    /*stampèo bordo superiore*/
    for(i=0;i<colonneMappa+2;i++){
        colorPrintf("       ", GRAY, GRAY);
    }
    printf("\n");
    for(i=0;i<colonneMappa+2;i++){
        colorPrintf("       ", GRAY, GRAY);
    }
    printf("\n");


    
    for(i=0; i<righeMappa; i++){
        colorPrintf("       ", GRAY, GRAY); /*stampo bordo laterale sx*/
        for(j=0;j<colonneMappa;j++){

                if((&mappa[i][j])->cellType == ROAD){
                    sprintf(strTmp, " %-5d ", (&mappa[i][j])->taxiOnThisCell );
                    colorPrintf(strTmp, BLACK, WHITE);
                }else if((&mappa[i][j])->cellType == SOURCE){
                    sprintf(strTmp, " %-5d ", (&mappa[i][j])->taxiOnThisCell );
                    colorPrintf(strTmp, BLACK, MAGENTA);
                }else{
                    sprintf(strTmp, " %-5d ", (&mappa[i][j])->taxiOnThisCell );
                    colorPrintf(strTmp, BLACK, BLACK);
                }
                
        }
         colorPrintf("       ", GRAY, GRAY); /*stampo bordo laterale dx*/
         if(i<6) printf("%s\n", stats[i]);
         else printf("\n");
    }


    /*stampo bordo inferiore*/
    for(i=0;i<colonneMappa+2;i++){
        colorPrintf("       ", GRAY, GRAY);
    }
    printf("\n");
    for(i=0;i<colonneMappa+2;i++){
        colorPrintf("       ", GRAY, GRAY);
    }
    printf("\n");

    /*controllo che ho stampato tutti gli stats*/
    if(righeMappa < 6){
        for(i=colonneMappa+2; i<6;i++){
            for(j=0;j<colonneMappa; j++) printf("   "); /*mi allineo alla fine*/
            printf("%s\n", stats[i]);
        }
    }
}






int main(){

    /*esmpio di codice di come allocare la matrice nello heap per poter passare la matrice qualsiasi dimensione all funzione di stampa!*/

    int i,j;
    int righe,colonne;
    map_cell **mappa;

    righe = 10;
    colonne = 10;

    mappa = (map_cell **) malloc(10*sizeof(map_cell*));


    for(i=0;i<righe;i++){
        mappa[i] = (map_cell *) malloc(10*sizeof(map_cell));
    }


   for(i=0;i<righe;i++){
       srand(i);
       for(j=0;j<righe;j++){
           (&mappa[i][j])->cellType = ((i+j)*(i*j) * rand())%3;
           (&mappa[i][j])->taxiOnThisCell = i+j;
       }
   }

   
      
   stampaStatistiche(mappa, righe, colonne, NULL);

   return 0;
}



