
#include "include_main.h"



/*
    funzione per la stampa in master delle statistiche della cella
    map_cell mappa[][] la mappa del gioco
    int statistiche[6]: array contenente le statistiche:
        0-> numero di corse completate con successo
        1-> numero di corse no completate
        2-> numero di corse abortite
        3-> pid processo con la somma di tempo di corsa maggiore tra tutte
        4-> pid processo che ha percorso più distanza
        5-> pid processo che ha servito più clienti
*/
void stampaStatistiche(map_cell **mappa, int *statistiche);

/*
    funzione per inizializzare la simulazione e tenere il codice del main meno sporco possibile
    questa funzione legge dal stdin o argv i parametri e li assegna.
    Inoltre stampa un riepilogo dei parametri e informa l'utente della dimensione consigliata dello schermo
    aspetta infine che l'utente prenma un tasto per iniziare  la simulazione per potere andare a stampare in continuo.
    in questo modo utente ha tempo di aggiustare lo schermo e controllare che i parametri inseriti sono validi

*/
void setupSimulation(int *SO_TAXI, int *SO_SOURCES, int *SO_HOLES, int *SO_CAP_MIN, int *SO_CAP_MAX, int *SO_TIMENSEC_MIN, int* SO_TIMENSEC_MAX, int argc, char *argv[]);

/*Aggiungi descrizione*/
void spawnBlocks(map_cell **mappa, int numeroBuchi,int i, int j, int SO_HOLES);


int main(int argc, char *argv[]){
    int i,j,numeroBuchi; /*variabili iteratrici nei cicli. numerobuchi conta il numero di buchi che ho creato*/
    int SO_TAXI, SO_SOURCES, SO_HOLES, SO_CAP_MIN, SO_CAP_MAX, SO_TIMENSEC_MIN, SO_TIMENSEC_MAX; /*parametri letti o inseriti a compilazione*/

    map_cell **mappa;


    int stats[6]; /*TEMPORANEA DA METTERE MEGLIO SOLO PER NON AVERE BUFFER OVERFLOW nei test di stampa dello schermo*/

    srand(getpid()); 
 
    /*avvio setup della simulazione*/
    setupSimulation(&SO_TAXI, &SO_SOURCES, &SO_HOLES, &SO_CAP_MIN, &SO_CAP_MAX, &SO_TIMENSEC_MIN, &SO_TIMENSEC_MAX, argc, argv);
   

    /*codice di esempio per stampa mappa!*/

    mappa = (map_cell **) malloc(SO_HEIGHT*sizeof(map_cell*));

    /*generazione di buchi a celle con coordinate casuali che rispettano quanto richiesto dalla consegna (distanza uno dall'altro)*/
    spawnBlocks(mappa, numeroBuchi,i,j,SO_HOLES);   
      
    stampaStatistiche(mappa, stats);

   return 0;
}

/*Togli i, j*/
void spawnBlocks(map_cell ** mappa, int numeroBuchi,int i,int j,int SO_HOLES) {

    numeroBuchi = 0;
    /*inizializzo il tipo di ogni cella della tabella ROAD e marco come 1(inteso come la variabile booleana TRUE) la disponibilita' di ospitare un buco*/
    /*========================NICK============================*/
    for(i=0;i<SO_HEIGHT;i++) {
        for(j=0;j<SO_WIDTH;j++) {
            (&mappa[i][j])->cellType = ROAD; 
            (&mappa[i][j])->availableForHoles = 1;
        }
    }

    /*Ciclo fino a quando non ho posizionato tutti i buchi richiesti*/
    while(numeroBuchi < SO_HOLES) {
        /*PROBLEMA DELL'ALGORITMO: TROPPI BUCHI LO FANNO CICLARE ALL'INFINITO PERCHE' NON TROVA COORDINATE ADATTE, NON DOVREBBERO ESSERCI ALTRI PROBLEMI NOTI AL MOMENTO*/
        /*Randomizzo le coordinate della cella che voglio far diventare buco*/
        i = rand()%SO_HEIGHT;
        j = rand()%SO_WIDTH;
        if((&mappa[i][j])->availableForHoles == 1) {
            /*La cella con le coordinate ottenute randomicamente vengono segnate come BLOCK*/
            (&mappa[i][j])->cellType = BLOCK;
            (&mappa[i][j])->availableForHoles = 0;

            #ifdef DEBUG_BLOCK 
            fprintf(stdout, "%d - Posizionato buco in posizione %d%d\n", numeroBuchi,i,j); 
            #endif
            /*La cella su cui posiziono il buco non potra' contenere altri buchi e nemmeno le 8 celle che ha intorno potranno*/

            /*marco come inutilizzabili per contenere buchi le celle intorno alla cella su cui ho posizionato il buco*/
            if(i > 0) {
                if(j > 0)
                    {
                    /*Cella in alto a sinistra*/
                    (&mappa[i-1][j-1])->availableForHoles = 0;
                    }
                /*Cella in alto centrale*/
                (&mappa[i-1][j])->availableForHoles = 0;
                if(j < SO_WIDTH-1) {
                    /*Cella centrale a sinistra*/
                    (&mappa[i][j-1])->availableForHoles = 0;
                }
                    
            }

            if(j > 0) {
                /*Cella centrale a sinistra*/
                (&mappa[i][j-1])->availableForHoles = 0;
            }
                
            /*Per la cella centrale non ho bisogno di un ulteriore controllo perche' e' la cella su cui siamo seduti*/
            if(j < SO_WIDTH-1)
                {
                    /*Cella centrale a destra*/
                    (&mappa[i][j+1])->availableForHoles = 0;
                }
            
            if(i < SO_HEIGHT-1) {
                if(j > 0) {
                    /*Cella in basso a sinistra*/
                    (&mappa[i+1][j-1])->availableForHoles = 0;
                }
                /*Cella in basso centrale*/
                (&mappa[i+1][j])->availableForHoles = 0;
                
                if(j < SO_WIDTH-1) {
                        /*Cella in basso a destra*/
                        (&mappa[i+1][j+1])->availableForHoles = 0;
                    }
            }
            /*Se il buco e' stato posizionato allora incremento, altrimenti non ho trovato una posizione valida, continuo il ciclo*/
            numeroBuchi++;
        }
    }
}





void setupSimulation(int *SO_TAXI, int *SO_SOURCES, int *SO_HOLES, int *SO_CAP_MIN, int *SO_CAP_MAX, int *SO_TIMENSEC_MIN, int* SO_TIMENSEC_MAX, int argc, char *argv[]){
    int screenHeight, screenWidth;
    char bufferTemp[1024]; /*buffer temporaneo per lettura dei parametri da tastiera*/
    char tmpChar; /*per leggere eventuali input non desiderati*/


     if(argc == 7){ /*CONTROLLARE PERCHè NON MI RICORDO BENE SE FUNZIONA COSì*/
        *SO_TAXI = atoi(argv[1]);
        *SO_SOURCES = atoi(argv[2]);
        *SO_HOLES = atoi(argv[3]);
        *SO_CAP_MIN = atoi(argv[4]);
        *SO_CAP_MAX = atoi(argv[5]);
        *SO_TIMENSEC_MIN = atoi(argv[6]);
        *SO_TIMENSEC_MIN = atoi(argv[7]);
    }else{
        printf("Insert number of taxi available for the simulation: ");
        fgets(bufferTemp, 20, stdin); /*leggo il testo inserito nello stdin un intero ha al massimo 10 simboli ma non idandomi nell'utente dico che ne leggo al massimo 20 (inclus il \n alla fine)*/
        *SO_TAXI = atoi(bufferTemp); /*converto da stringa a intero*/

        printf("Insert number of sources available for the simulation: ");
        fgets(bufferTemp, 20, stdin); /*leggo il testo inserito nello stdin un intero ha al massimo 10 simboli ma non idandomi nell'utente dico che ne leggo al massimo 20 (inclus il \n alla fine)*/
        *SO_SOURCES = atoi(bufferTemp); /*converto da stringa a intero*/

        printf("Insert number of holes available for the simulation: ");
        fgets(bufferTemp, 20, stdin); /*leggo il testo inserito nello stdin un intero ha al massimo 10 simboli ma non idandomi nell'utente dico che ne leggo al massimo 20 (inclus il \n alla fine)*/
        *SO_HOLES = atoi(bufferTemp); /*converto da stringa a intero*/

        printf("Insert minimum capacity for each cell: ");
        fgets(bufferTemp, 20, stdin); /*leggo il testo inserito nello stdin un intero ha al massimo 10 simboli ma non idandomi nell'utente dico che ne leggo al massimo 20 (inclus il \n alla fine)*/
        *SO_CAP_MIN = atoi(bufferTemp); /*converto da stringa a intero*/

        printf("Insert maximum capacity for each cell: ");
        fgets(bufferTemp, 20, stdin); /*leggo il testo inserito nello stdin un intero ha al massimo 10 simboli ma non idandomi nell'utente dico che ne leggo al massimo 20 (inclus il \n alla fine)*/
        *SO_CAP_MAX = atoi(bufferTemp); /*converto da stringa a intero*/

        printf("Insert minimum crossing time for each cell: ");
        fgets(bufferTemp, 20, stdin); /*leggo il testo inserito nello stdin un intero ha al massimo 10 simboli ma non idandomi nell'utente dico che ne leggo al massimo 20 (inclus il \n alla fine)*/
        *SO_TIMENSEC_MIN = atoi(bufferTemp); /*converto da stringa a intero*/

        printf("Insert maximum crossing time for each cell: ");
        fgets(bufferTemp, 20, stdin); /*leggo il testo inserito nello stdin un intero ha al massimo 10 simboli ma non idandomi nell'utente dico che ne leggo al massimo 20 (inclus il \n alla fine)*/
        *SO_TIMENSEC_MIN = atoi(bufferTemp); /*converto da stringa a intero*/
    }

    /*calcolo le dimensioni consigliate della finestra di terminale per esperienza ottimale*/
    screenHeight = 4 + SO_HEIGHT;
    if(SO_HEIGHT<2) screenHeight = 6;

    screenWidth = 7*(2+SO_WIDTH) +40; /*lunghezza della singola cella per la dimenzione della mappa piu la len max delle stat*/

    printf("Simulation will now start with thw following parameters:\n\tSO_TAXI: %d\n\tSO_SOURCES: %d\n\tSO_HOLES: %d\n\tSO_CAP_MIN: %d\n\tSO_CAP_MAX: %d\n\tSO_TIMENSEC_MIN: %d\n\tSO_TIMENSEC_MAX: %d\n\n%s For a better experience, a terminal with minimum %d char width and exactly %d character height is required %s\n\nPress any key to start the simulation...", *SO_TAXI, *SO_SOURCES, *SO_HOLES, *SO_CAP_MIN, *SO_CAP_MAX, *SO_TIMENSEC_MIN, *SO_TIMENSEC_MAX, C_YELLOW, screenHeight, screenWidth, C_DEFAULT);

    tmpChar=getc(stdin); /*leggo input per potere avviare simulazione, prima assicurandomi che la dimensione del terminale sia mantenuta delle dimansioni buone*/
    /*aggiuntgere chiamata ricorsiva per conferma*/
}



void stampaStatistiche(map_cell **mappa, int *statistiche){
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
    for(i=0;i<SO_WIDTH+2;i++){
        colorPrintf("       ", GRAY, GRAY);
    }
    printf("\n");
    for(i=0;i<SO_WIDTH+2;i++){
        colorPrintf("       ", GRAY, GRAY);
    }
    printf("\n");


    
    for(i=0; i<SO_HEIGHT; i++){
        colorPrintf("       ", GRAY, GRAY); /*stampo bordo laterale sx*/
        for(j=0;j<SO_WIDTH;j++){

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
         if(i<6) printf("%s%d\n", stats[i], statistiche[i]);
         else printf("\n");
    }


    /*stampo bordo inferiore*/
    for(i=0;i<SO_WIDTH+2;i++){
        colorPrintf("       ", GRAY, GRAY);
    }
    printf("\n");
    for(i=0;i<SO_WIDTH+2;i++){
        colorPrintf("       ", GRAY, GRAY);
    }
    printf("\n");

    /*controllo che ho stampato tutti gli stats*/
    if(SO_HEIGHT < 6){
        for(i=SO_WIDTH+2; i<6;i++){
            for(j=0;j<SO_WIDTH; j++) printf("   "); /*mi allineo alla fine*/
            printf("%s%d\n", stats[i],0);
        }
    }
}
