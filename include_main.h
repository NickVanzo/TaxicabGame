#ifndef INCLUDE_STDIO
    #define INCLUDE_STDIO
    #include <stdio.h>
#endif

#ifndef INCLUDE_STDLIB
    #define INCLUDE_STDLIB
    #include <stdlib.h>
#endif

#ifndef INCLUDE_TIME
    #define INCLUDE_TIME
    #include <time.h>
#endif

#ifndef INCLUDE_STRINGS
    #define INCLUDE_STRINGS
    #include <string.h>
#endif


#ifndef INCLUDE_COLORS
    #define INCLUDE_COLORS
    #include "./include_files/colors/colors.h"
#endif

#ifndef INCLUDE_UNISTD
    #define INCLUDE_UNISTD
    #include <unistd.h>
#endif

#ifndef INCLUDE_SIGNAL
    #define INCLUDE_SIGNAL
    #include <signal.h>
#endif


#ifndef INCLUDE_TYPES
    #define INCLUDE_TYPES
    #include <sys/types.h>
#endif

#ifndef INCLUDE_IPC
    #define INCLUDE_IPC
    #include <sys/ipc.h>
#endif

#ifndef INCLUDE_SEM
    #define INCLUDE_SEM
    #include <sys/sem.h>
#endif

#ifndef INCLUDE_LIMITS
    #define INCLUDE_LIMITS
    #include <limits.h>
#endif

#ifndef INCLUDE_ERRNO
    #define INCLUDE_ERRNO
    #include <errno.h>
#endif

#ifndef INCLUDE_SHM
    #define INCLUDE_SHM
    #include <sys/shm.h>
#endif

/*messa solo per potere fare del debug o comunque non avere errori dall'ide*/
#ifndef INCLUDE_TIME
    #define INCLUDE_TIME
    #include <time.h>
#endif


#ifndef DEFINES
    #define DEFINES

    /*dimensione della mappa del mondo*/
    #define SO_WIDTH 20 /*righe*/
    #define SO_HEIGHT 10 /*colonne*/

#endif


/*POICHE QUESTO HEADER FA RIFERIMENTO A ALTRI TIPI DI DATO DI TIPO STANDARD, DEVE ESSERE INCLUSO PER ULTIMO! (TIPO IL TIME NELLA STRUCT DELLA CELLA!*/
#ifndef INCLUDE_CUSTOM_DATA_TYPES
    #define INCLUDE_CUSTOM_DATA_TYPES
    #include "./include_files/custom_data_types/custom_data_types.h" 
#endif




/*tipo di dato booleano*/
typedef enum{FALSE, TRUE} boolean;


/*
    VARIABILI PER IL COLORE DELLO SFONDO E DEL TESTO
*/

enum color {DEFAULT, BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, GRAY, WHITE};


/*
    VARIABILI DELLA MAPPA
*7


/*tipo di dato della singola cella. il block corrisponde alla cella nera, ma non so come la vogliamo chiamare per cui la lascio block*/
typedef enum{ROAD, BLOCK, SOURCE} cell_type;


/*cella della mappa*/
typedef struct{
    cell_type cellType; /*indica il tipo di cella che sto creando */
    int availableSpace; /*spazio disponibile di taxi nella cella. è una variabile di tipo semaforo*/
    int taxiOnThisCell; /*numero intero che indica il numero di taxi presenti nella cella*/
    int availableForHoles; /*dice se una determinata cella che voglio trasformare in buco può essere trasformata*/
    int totalNumberOfTaxiPassedHere; /*indica il numero totale di taxi passati su questa cella*/
    time_t timeRequiredToCrossCell; /*indica il tempo richiesto in ms per attraversare la cella*/
    boolean isInTopSoCell; /*questo valore mi dice se la mia cella è nelle SO_TOP_CELL per traffico passato da qua da stampare a fine simulazione*/
} map_cell;

/*struttura per invvio messaggi contentente richiesta di corsa a un taxi*/
struct msgBuf{
    long mtype; /*se mtype = 0 kill di chi lo legge, se mtype = 1...n richiedere corsa a source 1...n*/ 
    int xDest; 
    int yDest;
};

struct grigliaCitta{
    map_cell matrice[SO_HEIGHT][SO_WIDTH];
}; 

#ifndef DEFINE_CUSTOM_FUNCTIONS
    #define DEFINE_CUSTOM_FUNCTIONS
    /*qua dentro definiamo tutti i prototipi delle funzioni che andiamo a scrivere*/

    /*
        FUNZIONI UI
    
    */
    void colorPrintf(char *message, enum color colore, enum color bgcolore);

    int pointToID(struct grigliaCitta* mappa, int x, int y);



#endif