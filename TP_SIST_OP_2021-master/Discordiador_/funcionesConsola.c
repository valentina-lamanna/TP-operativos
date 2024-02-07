/*
 * funsionesConsola.c
 *
 *  Created on: May 24, 2021
 *      Author: utnso
 */

#include "funcionesConsola.h"
//CREAR PATOTA Y TRUPULANTES------------------------------------------------------------------------------------
Patota* crearPatota(uint32_t PID,int cantTrip, char* tareas)
{
	Patota* patota= malloc(sizeof(Patota));

	patota->PID = PID;
	patota->cantTripulantes = cantTrip;
	patota->todasLasTareas = tareas;
	patota->tripulantes = list_create();

	return patota;
}

void crearHiloTripulante(Patota* patota, uint32_t idTrip, char* direccionesDeTripulantes, int socketRAM)
{
	char** arrayConLas2Direcc = string_n_split(direccionesDeTripulantes, 2, "|"); //["1", "1"]

	uint32_t posicionX = (uint32_t)atoi(arrayConLas2Direcc[0]);
	uint32_t posicionY = (uint32_t)atoi(arrayConLas2Direcc[1]);

	Tripulante* tripulante = malloc(sizeof(Tripulante));
	tripulante->punteroPID = malloc(sizeof(uint32_t));

	tripulante->TID = idTrip;
	tripulante->estado = 'N';
	tripulante->posicionX = posicionX;
	tripulante->posicionY = posicionY;
	tripulante->punteroPID = &patota->PID;
	tripulante->quantumRestante = quantum;
	tripulante->tareaActual = NULL;
	tripulante->socket_RAM = -1;
	tripulante->socket_Mongo = -1;
	tripulante->movimientoParaBitacora = string_new();
	tripulante->estaVivo = true;
	tripulante->terminoTareaActual = false;
	tripulante->yoResuelvoSabotaje = false;
	sem_init(&tripulante->yaFueCreadoEnRAM, 0, 0);
	sem_init(&tripulante->ejecutando, 0, 0);
	sem_init(&tripulante->sabotaje, 0, 0);
	pthread_mutex_init(&tripulante->mutex_socket, NULL);

	if(planificacionActivada){
		sem_init(&tripulante->sem_planificador, 0, 1); // BINARIO - TIENE QUE EMPEZAR LA PLANIFICACION? SI/NO
	}
	else {
		sem_init(&tripulante->sem_planificador, 0, 0); // BINARIO - TIENE QUE EMPEZAR LA PLANIFICACION? SI/NO
	}

	//pthread_mutex_lock(&mutex_lista_semaforos);
	//list_add(listaDeSemaforosTrips, &tripulante->sem_planificador);
	//pthread_mutex_unlock(&mutex_lista_semaforos);

	pthread_mutex_lock(&mutex_patota_list);
	list_add(patota->tripulantes, tripulante);
	pthread_mutex_unlock(&mutex_patota_list);

	accionesCrearTripulante(tripulante, socketRAM);


	pthread_create(&(tripulante->hiloTrip), NULL, (void*)ejecutarTarea, tripulante);

	//pthread_join(tripulante->hiloTrip, NULL); // SI NO HAGO JOIN NO ME CREA EL TRIP, EL PROGRAMA TERMINA SOLO :(
	//pthread_detach(tripulante->hiloTrip); // como no me importa que "retorna" un tripulante,
								//se hace un detach, y si me importara es un join

	// NO SE PUEDE DETACHAR UN HILO SI QUIERO HACER CANCEL, ASI QUE CUANDO TERMINE LE HAGO CANCEL + JOIN
}

void accionesCrearTripulante(Tripulante* tripulante, int socketRam)
{
	log_info(logger, "Acciones del inicio del tripulante %d", tripulante->TID);

	uint32_t tamPaquete = 4 * sizeof(uint32_t); //TID, x, y, punteroPCB

	pthread_mutex_lock(&mutex_socket_patota);
	paquete(socketRam, tripulante, tamPaquete, INICIAR_PATOTA, true);
	pthread_mutex_unlock(&mutex_socket_patota);

	// SE AGREGA ALGUIEN A NEW
	agregarAEstado(estadoNew, tripulante);
	log_info(logger, "Tripulante %d agregado a New", tripulante->TID);

	bitacora_mensaje* mensajeBitacora = malloc(sizeof(bitacora_mensaje));
	mensajeBitacora->idTrip = tripulante->TID;
	mensajeBitacora->accion = "Hola, me acaban de crear \n";

	tamPaquete = 2 * sizeof(uint32_t) + strlen(mensajeBitacora->accion) + 1;

	conectarseA(tripulante, "MONGO");
	paquete(tripulante->socket_Mongo, mensajeBitacora, tamPaquete, NUEVO_TRIP, true);
	char* buffer = recibirUnPaqueteConUnMensaje(tripulante->socket_Mongo);

	free(buffer);
	free(mensajeBitacora);
}

Tripulante* buscarTripulantePorId(uint32_t id){

	Tripulante* tripulante = NULL;

	bool idEsIgual(Tripulante* trip){
		return trip->TID == id;
	}

	void iteradorDePatotas(Patota* patota)
	{
		Tripulante* trip = list_find(patota->tripulantes, (void*)idEsIgual);
		if(trip != NULL){
			tripulante = trip;
			return;
		}
	}

	list_iterate(listaDePatotas, (void*)iteradorDePatotas);

	return tripulante;
}



// NO CREO QUE USEMOS ESTO
char** asignarDireccDeMemoria(char** array, int cantidadDeTripulantes)
{
	//["1|3", "2|5", "0|0", "0|0", "0|0"]
	//5 tripulantes

	int posiDelArray = 3;
	char** direcciones;

	// con eso se resuelve lo de los nulls, queda t o d o inicializado en "0|0" :)
	for(int i = 0; i < cantidadDeTripulantes; i++)
	{
		direcciones[i] = "0|0";
	}

	for(int i = 0; i < cantidadDeTripulantes; i++)
	{
		if(array[posiDelArray] != NULL)
		{
			direcciones[i] = array[posiDelArray];
		}

		posiDelArray ++;
	}

	return direcciones;
}
