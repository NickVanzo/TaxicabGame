TaxicabGame

IMPORTANTE: LE FUNZIONI DI MASTER, TAXI E CONTROL LE DEFINIAMO DENTRO IL RELATIVO FILE C.  SOLO LE FUNZIONI CHE VENGONO USATE OVUNQUE VENGONO DICHIARATE DENTRO include_main.h E IMPLEMENTAATE IN FILE SEPARATI APPOSTA.
            P.S. SCUSA IL GRASSETTO MA NON SO SE LEGGI PRIMA O SE PRIMA NE PARLIAMO :P

================= MASTER ====================

è il file che genera la mappa









================ TAXI =====================

è il processo che si muove nella mappa creata da master










============== CONTROL ====================

programma che mette roba dentro la coda di messaggi per nuove richieste




============= APPUNTI NICK =================

generazione random di: source e buchi
default mappa: strada
modifica dopo creazione
Parametri OK:
./master 1000 190 10 1 100 800000000 900090000 40 1000 5

./master 100 190 10 1 100 800000000 900090000 40 1000 20

Parametri dense
./master 95 190 10 3 5 100000000 300000000 40 1000 20

Domanda da fare a Bini:

I ) La funzione atexit() viene eseguita quando lancio un control + C da terminale 
II) Controllare se i processi morti usando come parametri la large siano corretti (media di 20MILA morti)
III) le SO_TOP_CELLS devono includere le SO_SOURCES oppure devono essere solamente celle di tipo ROAD?
IV) signal al posto di sigaction per impostare l'handler dei segnali in source.c?
