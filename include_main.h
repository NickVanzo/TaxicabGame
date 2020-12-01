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


#ifndef DEFINE_CUSTOM_FUNCTIONS
    #define DEFINE_CUSTOM_FUNCTIONS
    /*qua dentro definiamo tutti i prototipi delle funzioni che andiamo a scrivere*/

    /*
        FUNZIONI UI
    
    */
    void colorPrintf(char *message, enum color colore, enum color bgcolore);





#endif