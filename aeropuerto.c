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
//#include<pthreads.h>

void nuevoUsuario(int sig);
void accionesUsuario(int sig);
void accionesFacturador(int sig);
void accionesAgenteSeguridad(int sig);
void inicializalog();
void inicializarmutex();
void WriteLogMessage(char *id, char *msg);

//declaraciones globales
//mutex y variables de condicion
//lista de 10 usuarios (id facturado atendido tipo
//usuario en control 
FILE *logFile;
char *logFileName="log.txt";


//Mutex

pthread_mutex_t mId;
pthread_mutex_t mFacturado;
pthread_mutex_t mAtendido;
pthread_mutex_t mTipo;

struct usuario{

	int id; //id del usuario
	int facturado;
	int atendido;
	int tipo;
	pthread_t usuario;

};
//Array de los atletas
struct usuario us [USUARIOS];

int main(int argc, char *argv[]){

	inicializalog();
	inicializarmutex();

}

void nuevoUsuario(int sig){

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

void accionesUsuario(int sig){
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

void accionesFacturador(int sig){

}

void accionesAgenteSeguridad(int sig){

}

void inicializalog(){

	logFile=fopen(logFileName,"w");
	
	fclose(logfile);

	writeLogMessage("Sistema", "Log iniciado.");

}
void inicializarmutex(){

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

void WriteLogMessage(char *id, char *msg){

	time_t now =time(0);
	struct tm *tlocal = localtime(&now);
	char stnow[19];
	strftime(stnow,19, "%d/%m/ %y %H:%M:%S", tlocal);
	logFile = fopen(logFileName, "a");
	fprintf(logFile,"[%s] %s: %s\n", stnow, id,msg);
	fclose(logFile);

}
