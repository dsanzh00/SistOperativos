#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<errno.h> 
#include<sys/wait.h>
#include<signal.h>
#include<pthread.h>
#include<time.h>
#include<sys/types.h>
#include<sys/syscall.h>
#include<signal.h>
#include<ctype.h>
#include<string.h>
#define USUARIOS 10
#define COLA 1
//#include<pthreads.h>

void nuevoUsuario(int sig);
void accionesUsuario(int sig);
void matamosHilo(int posicion);
void control(int id);
void accionesFacturador(int sig);
void accionesAgenteSeguridad(int sig);
void inicializaLog();
void inicializaMutex();
void inicializaGlobales();
void inicializarUsuarios();
void WriteLogMessage(char *id, char *msg);


//Declaraciones globales
int contUsuarios; 		// Numero de usuarios que han pasado
int totalUsuarios;		// Numero total de usuarios normales y vip que han pasado
int admite; 			// A ver si acepta mas usuarios
int totalAtendidos; 		// Numero de usuarios que han sido atendidos
int controlSeguridad		// Usuarios que llegan al control de seguridad
int idControl			// Guarda ID del ultimo en el control 
int totalEmbarcados; 		// Numero de usuarios que han embarcado
int sigint; 			// Variable para saber si acabamos el prg o aun quedan usuarios por comprobar
int cabinasfact;



//lista de 10 usuarios (id facturado atendido tipo
//usuario en control 
FILE *logFile;
char *logFileName="log.txt";


//Mutex

pthread_mutex_t mId;
pthread_mutex_t mFacturado;
pthread_mutex_t mEscritura;
pthread_mutex_t mAtendido;
pthread_mutex_t mTipo;

struct usuario{

	int id; //id del usuario
	int tipo;
	int cola;	
	int atendido;	
	int facturado;

	pthread_t usuario;

};

//Array de los atletas
struct usuario us [USUARIOS];


int main(int argc, char *argv[]){
	srand(time(NULL));
	if(argc>2){
		exit(0);
	}
	else{
		cabinasfact=2;
		if(argc>1){
			cabinasfact=atoi(argv[1]);
		}
		if(signal(SIGUSR1, nuevoUsuario) == SIG_ERR){//señal para un usuario no vip
			perror("Error en el envio de la senial SIGUSR1");
			exit(-1);
		}

		if(signal(SIGUSR2, nuevoUsuario) == SIG_ERR){//señal para un usuario vip
			perror("Error en el envio de la senial SIGUSR2");
			exit(-1);
		}
		if(signal(SIGINT, finPograma) == SIG_ERR){
			perror("Error en el envio de la senial SIGINT");
			exit(-1);
		}
		inicializaMutex();
		inicializaGlobales();
		inicializaUsuarios();
		inicializaLog();
		//vamos a crear las mesas de facturacion
		int i;
		for(i=0;i<cabinasfact;i++){
			//Creamos los hilos
			pthread_t facturadoes;
			pthread_create(&facturadores,NULL,accionesFacturador,(void *)&i);
			sleep(1);
		}		
		//Bucle para que coja seniales infinitamente
		while(1){
			pause();
		}
		return 0;
	}

}

void nuevoUsuario(int sig){

	if(signal(SIGUSR1, nuevoUsuario) == SIG_ERR){
		exit(0);
	}

	if(signal(SIGUSR2, nuevoUsuario) == SIG_ERR){
		exit(0);
	}

	if(contUsuarios<USUARIOS && sigint==0){

		pthread_mutex_lock(&mAtendido);
		totalUsuarios++;
		us[contUsuarios].id = totalUsuarios;
		if(USUARIOS){
			us[contUsuarios].cola = 1; 
		} //esta parte hay que mirarla bien.

		pthread_mutex_lock(&mEscritura);
		char id[20];
		sprintf(id, "El usuario %d entra en la cola", us[contUsuarios].id);
		writeLogMessage(id);
		pthread_mutex_lock(&mEscritura);
		// creación del hilo del usuario
		pthread_create(&us[contUsuarios].usuario, NULL, accionesUsuario, NULL);
	}


/*
1. Comprobar si hay espacio en la lista de facturación
	Si lo hay:
		Se añade el usuario, el contador aumenta
		nuevoUsuario_Id = contadorUsuarios.
		nuevoUsuario_atendido = 0;
		tipo = normal(SIGUSR1) o VIP (SIGUSR2)
		Creamos el hilo

	Si NO lo hay:
		Se ignora. 


*/


}

void *accionesUsuario(void* posicion){

	int aleatorio, i, pos;
	pos = contUsuarios;
	int id = us[pos].id
	contUsuarios++;

	/* Mientras no han sido atendidos comprobamos (cada 3 seg) si necesitan ir al baño y se van */

	while(us[pos].atendido == 0){

		aleatorio = rand()%100;

		if(aleatorio  < 10){
	
		/* Si el numero pertenece al 10% ese usuario abandona la cola porque se va al baño y lo registramos en el log */
		/* ¿Habrá que repetir para los que se cansan? */	

			pthread_mutex_lock(&mEscritura);
			us[pos].atendido = 2;
			char id1[20];
			char msg1[200];
			sprintf(id1, "El usuario_%d ", id);
			sprintf(msg1, "se va de la cola porque necesita ir al baño");
			writeLogMessage(id1, msg1);
			pthread_mutex_unlock(&mEscritura);
			contUsuarios--;
			matamosHilo(pos);
		}else{	
		
			pthread_mutex_unlock(&mAtendido);
			sleep(3);
		}
	
	pthread_mutex_lock(&mAtendido);
	for(i=0; i<USUARIOS; i++){
		if(us[i].id == id){
			pos = i;
		}
	}

	} // Se acaba el while

	while(us[pos].atendido == 1){
		pthread_mutex_unlock(&mAtendido);
		pthread_mutex_lock(&mAtendido);
	
		for(i=0; i<USUARIOS; i++){
			if(us[i].id == id){
				pos = i;
			}	
		}
	}

	if(us[pos].facturado == 1){
		/* Mandamos al usuario a control de seguridad */
		control(us[pos].id);
	}else{
		/* Ha fallado el visado y te piras*/
		matamosHilo(pos);
	}


/*

1. Guarda en el log la hora de entrada
2. Guarda en el log el tipo de usuario
3. Duerme 4 segundos
	Comprueba si está siendo atendido
 		Si NO está atendido, se mira si se pira (Hilo eliminar) 
 			Se libera espacio de la cola
		Se está atendido, espera a que termine.
			puedes estar atendido y pasas a control. 
				liberas cola del facturador
			te puede joder el visado y que te piras. 

*/

}

void matamosHilo(int posicion){

	int i;

	for(i=posicion; i<USUARIOS-1; i++){

		us[i].id = us[i+1].id;
		us[i].facturado = us[i+1].facturado;
		us[i].cola = us[i+1].cola;
		us[i].atendido = us[i+1].atendido;
		us[i].tipo = us[i+1].tipo;
	}

	us[USUARIOS-1].id = 0;
	us[USUARIOS-1].facturado = 0;
	us[USUARIOS-1].cola = 0;
	us[USUARIOS-1].atendido = 0;
	us[USUARIOS-1].tipo = 0;	

	totalAtendidos++;
	pthread_mutex_unlock(&mAtendido);
	pthread_exit(NULL);

}

void control(int id){
	int i;
	
	/* Si no hay nadie en el control, entra */
	if(controlSeguridad == 0){
		controlSeguridad = 1;
		idControl = id;
	}else{
		/* Si el control está ocupado */
		pthread_mutex_lock(&mEscritura);
		char a[100];
		char msg[200];
		sprintf(a, "El usuario_%d ", id);
		sprintf(msg, "esta esperando a que el usuario_%d acabe", idControl);
		writeLogMessage(a, msg);
		pthread_mutex_unlock(&mEscritura);
		idControl = id;
	}
}

void *accionesFacturador(void* numfact){

}

void accionesAgenteSeguridad(int sig){

}

void inicializaLog(){

	logFile=fopen(logFileName,"w");
	
	fclose(logfile);

	writeLogMessage("Sistema", "Log iniciado.");

}
void inicializaMutex(){

	if(pthread_mutex_init(&mId,NULL)!=0){
		exit(-1);
	}
	if(pthread_mutex_init(&mFacturado,NULL)!=0){
		exit(-1);
	}
	if(pthread_mutex_init(&mAtendido,NULL)!=0){
		exit(-1);
	}
	if(pthread_mutex_init(&mTipo,NULL)!=0){
		exit(-1);
	}
}


void inicializaGlobales(){

	admite=1;
	contUsuario=0;
	atendido=0;
	idAtendido=0;
	totalUsuarios=0;
	sigint=0;
	totalEmbarcados=0;

}
void WriteLogMessage(char *id, char *msg){

	time_t now =time(0);
	struct tm *tlocal = localtime(&now);
	char stnow[19];
	strftime(stnow,19, "%d/%m/ %y %H:%M:%S", tlocal);
	logFile = fopen(logFileName, "a");
	fprintf(logFile,"[%s] %s: %s\n", stnow, id,msg);
	fclose(logFile);

}
void inicializarUsuarios(){
	int i;
	for(i=0;i<USUARIOS;i++){
		us[i].id=0;
		us[i].facturado=0;
		us[i].atendido=0;
		us[i].tipo=0;
	}

	//aqui falta algo

}
