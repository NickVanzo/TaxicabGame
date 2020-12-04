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

#ifndef COLORS_SCHEME
    #define COLORS_SCHEME
    #define C_DEFAULT "\e[39m"
    #define C_BLACK "\e[30m"
    #define C_RED  "\e[31m"
    #define C_GREEN "\e[32m"
    #define C_YELLOW "\e[33m"
    #define C_BLUE "\e[34m"
    #define C_MAGENTA "\e[35m"
    #define C_CYAN "\e[36m"
    #define C_GRAY "\e[37m"
    #define C_WHITE "\e[97m"


    #define BG_C_DEFAULT "\e[49m"
    #define BG_C_BLACK "\e[40m"
    #define BG_C_RED  "\e[41m"
    #define BG_C_GREEN "\e[42m"
    #define BG_C_YELLOW "\e[43m"
    #define BG_C_BLUE "\e[44m"
    #define BG_C_MAGENTA "\e[45m"
    #define BG_C_CYAN "\e[46m"
    #define BG_C_GRAY "\e[100m"
    #define BG_C_WHITE "\e[107m"
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

    int pointToID(struct grigliaCitta* mappa, int x, int y, int SO_SOURCES);



#endif