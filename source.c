#include "include_main.h"

int queueKey, queueId;
struct msgBuf myMessage;
int SO_SOURCE, SO_DURATION;
int shmKey, shmId;
struct grigliaCitta *mappa;

void signalHandler(int signal);

int main(int argc,  char * argv[]){
    
    if(argc != 3){
        printf("Error parametrs not matching!\n");
        exit(EXIT_FAILURE);
    }
    srand(getpid());
    SO_SOURCE = atoi(argv[1]);
    SO_DURATION = atoi(argv[2]);

    
    queueKey = ftok("msgQueue.key", 1);
    queueId = msgget(queueKey, 0);

    shmKey = ftok("msgQueue.key", 2);
    shmId = shmget(shmKey, sizeof(struct grigliaCitta),0);
    mappa = shmat(shmId, NULL, 0);



    signal(SIGALRM, signalHandler);

    while(SO_SOURCE > 0){
        alarm((rand() % SO_DURATION)+1);  
        pause();

        SO_SOURCE--;
    }
    
    shmdt(mappa);

}


void signalHandler(int signal){
    switch(signal){
        case SIGALRM:
            myMessage.mtype = (rand()%SO_SOURCE)+1;
            do{
                myMessage.xDest = rand()%SO_WIDTH;
                myMessage.yDest = rand()%SO_HEIGHT;
            }while(mappa->matrice[myMessage.xDest][myMessage.yDest].cellType != ROAD);
                

            if(msgsnd(queueId, &myMessage, 2*sizeof(int), 0) == -1){
                write(stdout,"Error sending message!\n", strlen("Error sending message!\n"));
            }
            break;
            

        default:
            break;
    }
}