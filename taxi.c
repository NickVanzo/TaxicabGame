#include "include_main.h"

/*
	La funzione "spawnTaxi" crea un taxi in una posizione casuale della mappa facendo attenzione che la posizione casuale risponda ai seguenti criteri:
	1 - ci deve essere spazio per ospitare il taxi
	2 - la cella selezionata deve essere di tipo ROAD o SOURCE
*/
void spawnTaxi(struct grigliaCitta *mappa, int taxiSemaphore_id);

/*
    La funzione "closestSource" restituisce la so_source piu vicina date le coordinate taxiX e taxiY. 
    le coordinate della dest piu vicina sono dentro  destX e desY.
*/

void closestSource(struct grigliaCitta *mappa);

/*
    La funzione "enumSoSources" permette di ottenere il tipo di messaggio su cui mettersi in ascolto, data la posizione del taxi
    il tipo di messaggio corrsiponde al numero in cui compare la SO_SOURCE con una letttura sequenziale della mappa
    ovvero per esempio se ho una so source in 0,0 e una in 0,3 senza altre source nelle coordiante intermedie, la prima avrà mtype = 1  e la seconda avrà mtype=2 e così via...
    la funzione ritorna -1 in caso di errore
*/
int enumSoSources(struct grigliaCitta *mappa);

/*
	La fuzione move permette al taxi di muoversi verso la sua destinazione, sia SO_SOURCE che destinazione prelevata dal messaggio
	move fa uso di funzioni di appoggio, ossia: moveUp, moveDown, moveLeft, moveRight. Le quali permettono di muoversi verso la direzione precisata nel nome della funzione.
*/
void move(struct grigliaCitta *mappa);
void moveUp(struct grigliaCitta *mappa);
void moveDown(struct grigliaCitta *mappa);
void moveLeft(struct grigliaCitta *mappa);
void moveRight(struct grigliaCitta *mappa);

/*
    questa funzione permette di prelevare un messaggio, dato un taxi presente in una so_source...
*/
void getRide(int msg_queue_id, long so_source);

/*
	La funzione signalHandler gestisce il segnale che indica al processo di fermare l'esecuzione e procedere con la disallocazione degli IPCS.
*/
void signalHandler(int signalNo);

/*
	La struttura "taxi_statistiche" viene caricata in memoria condivisa dal master e viene utilizzata dai taxi per permettere al master di stampare il pid del taxi con le caratteristiche richieste.
	Ogni volta che un taxi esegue uno spostamento aggiorna le variabili presenti nella sua struct e le confronta con quelle presenti in memoria condivisa, effettuando le dovute sostituzioni
	qualora le variabili del taxi che sta controllando la struct siano minori delle sue caratteristiche, così il master può eseguire la stampa del pid del taxi corretto.
*/
struct taxiStatistiche * taxi_statistiche;

/*
	La funzione "Ptemp" è una funzione usata solamente da taxi.c e serve per attendere l'accesso alle risorse della mappa e rilasciare le proprie quando il il tempo della simulazione scade.
	I parametri accettati dalla funzione sono: l'id del semaforo su cui esegurie l'operazione specifica e il tempo che deve passare affinchè il taxi cessi di aspettare le risorse e rilasci le sue.
*/
int Ptemp(int semaphore, long time)
{
	struct timespec timeout_taxi;
	struct sembuf sops;
	sops.sem_flg = 0;
	sops.sem_num = 0;
	sops.sem_op = -1;
	timeout_taxi.tv_sec = 0;
	timeout_taxi.tv_nsec = time;
	return semtimedop(semaphore, &sops, 1, &timeout_taxi);
}

/*
	Dichiarazione variabili e struct globali

	La struct _posTaxi viene usata per ottenere informazioni sulla posizione del taxi nella mappa.
	La variabile intera posR indica la posizione effettiva del taxi sulle x. (R sta per riga)
	La variabile intera posC indica la posizione effettiva del taxi sulle y. (C sta per colonna)
	La variabile intera desR indica la posizione verso cui si deve dirigere il taxi. (Sulle x)
	La variabile intera desC indica la posizione verso cui si deve dirigere il taxi. (Sulle y)
*/
struct _posTaxi
{
	int posR;
	int posC;
	int destR;
	int destC;
}
posizioneTaxi;

/*variabili riguardanti la gestione dei tempi necessari ai taxi per lo spostamento e per il rilascio di risorse*/
int so_timeout;
struct timespec time_struct;

/*Variabili con cui si eseguirà il confronto con altri taxi per la stampa del pid del taxi con la caratteristica più alta*/
int strada_percorsa;
int richieste_finite;
unsigned long int tempo_in_strada;

/*Variabili gestionali per: sapere quando terminare la propria esecuzione (exit_program) e sapere se è stata presa in carico una richiesta di un cliente*/
boolean exit_program;
boolean ride_taken;

int main(int argc, char *argv[])
{
	/*Variabili per la memoria condivisa della mappa, dei taxi (per ottenere il semaforo usato per aspettare che tutti vengano posizionati) e delle statistiche da confrontare*/
	struct grigliaCitta * mappa;
	int shm_Key, shm_id, shmId_ForTaxi, shmKey_ForTaxi, shmKey_stats, shmID_stats;
	int taxiSemaphore_id;
	/*Variabili per la coda di messaggi*/
	int queue_key, queue_id;
	/*Variabile per ottenere la durata della simulazione*/
	int time_duration;
	struct sigaction sa;
	srand(getpid());
	/*Imposto l'handler del segnale SIGARLM con il mio handler personalizzato*/
	bzero(&sa, sizeof(sa));
	sa.sa_handler = signalHandler;
	sigaction(SIGALRM, &sa, NULL);
	
	exit_program = FALSE;
	ride_taken = FALSE;

	/*500000000 sono 0,5 secondi*/
	so_timeout = atol(argv[1]);
	time_duration = atoi(argv[2]);
	alarm(time_duration);

	/*Apertura coda di messaggi*/
	queue_key = ftok("ipcKey.key", 1);
	if (queue_key == -1)
	{
		printf("Error retriving message queue key!\n");
		exit(EXIT_FAILURE);
	}

	queue_id = msgget(queue_key, IPC_CREAT | 0666);
	if (queue_id == -1)
	{
		printf("Error retriving queue id!\n");
		exit(EXIT_FAILURE);
	}

	/*Attach alla memoria condivisa*/
	shm_Key = ftok("ipcKey.key", 2);
	if (shm_Key == -1)
	{
		printf("Error retriving shared memory key!\n");
		exit(EXIT_FAILURE);
	}

	shm_id = shmget(shm_Key, sizeof(struct grigliaCitta), IPC_CREAT | 0666);
	if (shm_id == -1)
	{
		printf("Error retriving shared memory ID!\n");
		exit(EXIT_FAILURE);
	}

	mappa = shmat(shm_id, NULL, 0);
	if (mappa == (struct grigliaCitta *)(-1))
	{
		printf("Error attaching memory segment!\n");
		exit(EXIT_FAILURE);
	}
	/*Attach alla memoria condivisa contenente il semaforo che mi permette di aspettare tutti i taxi prima di partire*/
	shmKey_ForTaxi = ftok("ipcKey.key", 3);
	taxiSemaphore_id = semget(shmKey_ForTaxi, 1, IPC_CREAT | 0666);

	/*Attach alla memoria condivisa che mi permette di confrontare le statistiche e far stampare al master il pid corretto*/
	shmKey_stats = ftok("ipcKey.key", 5);
	shmID_stats = shmget(shmKey_stats, sizeof(struct taxiStatistiche), IPC_CREAT | 0666);
	taxi_statistiche = shmat(shmID_stats, NULL, 0);

	spawnTaxi(mappa, taxiSemaphore_id);

	/*
		Ciclo principale di esecuzione del taxi:
		1) dopo essere spawnato il taxi individua la source più vicina e si muove per raggiungerla;
		2) preleva dalla coda di messaggi un messaggio contenente le informazioni su dove deve andare;
		3) arriva a destinazione e ricomincia con il punto 1;
		4) quando il tempo della simulazione scade il taxi riceve un segnale dal SO (impostato con alarm) che permette al taxi di uscire dal ciclo ed eseguire il detach dalla memoria condivisa
	*/
	while (!exit_program)
	{
		closestSource(mappa);
		move(mappa);
		getRide(queue_id, (long) enumSoSources(mappa));
		move(mappa);
	}

	/*Detach dalla memoria condivisa, terminazione con successo*/
	shmdt(mappa);
	shmdt(taxi_statistiche);
	exit(EXIT_SUCCESS);
}

void spawnTaxi(struct grigliaCitta *mappa, int taxiSemaphore_id)
{
	int availableSpaceOnCell;
	int i = 0, j = 0;
	/*Seleziono un punto casuale della mappa in cui spawnare, se il massimo di taxi in quella cella è stato raggiunto o la cella ha tipo BLOCK cambio cella*/
	do {
		posizioneTaxi.posR = rand() % SO_HEIGHT;
		posizioneTaxi.posC = rand() % SO_WIDTH;
		availableSpaceOnCell = semctl(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].availableSpace, 0, GETVAL);
	} while (availableSpaceOnCell == 0 && (mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].cellType != BLOCK));

	/*Il controllo su taxisemaphore permette ai taxi che vengono creati dopo che tutti gli altri taxi hanno cominciato a muoversi di non bloccarsi facendo una P su tale semaforo*/
	if (semctl(taxiSemaphore_id, 0, GETVAL) > 0)
	{
		P(taxiSemaphore_id);
	}
	/*Richiedo i semafori mutex e available space diminuendo il loro valore di 1*/
	P(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
	P(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].availableSpace);
	/*Sezione critica in cui diminuisco i valori presenti in memoria condivisa che servono per eseguire una stampa corretta*/
	mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].taxiOnThisCell++;
	mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].totalNumberOfTaxiPassedHere++;
	/*Rilascio di risorsa (availableSpace non viene rilasciato in quanto non mi sto spostando dalla cella)*/
	V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
	/*Aspetto che tutti i taxi siano spawnato*/
	VwaitForZero(taxiSemaphore_id);
}

/*
	La funzione move funziona nel seguente modo: 
	1) il taxi si muove lungo le y (NON CARTESIANE, IL SISTEMA DI RIFERIMENTO E' UNA MATRICE[X][Y]) finchè la sua y è diversa da quella della sua destinazione;
	2) se incontra una cella block si sposta di uno in su o in giù (tenendo conto dei limiti imposti dalla mappa) per poi proseguire con lo spostamento sulle y;
	3) esegue le stesse cose di 1 e 2 ma sulle x (muovendosi a destra o sinistra in caso ci sia una cella di tipo BLOCK a sbarrarle la strada);
*/
void move(struct grigliaCitta *mappa)
{
	/*----------------------------------------------SPOSTAMENTO VERSO SINISTRA---------------------------------------------------------*/
	while (posizioneTaxi.posC > posizioneTaxi.destC)
	{
		if (mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC - 1].cellType != BLOCK)
		{
			moveLeft(mappa);
		}
		else
		{
			if (posizioneTaxi.posR > 0)
			{
				moveUp(mappa);
			}
			else
			{
				moveDown(mappa);
			}
		}
	}
	/*------------------------------------------------SPOSTAMENTO VERSO IL BASSO-------------------------------------------------------------*/
	while (posizioneTaxi.posR < posizioneTaxi.destR)
	{
		if (mappa->matrice[posizioneTaxi.posR + 1][posizioneTaxi.posC].cellType != BLOCK)
		{
			moveDown(mappa);
		}
		else
		{
			if (posizioneTaxi.posC > 0)
			{
				moveLeft(mappa);
			}
			else
			{
				moveRight(mappa);
			}
		}
	}
	/*---------------------------------------------------SPOSTAMENTO VERSO L'ALTO-----------------------------------------------------------*/
	while (posizioneTaxi.posR > posizioneTaxi.destR)
	{
		if (mappa->matrice[posizioneTaxi.posR - 1][posizioneTaxi.posC].cellType != BLOCK)
		{
			moveUp(mappa);
		}
		else
		{
			if (posizioneTaxi.posC > 0)
			{
				moveLeft(mappa);
			}
			else
			{
				moveRight(mappa);
			}
		}
	}
	/*---------------------------------------------------SPOSTAMENTO VERSO DESTRA-----------------------------------------------------------*/
	while (posizioneTaxi.posC < posizioneTaxi.destC)
	{
		if (mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC + 1].cellType != BLOCK)
		{
			moveRight(mappa);
		}
		else
		{
			if (posizioneTaxi.posR > 0)
			{
				moveUp(mappa);
			}
			else
			{
				moveDown(mappa);
			}
		}
	}
	/*
    	Se lo spostamento non è completo, riavvio lo spostamento in maniera ricorsiva. 
		Se siamo arrivati a destinazione controllo la memoria condivisa delle statistiche per verificare se ho servito più clienti di quanti ne abbia serviti il taxi con più clienti serviti.
		Se i clienti da me serviti sono maggiori allora aggiorno le variabili della struct in memoria condivisa che si occupa delle statistiche.
    */
	if (posizioneTaxi.posR != posizioneTaxi.destR || posizioneTaxi.posC != posizioneTaxi.destC)
	{
		move(mappa);
	}
	else
	{
		posizioneTaxi.posR = posizioneTaxi.destR;
		posizioneTaxi.posC = posizioneTaxi.destC;
		if (ride_taken)
		{
			P(mappa->mutex);
			mappa->successes_rides++;
			V(mappa->mutex);

			richieste_finite++;
			P(taxi_statistiche->mutex);
			if (richieste_finite > taxi_statistiche->clienti_serviti)
			{
				taxi_statistiche->clienti_serviti = richieste_finite;
				taxi_statistiche->pid_clienti_serviti = getpid();
			}
			V(taxi_statistiche->mutex);
			ride_taken = FALSE;
		}
	}
}

/*
	La funzione moveUp permette al taxi di spostarsi di 1 cella in su richiedendo i seguenti semafori per permettere lo spostamento:
	1) il mutex della cella in cui si vuole spostare;
	2) il mutex della cella su cui correntemente risiede il taxi;
	3) il semaforo availableSpace per controllare che ci sia posto nella cella;
	Le richieste dei semafori vengono eseguite con una semtimedop() che permette al taxi di aspettare X secondi/millisecondi per poi allo scadere del tempo liberare le risorse qualora non avesse
	ottenuto le risorse necessarie a compiere lo spostamento.
	Le risorse liberate dipendono da quali semafori il taxi ha ottenuto
*/
void moveUp(struct grigliaCitta *mappa)
{
	time_struct.tv_sec = 0;
	time_struct.tv_nsec = mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].timeRequiredToCrossCell;
	if (Ptemp(mappa->matrice[posizioneTaxi.posR - 1][posizioneTaxi.posC].availableSpace, so_timeout) == -1 && errno == EAGAIN)
	{
		/*Il semaforo non è riuscito ad ottenere il semaforo, aumento corse abortite e rilascio eventuali risorse ottenute*/
		P(mappa->mutex);
		if (ride_taken == TRUE)
		{
			mappa->aborted_rides++;
		}
		V(mappa->mutex);
		P(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
		mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].taxiOnThisCell--;
		V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].availableSpace);
		V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
		exit(EXIT_FAILURE);
	}
	if (Ptemp(mappa->matrice[posizioneTaxi.posR - 1][posizioneTaxi.posC].mutex, so_timeout) == -1 && errno == EAGAIN)
	{
		P(mappa->mutex);
		if (ride_taken == TRUE)
		{
			mappa->aborted_rides++;
		}
		V(mappa->mutex);
		V(mappa->matrice[posizioneTaxi.posR - 1][posizioneTaxi.posC].availableSpace);
		P(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
		mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].taxiOnThisCell--;
		V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].availableSpace);
		V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
		exit(EXIT_FAILURE);
	}
	if (Ptemp(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex, so_timeout) == -1 && errno == EAGAIN)
	{
		P(mappa->mutex);
		if (ride_taken == TRUE)
		{
			mappa->aborted_rides++;
		}
		V(mappa->mutex);
		V(mappa->matrice[posizioneTaxi.posR - 1][posizioneTaxi.posC].availableSpace);
		V(mappa->matrice[posizioneTaxi.posR - 1][posizioneTaxi.posC].mutex);
		P(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
		mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].taxiOnThisCell--;
		V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].availableSpace);
		V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);

		exit(EXIT_FAILURE);
	}
	/*Il rilascio di risorse non è avvenuto perciò procedo con la modifica dei dati delle celle e procedo con lo spostamento*/

	/*
		SEZIONE CRITICA: qui vengono eseguite le seguenti azioni:
		1) abbandono la cella diminuendo il numero di taxi in quella cella;
		2) entro nella nuova cella aumentando il numero di taxi in quella cella;
		3) aumento il numero di taxi totali che hanno attraversato quella cella;
		4) eseguo lo spostamento decrementando o incrementando la variabile usata per muoversi lungo le x o le y;

	*/
	mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].taxiOnThisCell--;
	mappa->matrice[posizioneTaxi.posR - 1][posizioneTaxi.posC].taxiOnThisCell++;
	mappa->matrice[posizioneTaxi.posR - 1][posizioneTaxi.posC].totalNumberOfTaxiPassedHere++;
	posizioneTaxi.posR--;

	/*ESCO DALLA SEZIONE CRITICA e rilascio le risorse*/
	V(mappa->matrice[posizioneTaxi.posR + 1][posizioneTaxi.posC].availableSpace);
	V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
	V(mappa->matrice[posizioneTaxi.posR + 1][posizioneTaxi.posC].mutex);

	/*
		Blocco in cui si esegue il confronto tra le variabili del singolo taxi e le variabili presenti nella struct delle statistiche per sapere se è necessario inserire il proprio pid 
		o lasciare quello di un taxi con statistiche più alte di quelle del taxi che sta facendo il confronto.
		Il confronto è eseguito in mutua esclusione per evitare inconsistenze nei dati. Un taxi per volta può accedere alla struct in memoria condivisa con le statistiche.
	*/
	strada_percorsa++;
	tempo_in_strada = tempo_in_strada + time_struct.tv_nsec;
	P(taxi_statistiche->mutex);
	if (strada_percorsa > taxi_statistiche->strada_fatta)
	{
		taxi_statistiche->pid_strada_fatta = getpid();
		taxi_statistiche->strada_fatta = strada_percorsa;
	}
	if (tempo_in_strada > taxi_statistiche->tempo_in_strada)
	{
		taxi_statistiche->pid_tempo_in_strada = getpid();
		taxi_statistiche->tempo_in_strada = tempo_in_strada;
	}
	V(taxi_statistiche->mutex);
	nanosleep(&time_struct, NULL);
}

/*
	moveDown fa le stesse cose di moveUp con la differenza che il movimento è verso il basso.
	L'altro dettaglio che cambia è l'ordine con cui si richiedono le risorse. L'ordine con cui vengono richieste le risorse è studiato per evitare il Deadlock (ulteriori informazioni vengono fornite
	all'interno della documentazione allegata a questi file - di preciso la si può trovare nella relazione del progetto).
*/
void moveDown(struct grigliaCitta *mappa)
{
	time_struct.tv_sec = 0;
	time_struct.tv_nsec = mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].timeRequiredToCrossCell;
	if (Ptemp(mappa->matrice[posizioneTaxi.posR + 1][posizioneTaxi.posC].availableSpace, so_timeout) == -1 && errno == EAGAIN)
	{
		P(mappa->mutex);
		if (ride_taken == TRUE)
		{
			mappa->aborted_rides++;
		}
		V(mappa->mutex);
		P(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
		mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].taxiOnThisCell--;
		V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
		V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].availableSpace);

		exit(EXIT_FAILURE);
	}
	if (Ptemp(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex, so_timeout) == -1 && errno == EAGAIN)
	{
		P(mappa->mutex);
		if (ride_taken == TRUE)
		{
			mappa->aborted_rides++;
		}
		V(mappa->mutex);
		V(mappa->matrice[posizioneTaxi.posR + 1][posizioneTaxi.posC].availableSpace);
		P(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
		mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].taxiOnThisCell--;
		V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
		V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].availableSpace);

		exit(EXIT_FAILURE);
	}
	if (Ptemp(mappa->matrice[posizioneTaxi.posR + 1][posizioneTaxi.posC].mutex, so_timeout) == -1 && errno == EAGAIN)
	{
		P(mappa->mutex);
		if (ride_taken == TRUE)
		{
			mappa->aborted_rides++;
		}
		V(mappa->mutex);
		V(mappa->matrice[posizioneTaxi.posR + 1][posizioneTaxi.posC].availableSpace);
		V(mappa->matrice[posizioneTaxi.posR + 1][posizioneTaxi.posC].mutex); /*Rilascio il mutex vecchio*/
		P(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
		mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].taxiOnThisCell--;
		V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
		V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].availableSpace);

		exit(EXIT_FAILURE);
	}

	mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].taxiOnThisCell--;
	mappa->matrice[posizioneTaxi.posR + 1][posizioneTaxi.posC].taxiOnThisCell++;
	mappa->matrice[posizioneTaxi.posR + 1][posizioneTaxi.posC].totalNumberOfTaxiPassedHere++;
	posizioneTaxi.posR++;

	V(mappa->matrice[posizioneTaxi.posR - 1][posizioneTaxi.posC].availableSpace);
	V(mappa->matrice[posizioneTaxi.posR - 1][posizioneTaxi.posC].mutex);
	V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);

	strada_percorsa++;
	tempo_in_strada = tempo_in_strada + time_struct.tv_nsec;
	P(taxi_statistiche->mutex);
	if (strada_percorsa > taxi_statistiche->strada_fatta)
	{
		taxi_statistiche->pid_strada_fatta = getpid();
		taxi_statistiche->strada_fatta = strada_percorsa;
	}
	if (tempo_in_strada > taxi_statistiche->tempo_in_strada)
	{
		taxi_statistiche->pid_tempo_in_strada = getpid();
		taxi_statistiche->tempo_in_strada = tempo_in_strada;
	}
	V(taxi_statistiche->mutex);
	nanosleep(&time_struct, NULL);
}

/*
	moveLeft fa le stesse cose di moveUp con la differenza che il movimento è verso sinistra.
	L'altro dettaglio che cambia è l'ordine con cui si richiedono le risorse. L'ordine con cui vengono richieste le risorse è studiato per evitare il Deadlock (ulteriori informazioni vengono fornite
	all'interno della documentazione allegata a questi file - di preciso la si può trovare nella relazione del progetto).
*/
void moveLeft(struct grigliaCitta *mappa)
{
	time_struct.tv_sec = 0;
	time_struct.tv_nsec = mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].timeRequiredToCrossCell;
	if (Ptemp(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC - 1].availableSpace, so_timeout) == -1 && errno == EAGAIN)
	{
		P(mappa->mutex);
		if (ride_taken == TRUE)
		{
			mappa->aborted_rides++;
		}
		V(mappa->mutex);
		P(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
		mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].taxiOnThisCell--;
		V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].availableSpace);
		V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
		exit(EXIT_FAILURE);
	}
	if (Ptemp(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC - 1].mutex, so_timeout) == -1 && errno == EAGAIN)
	{
		P(mappa->mutex);
		if (ride_taken == TRUE)
		{
			mappa->aborted_rides++;
		}
		V(mappa->mutex);
		V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC - 1].availableSpace);
		P(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
		mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].taxiOnThisCell--;
		V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].availableSpace);
		V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
		exit(EXIT_FAILURE);
	}
	if (Ptemp(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex, so_timeout) == -1 && errno == EAGAIN)
	{
		P(mappa->mutex);
		if (ride_taken == TRUE)
		{
			mappa->aborted_rides++;
		}
		V(mappa->mutex);
		V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC - 1].availableSpace);
		V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC - 1].mutex);
		P(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
		mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].taxiOnThisCell--;
		V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].availableSpace);

		V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
		exit(EXIT_FAILURE);
	}

	mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].taxiOnThisCell--;
	mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC - 1].taxiOnThisCell++;
	mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC - 1].totalNumberOfTaxiPassedHere++;
	posizioneTaxi.posC--;

	V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC + 1].availableSpace);
	V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC + 1].mutex);
	V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);

	strada_percorsa++;
	tempo_in_strada = tempo_in_strada + time_struct.tv_nsec;
	P(taxi_statistiche->mutex);
	if (strada_percorsa > taxi_statistiche->strada_fatta)
	{
		taxi_statistiche->pid_strada_fatta = getpid();
		taxi_statistiche->strada_fatta = strada_percorsa;
	}
	if (tempo_in_strada > taxi_statistiche->tempo_in_strada)
	{
		taxi_statistiche->pid_tempo_in_strada = getpid();
		taxi_statistiche->tempo_in_strada = tempo_in_strada;
	}
	V(taxi_statistiche->mutex);
	nanosleep(&time_struct, NULL);
}

/*
	moveRight fa le stesse cose di moveUp con la differenza che il movimento è verso destra.
	L'altro dettaglio che cambia è l'ordine con cui si richiedono le risorse. L'ordine con cui vengono richieste le risorse è studiato per evitare il Deadlock (ulteriori informazioni vengono fornite
	all'interno della documentazione allegata a questi file - di preciso la si può trovare nella relazione del progetto).
*/
void moveRight(struct grigliaCitta *mappa)
{
	time_struct.tv_sec = 0;
	time_struct.tv_nsec = mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].timeRequiredToCrossCell;
	if (Ptemp(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC + 1].availableSpace, so_timeout) == -1 && errno == EAGAIN)
	{
		P(mappa->mutex);
		if (ride_taken == TRUE)
		{
			mappa->aborted_rides++;
		}
		V(mappa->mutex);
		P(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
		mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].taxiOnThisCell--;
		V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].availableSpace);

		V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
		exit(EXIT_FAILURE);
	}
	if (Ptemp(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex, so_timeout) == -1 && errno == EAGAIN)
	{
		P(mappa->mutex);
		if (ride_taken == TRUE)
		{
			mappa->aborted_rides++;
		}
		V(mappa->mutex);
		V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC + 1].availableSpace);
		P(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
		mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].taxiOnThisCell--;
		V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].availableSpace);

		V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
		exit(EXIT_FAILURE);
	}
	if (Ptemp(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC + 1].mutex, so_timeout) == -1 && errno == EAGAIN)
	{
		P(mappa->mutex);
		if (ride_taken == TRUE)
		{
			mappa->aborted_rides++;
		}
		V(mappa->mutex);
		V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC + 1].availableSpace);
		V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
		P(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
		mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].taxiOnThisCell--;
		V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].availableSpace);

		V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
		exit(EXIT_FAILURE);
	}

	mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].taxiOnThisCell--;
	mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC + 1].taxiOnThisCell++;
	mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC + 1].totalNumberOfTaxiPassedHere++;
	posizioneTaxi.posC++;

	V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC - 1].availableSpace);
	V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC - 1].mutex);
	V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);

	strada_percorsa++;
	tempo_in_strada = tempo_in_strada + time_struct.tv_nsec;
	P(taxi_statistiche->mutex);
	if (strada_percorsa > taxi_statistiche->strada_fatta)
	{
		taxi_statistiche->pid_strada_fatta = getpid();
		taxi_statistiche->strada_fatta = strada_percorsa;
	}
	if (tempo_in_strada > taxi_statistiche->tempo_in_strada)
	{
		taxi_statistiche->pid_tempo_in_strada = getpid();
		taxi_statistiche->tempo_in_strada = tempo_in_strada;
	}
	V(taxi_statistiche->mutex);
	nanosleep(&time_struct, NULL);
}

/*
	L'handler gestisce la fine del programma quando il processo riceve il sengale SIGALRM mandato dal SO
*/
void signalHandler(int signalNo)
{
	switch (signalNo)
	{
		case SIGALRM:
			exit_program = TRUE;
			break;
	}
}

/*
	Il funzionamento della funzione "closestSource" è il seguente: il taxi scorre la mappa e man mano che scorre si calcola la distanza tra lui e ogni cella di tipo source che trova,
	esegue un confronto fra le distanze e sceglie quella minore man mano che procede con la scansione della mappa.
*/
void closestSource(struct grigliaCitta *mappa)
{

	int i = 0, j = 0;
	int minDistance = INT_MAX;
	int positionOfMinDistance = 0, tmp;

	if (mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].cellType == SOURCE)
	{
		posizioneTaxi.destR = posizioneTaxi.posR;
		posizioneTaxi.destC = posizioneTaxi.posC;
		return;
	}

	for (i = 0; i < SO_HEIGHT; i++)
	{
		for (j = 0; j < SO_WIDTH; j++)
		{
			if (mappa->matrice[i][j].cellType == SOURCE)
			{
				tmp = (int) sqrt(pow(posizioneTaxi.posR - i, 2) + pow(posizioneTaxi.posC - j, 2));
				if (tmp < minDistance)
				{
					minDistance = tmp;
					posizioneTaxi.destR = i;
					posizioneTaxi.destC = j;
				}
			}
		}
	}
}
/*
	Il funzionamento della funzione "enumSoSources" è il seguente: il taxi scorre la mappa, quando incontra una cella di tipo SOURCE aumenta la variabile count_source.
	Mentre scorre la mappa il taxi esegue un confronto tra le coordinate in cui si trova e il numero della source, se sono uguali allora la cella su cui si trova è la n-esima
	dove n indica quante source sono state trovate prima di quella su cui è posizionato il taxi.
	La funzione è utile a capire su quale messaggio deve mettersi in ascolto il taxi una volta giunto alla source.
	In caso di errori la funzione ritorna -1
*/
int enumSoSources(struct grigliaCitta *mappa)
{
	int i, j;
	int count_source = 0;
	for (i = 0; i < SO_HEIGHT; i++)
	{
		for (j = 0; j < SO_WIDTH; j++)
		{
			if (mappa->matrice[i][j].cellType == SOURCE)
			{
				count_source++;
				if (i == posizioneTaxi.posR && j == posizioneTaxi.posC) return count_source;
			}
		}
	}
	return -1;
}
/*
	Il funzionamento della funzione "getRide" è il seguente: il taxi preleva un messaggio dalla coda, il tipo di messaggio da prelevare è specificato dalla enumSources mentre 
	le coordinate della destinazione sono generate nella costruzione del corpo del messaggio dai processi scrittori source generati dal master. (Per maggiori informazioni 
	si consulti la funzione signalHandler e si legga il case SIGALRM a riga 122) 
*/
void getRide(int msg_queue_id, long so_source)
{
	struct msgBuf myMessage;
	if (msgrcv(msg_queue_id, &myMessage, 2* sizeof(int), so_source, 0) == -1)
	{
		exit(EXIT_FAILURE);
	}
	posizioneTaxi.destR = myMessage.xDest;
	posizioneTaxi.destC = myMessage.yDest;
	ride_taken = TRUE;
}