	#include "include_main.h"

int queueKey, queueId;
struct msgBuf myMessage;
int SO_SOURCE, SO_DURATION;
int shmKey, shmId;
struct grigliaCitta *mappa;

void signalHandler(int signal);
int exitFromLoop = 0;

int main(int argc,  char * argv[]){
    
    if(argc != 3){
        printf("Error parametrs not matching!\n");
        exit(EXIT_FAILURE);
    }
    srand(getpid());
    SO_SOURCE = atoi(argv[1]);
    SO_DURATION = atoi(argv[2]);

    queueKey = ftok("ipcKey.key", 1);
    if(queueKey == -1){
        printf("Error retriving message queue key!\n");
        exit(EXIT_FAILURE);
    }

    queueId = msgget(queueKey, IPC_CREAT | 0666);
    if(queueId == -1){
        printf("Error retriving queue id!\n");
        exit(EXIT_FAILURE);
    }

    shmKey = ftok("ipcKey.key", 2);
    if(shmKey == -1){
        printf("Error retriving shared memory key!\n");
        exit(EXIT_FAILURE);
    }

    shmId = shmget(shmKey, sizeof(struct grigliaCitta),0);
    if(shmId == -1){
        printf("Error retriving shared memory ID!\n");
        exit(EXIT_FAILURE);
    }

    mappa = shmat(shmId, NULL, 0);
    if(mappa == (struct grigliaCitta *)(-1)){
        printf("Error attaching memory segment!\n");
        exit(EXIT_FAILURE);
    }



    signal(SIGALRM, signalHandler);

    while(exitFromLoop == 0){
        alarm((rand() % SO_DURATION)+1);  
        pause();
    }
    
    shmdt(mappa);
    exit(EXIT_SUCCESS);
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
                write(1,"Error sending message!\n", strlen("Error sending message!\n"));
            }
            break;
          case SIGUSR1:
          	exitFromLoop = 1;
          	break;

        default:
            break;
    }
}