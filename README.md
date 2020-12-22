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
	

Domanda da fare a Bini:

III) le SO_TOP_CELLS devono includere le SO_SOURCES oppure devono essere solamente celle di tipo ROAD?
Risposta in generale.
IV) signal al posto di sigaction per impostare l'handler dei segnali in source.c?
Usare la sigaction
V) Quando bisogna consegnare il progetto entrambi gli studenti devono consegnarlo nel google form? (nel caso di studenti di turni diversi)
Fa tutto la google form. Risposta in generale.