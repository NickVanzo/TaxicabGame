/*messa solo per potere fare del debug o comunque non avere errori dall'ide*/
#ifndef INCLUDE_TIME
    #define INCLUDE_TIME
    #include <time.h>
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