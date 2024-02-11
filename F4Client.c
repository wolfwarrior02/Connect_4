/**
VR477731
*Laura Salinetti

VR471377
Michael Testini

VR471982
ALessandro Comai
2/2/2024
**/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#include <string.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define SHM_KEY 9230
#define SEM_KEY 456
#define MAX_NOMEGIOCATORE 100
#define MAX 15

//---------------------------------------VARIABILI GLOBALI E STRUCT-------------------------------------------------------------------------

struct CampoDaGioco{
	char campo[MAX][MAX];
	char gettone;
	int numGiocatori;
	int righe;
	int colonne;
	int mossa;
	pid_t giocatore1;
	pid_t giocatore2;
	pid_t server;
};

struct CampoDaGioco *memoria_condivisa;
struct sembuf sem_buf;
struct semid_ds sem_info;


int semid, shmid;
int giocatore = 0; //Questa variabile funge da identificatore. Se non viene modificato, vuol dire che non è iniziata la inizializzazione della partita
bool noErrore = true;

//---------------------------------------DICHIARAZIONE FUNZIONI--------------------------------------------------------------

void semOp (int semid, unsigned short sem_num, short sem_op);
void chiudi_client();
void connessione();
void errExit(const char *messaggio);
void mossa();
bool mossaNonValida(int mossa);
void stampaCampo();
void attesaTurno();
void chiusuraPartita();
void partita();
void gestoreSegnale(int sig);
void inizializzaSemafori();
void stringaErrore(const char *messaggio, bool modalita);

//---------------------------------------MAIN---------------------------------------------------------------------------------
int main(int argc, char *argv[]){

	//Stampa un messaggio di errore se non viene inserito il nome utente
	if(argc<2){
		stringaErrore("Hai scritto il comando in modo errato, segui il seguente aiuto:\n./F4Client nomeUtente\n",false);
	}

	char nomeGiocatore[MAX_NOMEGIOCATORE];
	strncpy(nomeGiocatore, argv[1],MAX_NOMEGIOCATORE);
	int singoloGiocatore=-1;
	
	if(strlen(nomeGiocatore)>100){
		stringaErrore("Hai dichiarato un nomeUtente troppo lungo! CARATTERI MASSIMI 100!\n",false);
	}
	
	//Inizializza i segnali interessati 
    	if (signal(SIGUSR2, gestoreSegnale) == SIG_ERR || signal(SIGINT, gestoreSegnale) == SIG_ERR ||signal(SIGUSR1, gestoreSegnale) == SIG_ERR || signal(SIGTERM, gestoreSegnale) == SIG_ERR){
		stringaErrore("inizializzaSegnali non ha avuto successo!\n",true);
	}
	
	inizializzaSemafori();
	printf("Ciao %s!Stai giocando a FORZA 4!\nAspettando che si connette il server...\n",nomeGiocatore);
	semOp(semid,3,-1);
	if(semctl(semid, 0, IPC_STAT, &sem_info) == -1) { //nel caso in cui uno dei due client in attesa viene chiuso (e di conseguenza elimina i semafori), anche l'altro viene chiuso facendo comparire un errore
               stringaErrore("semctl non ha avuto successo!",true); 
        }
        
	printf("Il server si è connesso!\n");
	connessione();	
	memoria_condivisa->numGiocatori ++;	
	if(memoria_condivisa->numGiocatori > 2){ //Questo controllo serve per assicurarsi che ci siano massimo 2 giocatori a giocare.
		printf("Al momento non puoi giocare! Riprova più tardi!\n");
		chiudi_client();
	}
	

	//Stampa un messaggio di errore se vengono inseriti meno o più di quattro parametri
	if(argc > 3 || argc == 1){
		printf("Hai scritto il comando in modo errato, segui il seguente aiuto:\n./client nomeGiocatore\n");
		chiudi_client();
	}else if(argc == 3){ //Se non ci sta già un giocatore che sta aspettando, allora si procede a creare la partita contro il bot
		singoloGiocatore=strcmp(argv[2],"*");
		if(memoria_condivisa->numGiocatori == 1){
			if(singoloGiocatore == 0){
				pid_t bot;
				printf("Stai giocando contro un bot!\n");
				if((memoria_condivisa->giocatore1 = getpid()) == -1){ //Si ottiene il pid del giocatore e lo si salva in memoria condivisa
					errExit("getpid non ha avuto successo!\n");
				}
				if((bot = fork()) == -1){ //si crea un processo figlio
					errExit("fork non ha avuto successo!\n");
				}
					
				if(bot == 0){ //Il processo figlio (bot) procede ad eseguire il file bot.c
					char *args[] = {"./bot", NULL};
					if(execvp(args[0],args) == -1){
						errExit("execvp non ha avuto successo!\n");
					}
					exit(EXIT_SUCCESS);
				}
			}else{
				noErrore = false;
				printf("Hai inserito un carattere non valido!\n");
				semOp(semid,3,1);
				chiudi_client();
			}
		}else{
			noErrore = false;
			if(singoloGiocatore != 0){
				printf("Hai inserito un carattere non valido!\n");
			}
			printf("Non puoi giocare contro un bot perchè ci sta un giocatore che ti sta aspettando!\n");
			chiudi_client();
		}
	}

	fflush(stdout);	
	if(memoria_condivisa->numGiocatori == 1){
		giocatore = 1;
		if((memoria_condivisa->giocatore1 = getpid()) == -1)
			errExit("getpid non ha avuto successo!\n");
		printf("Il tuo gettone e' %c\nAspettando che si connetta un altro giocatore...\n",memoria_condivisa->gettone);
		if (singoloGiocatore==0){
			memoria_condivisa->numGiocatori++;
		}
		semOp(semid,0,1); //Sblocca il server, così che cambia il gettone per il secondo giocatore
		semOp(semid,2,1);
		semOp(semid,giocatore,-1);
		printf("E' stato trovato un giocatore!\nAspettando la prima mossa dell'altro giocatore...\n");
		
	}else{ //Entra qui solamente se si è connesso un altro giocatore (e non il bot)
		giocatore = 2;
		if((memoria_condivisa->giocatore2 = getpid()) == -1)
			errExit("getpid non ha avuto successo!\n");
		printf("Il tuo gettone e' %c\nPartita iniziata!\nFai la tua prima mossa!\n",memoria_condivisa->gettone);
		stampaCampo();
		semOp(semid,0,1); //Sblocca il server per notificargli che sono entrati due giocatori.
		semOp(semid,3,1); //Serve per controllare se entra un altro client e per chiuderlo.
		semOp(semid,1,1); //libera il primo giocatore per notifigargli che è entrato il secondo giocatore
	}
	
	semOp(semid,giocatore,-1); //il secondo giocatore viene lasciato fare il primo turno. Il primo giocatore aspetta.
  	
  	partita();
  	chiusuraPartita();
        
  	printf("Partita finita!\n");
	chiudi_client();
}

//-------------------------------------GIOCA--------------------------------------
void partita(){
	while(memoria_condivisa->mossa>=0){ 
  		system("clear");	
	  	stampaCampo();  	
	  	mossa();
	  	attesaTurno(giocatore);	
  	}
}

//----------------------------------------CHIUSURA PARTITA----------------------------
void chiusuraPartita(){
	system("clear");
  	stampaCampo();
   	
   	//Il server restituisce un numero negativo quando la partita finisce. -1 e -2 indicano rispettivamente chi ha vinto dei due.
        if(memoria_condivisa->mossa==-1){
	  	if(giocatore == 1)
	  		printf("Hai vinto!\n");
	  	else
	  		printf("Hai perso!\n");
	  		
	}else{
		if(memoria_condivisa->mossa==-2){  
	  		if(giocatore == 2){
	  			printf("Hai vinto!\n");
	  		}else{
	  			printf("Hai perso!\n");
	  		}
		}else{
			printf("Partita conclusa in pareggio!\n");
		}
	}
}

//---------------------------------------SEMAFORO PER LA DECISIONE DEL TURNO------------------------

void attesaTurno(){
	
	system("clear");
	printf("Mossa inserita:%i\n",memoria_condivisa->mossa+1);
	printf("In attesa della mossa dell'altro giocatore...\n");
	semOp(semid, 0, 1);
	semOp(semid, giocatore,-1);
}

//----------------------------------------STAMPA CAMPO------------------------------------------------

void stampaCampo(){
	for (int i = 0; i < memoria_condivisa->righe; i++) {
    		for (int j = 0; j < memoria_condivisa->colonne; j++) {
      			printf("[%c]",memoria_condivisa->campo[i][j]); 
    		}
    		printf("\n");
	}
}
  	
//---------------------------------------MOSSA---------------------------------------------------------

void mossa(){
	int mossa;
	bool stato = false;
	
	printf("E' il tuo turno!\n");
	
	do{	
		stato = false;	
		printf("Inserisci la tua mossa(%c): ",memoria_condivisa->gettone);
		if(scanf("%d", &mossa)!=1){
				stato = true;
				while (getchar()!='\n');
				fflush(stdout);
				printf("Hai inserito un carattere, ATTENZIONE!\n");
		}else{
			if(mossa<=0 || mossa>memoria_condivisa->colonne){
				printf("La mossa inserita non è valida!\n");
				stato = true;
			}else{
				if(memoria_condivisa->campo[0][mossa-1]!=' '){
				 	printf("La colonna è piena, scegli un'altra colonna!\n");
				 	stato = true;
				}
			}
		}
		
		fflush(stdout);	
	}while(stato);
	
	mossa--;
	memoria_condivisa->mossa=mossa;
}

//---------------------------------------SEMOP------------------------------------------------------------

void semOp (int semid, unsigned short sem_num, short sem_op) {
	struct sembuf sop = {.sem_num = sem_num, .sem_op = sem_op, .sem_flg = 0};
	bool here = false;
		
    	while (semop(semid, &sop, 1) == -1 && errno == EINTR){ //Il processo continua ad aspettare finchè viene interrotto da un segnale
    		here = true; //Questa assegnazione viene fatta per assicurarmi che se avviene un errore è perchè è successo qualcosa con la system call del semaforo.
        }
        
        if(errno != EINTR && here){
        	errExit("semop non ha avuto successo");
        }
        	     	
}


//------------------------------------------GESTORE SEGNALE----------------------------------

void gestoreSegnale(int sig){
    switch(sig){
        case SIGTERM:
        case SIGINT:
            if(giocatore == 1) {
                printf("\nHai abbandonato la partita.\n");
                kill(memoria_condivisa->server, SIGUSR1);      
                chiudi_client();
            }else{
            	if(giocatore == 2){
		        printf("\nHai abbandonato la partita.\n");
		        kill(memoria_condivisa->server, SIGUSR2);
		        chiudi_client();
		}else{			
			//Rimuove i semafori
			if(semctl(semid, 0, IPC_RMID, 0) == -1){
				errExit("semctl non ha avuto successo!\n");
			}
			exit(EXIT_SUCCESS);
		}
            }
            break;
        case SIGUSR1:
            printf("\nL'avversario ha abbandonato la partita.\n Hai vinto!\n");
            chiudi_client();
            break;
        case SIGUSR2:
            printf("\nIl gioco è stato terminato per cause esterne.\n");
            chiudi_client();
            break;
    }
}

//---------------------------------------CONNESSIONE--------------------------------------------------------

void connessione(){

	//NOTA!Bisogna chiudere manualmente  nel caso in cui avvenga errori nella creazione della memoria condivisa		
        // Ottieni l'ID della memoria condivisa
    	shmid = shmget(SHM_KEY, sizeof(struct CampoDaGioco), IPC_CREAT | 0666);
    	if (shmid == -1){
        	stringaErrore("shmget non ha avuto successo!\n",true);
        }
	
    	// Collega la memoria condivisa al processo
    	memoria_condivisa = (struct CampoDaGioco*)shmat(shmid, NULL, 0);
    	if (memoria_condivisa == (struct CampoDaGioco *)-1){
        	stringaErrore("shmget non ha avuto successo!\n",true);
        }	
}

//-------------------------------------INIZIALIZZA SEMAFORI-----------------------------------------
void inizializzaSemafori(){

    	semid = semget(SEM_KEY, 4, IPC_CREAT | 0666);
    	if (semid == -1) {
        	errExit("semget non ha avuto successo!\n");
    	}
    	
   	 //Copia i semafori inizializzati 
    	if(semctl(semid, 0, IPC_STAT, &sem_info) == -1) { 
               stringaErrore("semctl non ha avuto successo!",true); 
        }   		
}

//---------------------------------------CHIUDI CLIENT--------------------------------------------------------

void chiudi_client(){
	//Questo controllo serve per assicurarsi di due cose: l'ultimo client rimasto accesso libera il server così che si possa chiudere...
	//... se il secondo client tenta di accedere dichiarando il bot, viene fatto si che non liberi il server (che è occupato con la partita)
	memoria_condivisa->numGiocatori--;	
	if(memoria_condivisa->numGiocatori == 0 && noErrore){ 
		if(shmdt(memoria_condivisa)== -1){ 
			errExit("shmdt non ha avuto successo!");
		}
		semOp(semid, 0, 1);
	}else{		
		if(shmdt(memoria_condivisa)== -1){
			errExit("shmdt non ha avuto successo!");
		}	
	}
	
	semOp(semid,3,1);
	exit(EXIT_SUCCESS);
}

//---------------------------------STRINGA ERRORE------------------------------------------------
void stringaErrore(const char *messaggio, bool modalita){
	if(modalita){
		perror(messaggio);
		exit(EXIT_SUCCESS);
	}else{
		printf("%s\n",messaggio);
		exit(EXIT_SUCCESS);
	}
}

//---------------------------------------ERREXIT---------------------------------------------------------------

void errExit(const char *messaggio) { 
    	perror(messaggio);
    	
    	if(giocatore == 1){
    		kill(memoria_condivisa->server, SIGUSR1);
   	}else{
   		kill(memoria_condivisa->server, SIGUSR2);
   	}
    	
    	chiudi_client();
}

