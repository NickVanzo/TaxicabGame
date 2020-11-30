#include "colors.h"


void colorPrint(char *message, enum color colore){
    switch(colore){
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

    printf("%s%s", message, C_DEFAULT);
}