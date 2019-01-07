/*

		PRACTICA FINAL 
		SISTEMAS OPERATIVOS 
		GRUPO 40

*/

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
#define COLA 2
//#include<pthreads.h>

void nuevoUsuario(int sig);
void *accionesUsuario(void* posicion);
void matamosHilo(int posicion);
void control(int id);
void *accionesFacturador(void* numfact);
void accionesAgenteSeguridad(int usuario_id);
void inicializaLog();
void inicializaMutex();
void inicializaGlobales();
void inicializaUsuarios();
void writeLogMessage(char *id, char *msg);
void finPrograma(int sig);
void Visado_Incorrecto(int usuario_id);
void Exceso_Peso(int usuario_id);

//Declaraciones globales
int contUsuarios; 		// Numero de usuarios que han pasado
int totalUsuarios;		// Numero total de usuarios normales y vip que han pasado
int admite; 			// A ver si acepta mas usuarios
int totalAtendidos; 		// Numero de usuarios que han sido atendidos
int controlSeguridad;		// Usuarios que llegan al control de seguridad
int idControl;			// Guarda ID del ultimo en el control 
int totalEmbarcados; 		// Numero de usuarios que han embarcado
int sigint; 			// Variable para saber si acabamos el prg o aun quedan usuarios por comprobar
int cabinasfact;
int atendidos1;			// Numero de usuarios atendidos por el facturador1
int atendidos2;			// Numero de usuarios atendidos por el facturador2


//lista de 10 usuarios (id facturado atendido tipo
//usuario en control 
FILE *logFile;
char *logFileName="log.txt";


//Mutex

pthread_mutex_t mId;
pthread_mutex_t mEscritura;
pthread_mutex_t mCola;
pthread_mutex_t mControl;


struct usuario{

	int id; //id del usuario
	int tipo; //1=normal 2=vip
	int cola;	
	int atendido;	//0=no atendido, 1=le estan atendiendo, 2=atendido
	int facturado;	//0=no facturado, 1=facturado
	int cacheado;	//0=no cacheado, 1=cacheado

	pthread_t usuario;

};

//Array de los usuarios
struct usuario us [USUARIOS];


int main(int argc, char *argv[]){
	srand(time(NULL));
	
		cabinasfact=2;

		if(signal(SIGUSR1, nuevoUsuario) == SIG_ERR){//senial para un usuario no vip: kill -s 10 (pid del programa)
			perror("Error en el envio de la senial SIGUSR1");
			exit(-1);
		}

		if(signal(SIGUSR2, nuevoUsuario) == SIG_ERR){//senial para un usuario vip: kill -s 12 (pid del programa)
			perror("Error en el envio de la senial SIGUSR2");
			exit(-1);
		}
		if(signal(SIGINT, finPrograma) == SIG_ERR){//senial para finalizar el programa: kill -s 2 (pid del programa)
			perror("Error en el envio de la senial SIGINT");
			exit(-1);
		}
		inicializaMutex(); //Inicializamos los semaforos
		inicializaGlobales(); //Inicializamos las variables globales
		inicializaUsuarios(); //Inicializamos las variables de cada usuario
		inicializaLog(); //Inicializamos el log
		//vamos a crear las mesas de facturacion
		int i;
			//Creamos los hilos
			for(i=0; i<cabinasfact; i++){
			pthread_t facturadores;
			pthread_create(&facturadores,NULL,accionesFacturador,(void *)&i);
			sleep(1);
		}		
		//Bucle para que coja seniales infinitamente
		while(1){
			pause();
		}
		return 0;
	

}

void nuevoUsuario(int sig){

	if(signal(SIGUSR1, nuevoUsuario) == SIG_ERR){
		exit(0);
	}

	if(signal(SIGUSR2, nuevoUsuario) == SIG_ERR){
		exit(0);
	}

	if(contUsuarios<USUARIOS && sigint==0){//Si en la cola hay 10 usuarios no recoge ninguna senial

		pthread_mutex_lock(&mCola); //Abrimos la cola
		totalUsuarios++; //Aumentamos el numero de usuarios de la cola
		us[contUsuarios].id = totalUsuarios; //El id del nuevo usuario es el siguiente al ultimo
		switch(sig){
			case SIGUSR1: //Si la senial es SIGUSR1 el usuario es no vip y accede a la cola1
				us[contUsuarios].cola = 1;
				us[contUsuarios].tipo = 1;
			break;
			case SIGUSR2: //Si la senial es SIGUSR1 el usuario es vip y accede a la cola2
				us[contUsuarios].cola = 2;
				us[contUsuarios].tipo = 2;
			break;
		}
		pthread_mutex_lock(&mEscritura); //Abrimos la escritura del log
		char id[20];
		char msg[200];
		sprintf(id, "El usuario_%d ", us[contUsuarios].id);
		sprintf(msg, "Entra en la cola %d",us[contUsuarios].cola);
		writeLogMessage(id, msg);
		pthread_mutex_unlock(&mEscritura); //Cerramos la escritura del log
		// creación del hilo del usuario
		pthread_create(&us[contUsuarios].usuario, NULL, accionesUsuario, NULL);
	}

}

void *accionesUsuario(void* posicion){

	int aleatorio, i, pos;
	pos = contUsuarios;
	int id = us[pos].id;
	contUsuarios++;

	/* Mientras no han sido atendidos comprobamos (cada 3 seg) si necesitan ir al banio y se van */
	
	while(us[pos].atendido == 0){ //Accede aqui si el usuario no ha sido atendido

		aleatorio = rand()%100;

		if(aleatorio  < 10){ //El 10% se van de la cola porque van al banio
	
		// Si el numero pertenece al 10% ese usuario abandona la cola porque se va al banio y lo registramos en el log 
			
			pthread_mutex_lock(&mEscritura); //Abrimos la escritura del log
			us[pos].atendido = 2;
			char id1[20];
			char msg1[200];
			sprintf(id1, "El usuario_%d ", id);
			sprintf(msg1, "Se va de la cola porque necesita ir al banio");
			writeLogMessage(id1, msg1);
			pthread_mutex_unlock(&mEscritura); //Cerramos la escritura del log
			contUsuarios--;
			matamosHilo(pos); //Matamos el hilo

		//Si el numero esta entre el 10 y el 30 ese usuario abandona la cola porque se cansa de esperar lo registramos en el log 

		}else if((aleatorio>=10)&&(aleatorio<=30)){
			pthread_mutex_lock(&mEscritura); //Abrimos la escritura del log
			us[pos].atendido=2;
			char id1[20];
			char msg1[120];
			sprintf(id1, "El usuario_%d ", id);
			sprintf(msg1,"Se va de la cola porque se ha cansado de esperar");
			writeLogMessage(id1, msg1);
			pthread_mutex_unlock(&mEscritura); //Cerramos la escritura del log
			contUsuarios--;
			matamosHilo(pos); //Matamos el hilo
		}else{	
			pthread_mutex_unlock(&mCola); //Cerramos la cola
			sleep(3);
		}
	
	pthread_mutex_lock(&mCola); //Abrimos la cola
	for(i=0; i<USUARIOS; i++){ //Buscamos al usuario
		if(us[i].id == id){
			pos = i;
		}
	}

	} // Se acaba el while

	while(us[pos].atendido == 1){ //Mientras el usuario esta siendo atendido
		pthread_mutex_unlock(&mCola); //Cerramos la cola
		pthread_mutex_lock(&mCola); //Abrimos la cola
	
		for(i=0; i<USUARIOS; i++){
			if(us[i].id == id){
				pos = i;
			}	
		}
	}

	if(us[pos].facturado == 1){ //Si el usuario ha facturado correctamente
		/* Mandamos al usuario a control de seguridad */
		control(us[pos].id);
	}
	else{
		/* Ha fallado el visado y te piras*/
	}
	while(us[pos].cacheado==1){ //Mientras el usuario este siendo cacheado
		pthread_mutex_unlock(&mControl); //Cerramos la cola de seguridad
		pthread_mutex_lock(&mControl); //Abrimos la cola de seguridad
	
		for(i=0; i<USUARIOS; i++){
			if(us[i].id == id){
				pos = i;
			}	
		}
	}
		matamosHilo(pos); //Cuando embarque, haya ido al banio, se haya ido de la cola, no haya  podido facturar o embarcar muere

}

void matamosHilo(int posicion){

	int i;

	for(i=posicion; i<USUARIOS-1; i++){ //Recolocamos el array para echar al hilo en cuestion

		us[i].id = us[i+1].id;
		us[i].facturado = us[i+1].facturado;
		us[i].cola = us[i+1].cola;
		us[i].atendido = us[i+1].atendido;
		us[i].tipo = us[i+1].tipo;
	}
	//Reiniciamos a 0 la ultima posicion
	us[USUARIOS-1].id = 0;
	us[USUARIOS-1].facturado = 0;
	us[USUARIOS-1].cola = 0;
	us[USUARIOS-1].atendido = 0;
	us[USUARIOS-1].tipo = 0;	

	totalAtendidos++; //Aumentamos en uno el numero de atendidos
	pthread_mutex_unlock(&mCola); //Cerramos la cola
	pthread_exit(NULL);

}

void control(int id){
	sleep(1);
	int control[50]; 
	int i, pos;
	int usuarios_control=0; //Numero de usuarios en la cola de seguridad
	control[usuarios_control]=id;
	usuarios_control++; //Buscamos al usuario en cuestion
		for(i=0; i<USUARIOS; i++){
			if(us[i].id == id){
				pos = i;
			}	
		}
	/* Si no hay nadie en el control, entra */
	if(controlSeguridad == 0){
		controlSeguridad = 1;
		pthread_mutex_lock(&mControl); //Abrimos la cola de seguridad
		us[pos].cacheado=1;
		accionesAgenteSeguridad(control[0]); //Le enviamos al cacheo
		idControl = id;
		pthread_mutex_unlock(&mControl); //Cerramos la cola de seguridad
		for(i=0;i<usuarios_control;i++){
			control[i]=control[i+1]; //Recolocamos el array echando al usuario cacheado
		}
		usuarios_control--; //Disminuimos en uno el numero de usuarios en la cola para el control de seguridad
		controlSeguridad=0;
	}else{
		/* Si el control está ocupado */
		pthread_mutex_lock(&mEscritura); //Abrimos la escritura del log
		char a[100];
		char msg[200];
		sprintf(a, "El usuario_%d ", id);
		sprintf(msg, "Esta esperando a que pare el control de seguridad");
		writeLogMessage(a, msg);
		pthread_mutex_unlock(&mEscritura); //Cerramos la escritura del log
		idControl = id;
	}
}

void *accionesFacturador(void* numfact){

	int facturadores = *(int *) numfact+1 ; //sirve para saber que facturador es
	int aleatorio;
	int contador1 =0;
	int contador2=0;
	int libre;
	int i;
	int pos;

	pthread_mutex_lock(&mEscritura); //Abrimos la escritura del log

	char in[10];
	sprintf(in, "%d", facturadores);
	writeLogMessage("Se ha montado la mesa de facturacion ", in);
	pthread_mutex_unlock(&mEscritura); //Cerramos la escritura del log
	int usuario_id;
	while(admite==1){
		libre=0;
		//Bucle para la busqueda de un usuario que no este siendo atendido y este en la cola
		while(libre==0){//Comienzo while2
				pthread_mutex_lock(&mCola); //Abrimos la cola
				

				for(i=0;i<contUsuarios;i++){
					if(libre==0){
					//Comprobamos que el usuario exista
						if(us[i].id!=0){
					//Comprobamos que el usuario exista
							if(us[i].atendido==0){
								//Comprobamos si tiene asignada la cola asignada y se esta ejecutando el hilo correcto
								if(us[i].cola==facturadores){
									libre=1; //Indicamos que se ha encontrado un usuario que va a facturar
									usuario_id=us[i].id; //Guardamos el id del usuarios que se dispone a facturar
									us[i].atendido=1; //Actualizamos su estado a: facturando
									contador1++; //Aumentamos en 1 el contador que permitira al facturador descansar
									pos=i; //guardamos la posicion en el array del usuario
								}
							}

						}
					}

				}

				sleep(1);

				for(i=0;i<contUsuarios;i++){
					if(libre==0){
					//Comprobamos que el usuario exista
						if(us[i].id!=0){
						//Comprobamos que el usuario exista
							if(us[i].atendido==0){
								libre=1; //Indicamos que se ha encontrado un usuario que va a facturar
								usuario_id=us[i].id;//Guardamos el id del usuarios que se dispone a facturar
								us[i].cola=facturadores; //Le asignamos la cola
								us[i].atendido=1; //Actualizamos su estado a: facturando
								contador1++; //Aumentamos en 1 el contador que permitira al facturador descansar
								pos=i; //guardamos la posicion en el array del usuario
							}

						}					
					}
				}
			
			pthread_mutex_unlock(&mCola);//Cerramos la cola
		}//final while2
		sleep(4);
		aleatorio=rand()%100;
		int dormir;
		if(aleatorio<90){ //El 90% de los usuarios pasa el control
		pthread_mutex_lock(&mEscritura); //Abrimos la escritura del log
		char id[20];
		char msg[120];
		sprintf(id, "El usuario_%d ", usuario_id);
		sprintf(msg, "Entra a facturar en el mostrador %d.", facturadores); //Le asignamos al usuario un mostrador
		writeLogMessage(id, msg);
		pthread_mutex_unlock(&mEscritura); //Cerramos la escritura del log
			if(aleatorio>=80){ //Del 90% de los usuarios que pasan el control, el 10% tiene exceso de equipaje

				Exceso_Peso(usuario_id); //Vamos a la funcion Exceso_Peso para que quede mas claro
			}
			else{			
				dormir=rand()%(4-1+1)+1;
				//Esperara entre 1 y 4s para realizarlo
				sleep(dormir);

				pthread_mutex_lock(&mEscritura); //Abrimos la escritura del log
				char id[20];
				char msg[120];
				sprintf(id, "El usuario_%d ", usuario_id);
				sprintf(msg, "Ha tardado en facturar %d segundos.", dormir);
				writeLogMessage(id, msg);
				pthread_mutex_unlock(&mEscritura); //Cerramos la escritura del log
			}
		}
		else if(aleatorio>=90){ //El 10% de los usuarios no pasan la facturacion
		pthread_mutex_lock(&mEscritura); //Abrimos la escritura del log
		char id[20];
		char msg[120];
		sprintf(id, "El usuario_%d ", usuario_id);
		sprintf(msg, "Entra a facturar en el mostrador %d.", facturadores);
		writeLogMessage(id, msg);
		pthread_mutex_unlock(&mEscritura); //Cerramos la escritura del log

			Visado_Incorrecto(usuario_id); //Vamos a la funcion Visado_Incorrecto para que quede mas claro
			
	
		}
		pthread_mutex_lock(&mCola); //Abrimos la cola
		for(i=0;i<USUARIOS;i++) if(us[i].id==usuario_id) pos=i;
		us[pos].atendido=2;
		contUsuarios--; //Deja su sitio en la cola
		if(aleatorio<90){ //Los que han pasado la facturacion
			//Cuando ha realizado la facturacion va a seguridad
			us[pos].facturado=1; //Su estado de facturacion pasa a: facturado
			//Reflejamos la accion de ir a seguridad se guarda  en el log
			pthread_mutex_lock(&mEscritura); //Abrimos la escritura del log
			char a[100];
			sprintf(a, "El usuario_%d ", usuario_id);
			writeLogMessage(a,"Ha sido mandado a seguridad.");
			pthread_mutex_unlock(&mEscritura); //Cerramos la escritura del log
		}
		pthread_mutex_unlock(&mCola); //Cerramos la cola
		//Aniadimos un usuario atendido al facturador
		if(facturadores==1){
			atendidos1++;
		}
		if(facturadores==2){
			atendidos2++;
		}
		if(contador1==5){ //Si el facturador lleva 5 usuarios atendidos se toma un descanso de 10 segundos

			pthread_mutex_lock(&mEscritura); //Abrimos la escritura del log

			char id[50];

			sprintf(id, "El facturador de la cola %d ", facturadores);
			writeLogMessage(id, "Empieza su periodo de descanso");
			pthread_mutex_unlock(&mEscritura); //Cerramos la escritura del log

			sleep(10);

			pthread_mutex_lock(&mEscritura); //Abrimos la escritura del log

			char id2[50];

			sprintf(id2, "El facturador de la cola %d ", facturadores);
			writeLogMessage(id2, "Termina su periodo de descanso");
			pthread_mutex_unlock(&mEscritura); //Cerramos la escritura del log
			contador1=0;
		
		
		}

	}
}


void Exceso_Peso(int usuario_id){

	int dormir;
	dormir=rand()%(6-2+1)+2;;
	
	sleep(dormir);

	//Registro en el log
	pthread_mutex_lock(&mEscritura); //Abrimos la escritura del log

	char id[20];
	char msg[120];

	sprintf(id, "El usuario_%d ", usuario_id);//inicializar variable usuario_id al principio de acciones facturador

	sprintf(msg, "Ha tenido una facturacion valida pero tiene exceso de peso porlo que ha estado %d segundos en la cola de facturacion.", dormir);

	writeLogMessage(id, msg);

	pthread_mutex_unlock(&mEscritura); //Cerramos la escritura del log

}

void Visado_Incorrecto(int usuario_id){

	int dormir;	
	
	dormir=rand()%(10-6+1)+6;

	sleep(dormir);//el usuario espera un 	aleatorio entre 6 y 10 

	//Registro en el log
	pthread_mutex_lock(&mEscritura); //Abrimos la escritura del log

	char id[30];
	char msg[150];

	sprintf(id, "El usuario_%d ", usuario_id);

	sprintf(msg, "No ha podido facturar por tener el visado incorrecto, y ha tardado %d segundos", dormir);

	writeLogMessage(id, msg);

	pthread_mutex_unlock(&mEscritura); //Cerramos la escritura del log
				
}



void accionesAgenteSeguridad(int usuario_id){

	int esperar, esperar1, inspeccion, i, pos;
	inspeccion = rand()%(100-0+1)+0;
	
	if(inspeccion < 60){ //El 60% pasa el control de seguridad
		
		pthread_mutex_lock(&mEscritura); //Abrimos la escritura del log
		char p[20];
		char m[200];

		esperar = rand()%(3-2+1)+2;
		sleep(esperar);
		
		sprintf(p, "El usuario_%d ", usuario_id);
		
		sprintf(m, "Va a embarcar tras tardar %d segundos", esperar);

		writeLogMessage(p,m);

		pthread_mutex_unlock(&mEscritura); //Cerramos la escritura del log
		totalEmbarcados++; //Aumentamos el numero de embarcados

	}else{ //El 40% no embarca
		pthread_mutex_lock(&mEscritura); //Abrimos la escritura del log
		char p1[20];
		char m1[200];

		esperar1 = rand()%(15-10+1)+10;
		sleep(esperar1);
		
		sprintf(p1, "El usuario_%d ", usuario_id);
		
		sprintf(m1, "Ha sido inspeccionado por lo que ha tardado %d segundos y no ha podido embarcar", esperar1);

		writeLogMessage(p1,m1);

		pthread_mutex_unlock(&mEscritura); //Cerramos la escritura del log

	}
	for(i=0; i<USUARIOS; i++){
			if(us[i].id == usuario_id){
				pos = i;
			}	
		}
		us[pos].cacheado=2; //Su estado pasa a cacheado
}

void inicializaLog(){

	logFile=fopen(logFileName,"w");
	
	fclose(logFile);

	writeLogMessage("Sistema", "Log iniciado.\n\nBienvenidos al aeropuerto nacional catalan Carles Puigdemont\n\n");

}
void inicializaMutex(){

	if(pthread_mutex_init(&mId,NULL)!=0){
		exit(-1);
	}
	
	if(pthread_mutex_init(&mEscritura,NULL)!=0){
		exit(-1);
	}
	if(pthread_mutex_init(&mCola,NULL)!=0){
		exit(-1);
	}
	
	if(pthread_mutex_init(&mControl,NULL)!=0){
		exit(-1);
	}
	
}


void inicializaGlobales(){

	admite=1;
	contUsuarios=0;
	totalAtendidos=0;
	controlSeguridad=0;
	idControl=0;
	totalUsuarios=0;
	sigint=0;
	totalEmbarcados=0;
	atendidos1=0;
	atendidos2=0;

}
void writeLogMessage(char *id, char *msg){

	time_t now =time(0);
	struct tm *tlocal = localtime(&now);
	char stnow[19];
	strftime(stnow,19, "%d/%m/ %y %H:%M:%S", tlocal);
	logFile = fopen(logFileName, "a");
	fprintf(logFile,"[%s] %s: %s\n", stnow, id,msg);
	fclose(logFile);

}
void inicializaUsuarios(){
	int i;
	for(i=0;i<USUARIOS;i++){
		us[i].id=0;
		us[i].cola=0;
		us[i].facturado=0;
		us[i].atendido=0;
		us[i].tipo=0;
		us[i].cacheado=0;
	}
}
void finPrograma(int sig){

	if(signal(SIGINT, finPrograma)==SIG_ERR){ 

		exit(-1);		
	
	}
	sigint=3;
	while(totalUsuarios>totalAtendidos){ //Esperamos a que todos los usuarios facturen y embarquen antes de terminar el programa
	}
	pthread_mutex_lock(&mEscritura); //Abrimos la escritura del log
	char msg1[100];
	sprintf(msg1,"\n\nEl facturador_1 ha atendido a: %d usuarios\nEl facturador_2 ha atendido a: %d usuarios\n", atendidos1, atendidos2);
	writeLogMessage("Sistema", msg1);
	pthread_mutex_unlock(&mEscritura); //Cerramos la escritura del log	
	pthread_mutex_lock(&mEscritura); //Abrimos la escritura del log
	char msg[100];
	sprintf(msg,"Log finalizado.\n\nEl numero de pasajeros embarcados es de: %d", totalEmbarcados);
	writeLogMessage("Sistema", msg);
	pthread_mutex_unlock(&mEscritura); //Cerramos la escritura del log
	exit(0);
}
