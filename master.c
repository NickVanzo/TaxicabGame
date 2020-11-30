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

/*NOTA PER MARCO: TOGLIERE IL PTR ALLA STRUTTURA MAP_CELL E METTERE UN PTR ALLA SHARED MEMORY! ora così non funziona ma l'algoritmo di base funge*/
void stampaStatistiche(map_cell *mappa[], int righeMappa, int colonneMappa, int *statistiche){
    int i,j,k;
    char *stats[] = {
        " | Number of successfoul rides: ",
        " | Number of unsuccessfoul rides: ",
        " | Number of aborted rides: ",
        " | Cumulative longest driving taxi: ",
        " | Cumulative farthest driving taxi: ",
        " | Taxi with most succesfoul rides: "
    };

    char *strTmp;

    for(i=0; i<righeMappa; i++){
        for(j=0;j<colonneMappa;j++){

                if((&mappa[i][j])->cellType == ROAD){
                    sprintf(strTmp, " %d ", (&mappa[i][j])->taxiOnThisCell );
                    colorPrintf(strTmp, BLACK, WHITE);
                }else if((&mappa[i][j])->cellType == SOURCE){
                    sprintf(strTmp, " %d ", (&mappa[i][j])->taxiOnThisCell );
                    colorPrintf(strTmp, BLACK, MAGENTA);
                }else{
                    colorPrintf("   ", BLACK, BLACK);
                }
                
        }
         if(i<6) printf("%s\n", stats[i]);
         else printf("\n");
    }

    /*controllo che ho stampato tutti gli stats*/
    if(righeMappa < 6){
        for(i=colonneMappa; i<6;i++){
            for(j=0;j<colonneMappa; j++) printf("   "); /*mi allineo alla fine*/
            printf("%s\n", stats[i]);
        }
    }
}






int main(){

   
    
   
    
    return 0;
}



