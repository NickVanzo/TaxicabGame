#include "include_main.h"

int main(int argc, char * argv[]){
	  	int x, y; /*Coordinate della posizione del taxi*/
		int queue_key, queue_id; /*Variabili per la coda di messaggi*/
		int shm_Key, shm_id; /*Variabili per la memoria condivisa*/
		struct grigliaCitta *mappa; /*mappa della citta*/

		/*Apertura coda di messaggi*/	
		queue_key = ftok("ipcKey.key", 1);
		if(queueKey == -1){
        	printf("Error retriving message queue key!\n");
        	exit(EXIT_FAILURE);
    	}	

		queue_id  = msgget(queue_key, 0);
		if(queueId == -1){
        	printf("Error retriving queue id!\n");
        	exit(EXIT_FAILURE);
    	}

		/*Attach alla memoria condivisa*/
		shm_Key = ftok("ipcKey.key", 2);
		if(shmKey == -1){
        	printf("Error retriving shared memory key!\n");
        	exit(EXIT_FAILURE);
    	}

		shm_id = shmget(shm_Key, sizeof(struct grigliaCitta, 0));
    	if(shmId == -1) {
        	printf("Error retriving shared memory ID!\n");
        	exit(EXIT_FAILURE);
    	}

    	mappa = shmat(shmId, NULL, 0);
    	if(mappa == (struct grigliaCitta *)(-1)){
        	printf("Error attaching memory segment!\n");
        	exit(EXIT_FAILURE);
    	}


    	/*detach dalla memoria condivisa*/
    	shmdt(mappa);
}