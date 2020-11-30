/*usto per avere debug e non avere warning nell'editor*/
#include "../../include_main.h"



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