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



//lista de 10 usuarios (id facturado atendido tipo
//usuario en control 
FILE *logFile;
char *logFileName="log.txt";


//Mutex

pthread_mutex_t mId;
pthread_mutex_t mEscritura;
pthread_mutex_t mAtendido;


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
		if(signal(SIGINT, finPrograma) == SIG_ERR){
			perror("Error en el envio de la senial SIGINT");
			exit(-1);
		}
		inicializaMutex();
		inicializaGlobales();
		inicializaUsuarios();
		inicializaLog();
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
		if(sig=SIGUSR1){
			us[contUsuarios].cola = 1; //El usuario es asignado a la cola normal
		}
		else{
			us[contUsuarios].cola = 2; //El usuario es asignado a la cola vip
		}
		pthread_mutex_lock(&mEscritura);
		char id[20];
		char msg[200];//error
		sprintf(id, "El usuario %d ", us[contUsuarios].id);
		sprintf(msg, "entra en la cola %d",us[contUsuarios].cola);
		writeLogMessage(id, msg);
		pthread_mutex_lock(&mEscritura);
		// creación del hilo del usuario
		pthread_create(&us[contUsuarios].usuario, NULL, accionesUsuario, NULL);
	}

}

void *accionesUsuario(void* posicion){

	int aleatorio, i, pos;
	pos = contUsuarios;
	int id = us[pos].id;
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
		}else if((aleatorio>=10)&&(aleatorio<=30)){
			pthread_mutex_lock(&mEscritura);
			us[pos].atendido=2;
			char id1[20];
			char msg1[120];
			sprintf(id1, "El usuario_%d ", id);
			sprintf(msg1,"se va de la cola porque se ha cansado de esperar");
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
	}
		matamosHilo(pos);

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

		accionesAgenteSeguridad(id);

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

	int facturadores = *(int *) numfact +1; //sirve para saber que facturador es
	int aleatorio;
	int contador1 =0;
	int contador2=0;
	int libre;
	int i;//error
	int pos;//error

	pthread_mutex_lock(&mEscritura);

	char in[10];
	sprintf(in, "%d", facturadores);
	writeLogMessage("Se ha montado la mesa de facturacion", in);
	pthread_mutex_unlock(&mEscritura);
	int contador, cont, usuario_id;
	while(admite==1){
		libre=0;
		//Bucle para la busqueda de un usuario que no este siendo atendido y este en la cola
		while(libre==0){//Comienzo while2
				pthread_mutex_lock(&mAtendido);
				for(i=0;i<contUsuarios;i++){
					if(libre==0){
					//Comprobamos que el usuario exista
						if(us[i].id!=0){
							//Comprobamos que el usuario esta en la cola
							if(us[i].facturado==0){
								//Comprobamos si tiene asignada la cola asogmada y se esta ejecutando el hilo correcto
								if(us[i].tipo==facturadores){
									libre=1;//Indicamos que se ha encontrado un usuario que va a facturar
									usuario_id=us[i].id;//Guardamos el id del usuarios que se dispone a facturar
									us[i].facturado=1;//Actualizamos su estado a: facturando
									cont++;//Aumentamos en 1 el contador que permitira al facturador descansar
									pos=i;//guardamos la posicion en el array del usuario
								}
							}

						}
					}
				}
				for(i=0;i<contUsuarios;i++){
					if(libre==0){
						if(us[i].id!=0){
							if(us[i].facturado==0){
								libre=1;
								usuario_id=us[i].id;
								us[i].tipo=facturadores;
								us[i].facturado=1;
								cont++;
								pos=i;
							}

						}					
					}
				}
			if(sigint==(facturadores+1)){	
			//Escribimos en el log la cantidad de usuarios que han pasado por cada mostrador
			pthread_mutex_lock(&mEscritura);
			char id6[30];
			char msg6[100];
			sprintf(id6, "El facturador %d",facturadores);
			sprintf(msg6, " ha evaluado %d usuarios.", contador);
			writeLogMessage(id6,msg6);
			pthread_mutex_unlock(&mEscritura);
			sigint++;
			}
			pthread_mutex_unlock(&mAtendido);//error
		}//final while2
		pthread_mutex_lock(&mEscritura);
		char id[20];
		char msg[120];
		sprintf(id, "El usuario %d ", usuario_id);
		sprintf(msg, "entra a facturar en el mostrador %d.", facturadores);
		writeLogMessage(id, msg);
		pthread_mutex_unlock(&mEscritura);
		sleep(4);
		aleatorio=rand()%100;
		int dormir;
		if(aleatorio<90){
			if(aleatorio>=90){

				Exceso_Peso(usuario_id);//error
			}
			else{					
				//facturacion=0;
				dormir=rand()%(6-2+1)+2;//error
				//Esperara entre 2 y 6s para realizarlo
				sleep(dormir);

				pthread_mutex_lock(&mEscritura);
				char id[20];
				char msg[120];
				sprintf(id, "El usuario %d ", usuario_id);
				sprintf(msg, "ha tardado en facturar %d segundos.", dormir);
				writeLogMessage(id, msg);
				pthread_mutex_unlock(&mEscritura);
			}
			us[pos].facturado=1;
		}
		else if(aleatorio>=90){

			Visado_Incorrecto(usuario_id);//error


		}
		if(cont==5){

			pthread_mutex_lock(&mEscritura);

			char id[50];

			sprintf(id, "el facturador de la cola %d ", facturadores);
			writeLogMessage(id, "empieza su periodo de descanso");
			pthread_mutex_unlock(&mEscritura);

			sleep(10);

			pthread_mutex_lock(&mEscritura);

			char id2[50];

			sprintf(id2, "el facturador de la cola %d ", facturadores);
			writeLogMessage(id2, "Termina su periodo de descanso");
			pthread_mutex_unlock(&mEscritura);
			cont=0;
		
		
		}

	}
}


void Exceso_Peso(int usuario_id){

	int dormir;

	//facturacion=1;
	dormir=rand()%6+2;
	
	sleep(dormir);

	//Registro en el log
	pthread_mutex_lock(&mEscritura);

	char id[20];
	char msg[120];

	sprintf(id, "El usuario %d ", usuario_id);//inicializar variable usuario_id al principio de acciones tarima

	sprintf(msg, "la facturacion ha sido valida pero tiene exceso de peso porlo que ha estado %d segundos en la cola de facturacion.", dormir);

	writeLogMessage(id, msg);

	pthread_mutex_unlock(&mEscritura);

}

void Visado_Incorrecto(int usuario_id){

	int dormir;	
	
	//facturacion=2;
	dormir=rand()%(10-6+1)+6;

	sleep(dormir);//el usuario espera un 	aleatorio entre 6 y 10 

	//Registro en el log
	pthread_mutex_lock(&mEscritura);

	char id[30];
	char msg[150];

	sprintf(id, "Usuario_%d ", usuario_id);

	sprintf(msg, "No ha podido facturar por tener el visado incorrecto, y ha tardado %d segundos", dormir);

	writeLogMessage(id, msg);

	pthread_mutex_unlock(&mEscritura);
				
}



void accionesAgenteSeguridad(int usuario_id){

	int esperar, esperar1, inspeccion;
	inspeccion = rand()%100;	//No estoy seguro de si %100 o %10
	
	if(inspeccion < 60){
		pthread_mutex_lock(&mEscritura);
		char p[20];
		char m[200];

		esperar = rand()%(3-2+1)+2;
		sleep(esperar);
		
		sprintf(p, "Usuario_%d ", usuario_id);
		
		sprintf(m, "va a embarcar tras tardar %d segundos", esperar);

		writeLogMessage(p,m);

		pthread_mutex_unlock(&mEscritura);

	}else{
		pthread_mutex_lock(&mEscritura);
		char p1[20];
		char m1[200];

		esperar1 = rand()%(15-10+1)+10;
		sleep(esperar1);
		
		sprintf(p1, "Usuario_%d ", usuario_id);
		
		sprintf(m1, "ha sido inspeccionado por lo que ha tardado %d segundos", esperar1);

		writeLogMessage(p1,m1);

		pthread_mutex_unlock(&mEscritura);

	}
}

void inicializaLog(){

	logFile=fopen(logFileName,"w");
	
	fclose(logFile);

	writeLogMessage("Sistema", "Log iniciado.\nBienvenidos al aeropuerto nacional catalan Carles Puigdemont");

}
void inicializaMutex(){

	if(pthread_mutex_init(&mId,NULL)!=0){
		exit(-1);
	}
	
	if(pthread_mutex_init(&mEscritura,NULL)!=0){
		exit(-1);
	}
	if(pthread_mutex_init(&mAtendido,NULL)!=0){
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
	}

	//aqui falta algo

}

//hay que editar esta funcion
void finPrograma(int sig){

	if(signal(SIGINT, finPrograma)==SIG_ERR){

		exit(-1);		
	
	}	

}
