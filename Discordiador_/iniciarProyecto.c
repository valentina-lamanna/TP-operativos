/*
 * iniciarProyecto.c
 *
 *  Created on: May 23, 2021
 *      Author: utnso
 */

#include "iniciarProyecto.h"


void leer_config(){
	IPRAM = strdup(config_get_string_value(config,"IP_MI_RAM_HQ"));
	puertoRAM = strdup(config_get_string_value(config,"PUERTO_MI_RAM_HQ"));
	IPiMongo = strdup(config_get_string_value(config,"IP_I_MONGO_STORE"));
	puertoImongo = strdup(config_get_string_value(config,"PUERTO_I_MONGO_STORE"));
	modoDePlanificacion= strdup(config_get_string_value(config,"ALGORITMO"));
	quantum= config_get_int_value(config,"QUANTUM");
	gradoMultitarea = config_get_int_value(config,"GRADO_MULTITAREA");
	duracionSabotaje = config_get_int_value(config,"DURACION_SABOTAJE");
	retardoCicloCPU = config_get_int_value(config,"RETARDO_CICLO_CPU");

	copiaDeGradoMultitarea = config_get_int_value(config,"GRADO_MULTITAREA");
}


void inicializar_estados(){
	estadoNew = list_create();
	estadoReady = list_create();
	estadoBlockedIO = list_create();
	estadoBlockedEmergencia = list_create();
	estadoExec = list_create(); // recordar config: grado de multitarea (cuantos a la vez)
	estadoExit = list_create();
}

void inicializar_semaforos(){
	/*if(planificacionActivada){
		sem_init(&sem_planificador, 0, 1); // BINARIO - TIENE QUE EMPEZAR LA PLANIFICACION? SI/NO
	}
	else {
		sem_init(&sem_planificador, 0, 0); // BINARIO - TIENE QUE EMPEZAR LA PLANIFICACION? SI/NO
	}*/

	sem_init(&sem_tripulante_ready, 0, 0); // BINARIO - HAY ALGUN TRIPULANTE EN READY? SI/NO
	sem_init(&sem_tripulante_exec, 0, 0); // BINARIO - HAY ALGUN TRIPULANTE EN READY? SI/NO
	sem_init(&sem_multitarea, 0, gradoMultitarea); // CONTADOR

	pthread_mutex_init(&mutex_cambio_est, NULL); // MUTEX - PROTEGE LA OPERACION DE SACER TRIPS DE UN ESTADO Y PONERLOS EN OTRO
	pthread_mutex_init(&mutex_bloqueados, NULL); // MUTEX - HAY UN UNICO DISPOSITIVO DE I/O, POR LO QUE UNO PUEDE USARLO A LA VEZ
	pthread_mutex_init(&mutex_trip_id, NULL); // MUTEX - PROTEGE LA VARIABLE TRIP ID
	pthread_mutex_init(&mutex_patota_id, NULL); // MUTEX - PROTEGE LA VARIABLE PATOTA ID
	pthread_mutex_init(&mutex_patota_list, NULL); // MUTEX - PROTEGE LA LISTA DE PATOTAS (y la lista de trips adentro de la patota?)
	pthread_mutex_init(&mutex_socket_patota, NULL); // MUTEX - PROTEGE EL SOCKET DE INICIAR PATOTA
	pthread_mutex_init(&mutex_sabotaje, NULL); // MUTEX - PROTEGE LA VARIABLE DE SI HAY SABOTAJE
	pthread_mutex_init(&mutex_popular_paquetes, NULL); // MUTEX - PROTEGE LO DE POPULAR PAQUETES PARA QUE NO PISEN LA VARIABLE 2 A LA VEZ
	pthread_mutex_init(&mutex_lista_semaforos, NULL); // MUTEX - PROTEGE LA LISTA DE SEMAFOROS

	//sem_init(&sem_consola, 0, 0);
	sem_init(&sem_sabotaje, 0, 0); // BINARIO - HAY SABOTAJE?
	sem_init(&sem_sabotaje_tripu_elegido, 0, 0);
}

void crear_listas_globales(){

	listaDePatotas = list_create();
	listaDeSemaforosTrips = list_create();
}
