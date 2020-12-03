#include "include_main.h"

/*
    utilizzo: ./control oppure ./control -r <xsource> <ysource> <xdest> <ydest> oppure ./controll -k per uccidere tutti i taxi
*/
int main(int argc, char *argv[]){
    int i, temp;
    char command[128]; /*per il -r o -k*/
    int xSource, ySource, xDest, yDest; /*cordinate di inzio e di fine della corda*/
    int sourceIdentifier; /*identificatore univoco della sorgente*/
    int msgQueueKey; /*chiave di accesso della messagebox*/
    int msgQueue;
    struct msgBuf bufferMessaggio;
    boolean exitFromProgram = FALSE;
    
    msgQueueKey = ftok("./msgQueue.key",1);

    msgQueue = msgget(msgQueueKey, 0); /*ottengo la chiave della msg queue*/

    if(msgQueue <0){
        colorPrintf("\n\tUnable to connect to main simulation message queue...\n\tAre you shure the simulation is running?\n", RED, DEFAULT);
        printf("\tDetailed error information: %s\n\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if(argc > 1 ){
         if(strcmp(argv[1], "-c") == 0){
            /*sto andando a creare un messaggio di tipo request*/
            if(argc != 6){
                colorPrintf("\nError: usacge -> ./control -c sourceX sourceY destX destY\n",RED, DEFAULT );
                exit(EXIT_FAILURE);
            }
            xSource = atoi(argv[2]);
            ySource = atoi(argv[3]);
            xDest = atoi(argv[4]);
            yDest = atoi(argv[5]);

            bufferMessaggio.mtype = 1234; /*impostare funzione che crea il tipo*/

            bufferMessaggio.xDest = xDest;
            bufferMessaggio.yDest = yDest;
            printf("senting message from <%d;%d> to <%d;%d>\n",xSource,ySource ,xDest, yDest);
            if(msgsnd(msgQueue, &bufferMessaggio, 2*sizeof(int), 0) <0){
                colorPrintf("Error sending command!\n", RED, DEFAULT);
                printf("Detailed error information: %s\n\n", strerror(errno));
                exit(EXIT_FAILURE);
            }

            colorPrintf("Command sent!\n", GREEN, DEFAULT);

        }else if(strcmp(argv[1], "-k")==0){
            if(argc != 3){
                colorPrintf("\nError: usacge -> ./control -k NUM_OF_TAXI_TO_REMOVE\n",RED, DEFAULT );
                exit(EXIT_FAILURE);
            }
            temp = atoi(argv[2]); /*ottengo il numero di processi...*/
            /*sto cercando di killare tutti i taxi forzatamente*/
            colorPrintf("Sending kill message to all taxi\n", YELLOW, DEFAULT);
            bufferMessaggio.mtype = 0; /*0 per uccidere il messaggio*/
            bufferMessaggio.xDest=-1;
            bufferMessaggio.yDest=-1;
            for(i=0;i<temp;i++){
                if(msgsnd(msgQueue, &bufferMessaggio, 2*sizeof(int), 0) <0){
                        colorPrintf("Error sending command!\n", RED, DEFAULT);
                        printf("Detailed error information: %s\n\n", strerror(errno));
                        exit(EXIT_FAILURE);
                }
            }
            colorPrintf("All messages have been sent... Bye\n", GREEN, DEFAULT);
        }else{
            /*comando non implementato*/
            colorPrintf("Error: command not implemented... aborting...\n", RED, DEFAULT);
            exit(EXIT_FAILURE);
        }
    }else{

        colorPrintf("\n\nControl program for Taxicab Project.\nCreated by Santimaria m.e. and Vanzo N.\n\n", YELLOW, DEFAULT);
        colorPrintf("For a list of commands type \"help\"\n", MAGENTA, DEFAULT);
        printf("Last compile time and date: %s:%s\n\n", __DATE__, __TIME__ );

        while(!exitFromProgram){
            colorPrintf("\nCOMMAND>", YELLOW, DEFAULT);
            scanf("%s", command);

            if(strcmp(command, "help") == 0){
                printf("\nList of commands:\n\n\thelp:\tShows this help\n\texit:\texit from this program\n\tcreate\tcreate a new request for taxicab\n\tkill:\tkill all the taxi in the simulation\n\n");
            }else if(strcmp(command, "exit") == 0){
                exitFromProgram = TRUE;
            }else if(strcmp(command, "kill") == 0){
                printf("Insert number of taxi to kill in the simulation: ");
                scanf("%s", command);
                temp = atoi(command);
                bufferMessaggio.mtype = 0; /*0 per uccidere il messaggio*/
                bufferMessaggio.xDest=-1;
                bufferMessaggio.yDest=-1;
                for(i=0;i<temp;i++){
                    if(msgsnd(msgQueue, &bufferMessaggio, 2*sizeof(int), 0) <0){
                        colorPrintf("Error sending command!\n", RED, DEFAULT);
                        printf("Detailed error information: %s\n\n", strerror(errno));
                        exit(EXIT_FAILURE);
                    }
                }
                colorPrintf("All kill comands have been sent!\n", GREEN, DEFAULT);
                


            }else if(strcmp(command, "create") == 0){
                printf("Insert source X and Y coordinate separated by space: ");
                scanf("%d %d", &xSource, &ySource);
                printf("Insert desintation X and Y coordinate separated by space: ");
                scanf("%d %d", &xDest, &yDest);
                bufferMessaggio.mtype = 1234566 ;/*cambiare con funzione che calcola il tipo di messaggio*/
                bufferMessaggio.xDest = xDest;
                bufferMessaggio.yDest = yDest;
                printf("senting message from <%d;%d> to <%d;%d>\n",xSource,ySource ,xDest, yDest);
                if(msgsnd(msgQueue, &bufferMessaggio, 2*sizeof(int), 0) <0){
                        colorPrintf("Error sending command!\n", RED, DEFAULT);
                        printf("Detailed error information: %s\n\n", strerror(errno));
                        exit(EXIT_FAILURE);
                }
                colorPrintf("Message has been sent!\n", GREEN, DEFAULT);



            }else{
                colorPrintf("\n\n\tUNKNOWN COMMAND! For available commands type \"help\"\n\n", RED, DEFAULT);
            }
        }

    }

    printf("\nThank you for using this program... bye!\n\n");

    exit(EXIT_SUCCESS);
    
}