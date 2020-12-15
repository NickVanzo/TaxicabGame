#include "include_main.h"

/*Il prototipo della funzione e' contenuto nel file include_main.h */
/*
	Funzione che prende come parametri le coordinate di x e y e la mappa
	Ritorna il tipo di messaggio da ascoltare
*/


/*
	idea della funzione: scorro la mappa per sapere dove sono le sources. 
	Costruisco lo stesso identico array che uso per costruire i messaggi in source.c. L'array contiene le celle sources.
	Il numero ritornato e' l'indice dell'array a cui siamo arrivati. Dato che hanno lo stesso ordine nell'array hanno lo stesso tipo di messaggio
	Questa e' l'idea, verificare se funziona o meno. 
*/




void colorPrintf(char *message, enum color colore, enum color bgcolore){

     switch(bgcolore){

        case GRAY:
            printf("%s", BG_C_GRAY);
            break;

        case BLACK:
            printf("%s", BG_C_BLACK);
            break;

        case RED:
            printf("%s", BG_C_RED);
            break;

        case GREEN:
            printf("%s", BG_C_GREEN);
            break;
        
        case YELLOW:
            printf("%s", BG_C_YELLOW);
            break;
        
        case BLUE:
            printf("%s", BG_C_BLUE);
            break;
        
        case MAGENTA:
            printf("%s", BG_C_MAGENTA);
            break;

        case CYAN:
            printf("%s", BG_C_CYAN);
            break;

        case WHITE:
            printf("%s", BG_C_WHITE);
            break;

        
        default:
            break;
    }


    switch(colore){

        case GRAY:
            printf("%s", C_GRAY);
            break;

        case BLACK:
            printf("%s", C_BLACK);
            break;

        case RED:
            printf("%s", C_RED);
            break;

        case GREEN:
            printf("%s", C_GREEN);
            break;
        
        case YELLOW:
            printf("%s", C_YELLOW);
            break;
        
        case BLUE:
            printf("%s", C_BLUE);
            break;
        
        case MAGENTA:
            printf("%s", C_MAGENTA);
            break;

        case CYAN:
            printf("%s", C_CYAN);
            break;

        case WHITE:
            printf("%s", C_WHITE);
            break;

        
        default:
            break;
    }

    printf("%s%s%s", message, C_DEFAULT, BG_C_DEFAULT);
}


void P(int semaphore){
    struct sembuf sops;
    sops.sem_flg = 0;
    sops.sem_num = 0;
    sops.sem_op = -1;
    semop(semaphore, &sops, 1);
}


void V(int semaphore){
    struct sembuf sops;
    sops.sem_flg = 0;
    sops.sem_num = 0;
    sops.sem_op = 1;
    semop(semaphore, &sops, 1);
}