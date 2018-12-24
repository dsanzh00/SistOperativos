# include <stdio.h>
# include <stdlib.h>
# include <sys/wait.h>
# include <signal.h>
# include <unistd.h>
# include <sys/types.h>
# include <errno.h>
# include <time.h>
# include <signal.h>

void nuevoUsuario(int sig);
void accionesUsuario(int sig);
void accionesFacturador(int sig);
void accionesAgenteSeguridad(int sig);



int main(int argc, char *argv[]){

	printf("hoola");
	printf("hey");
	printf("dj");


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
