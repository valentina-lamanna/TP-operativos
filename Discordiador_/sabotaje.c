/*
 * sabotaje.c
 *
 *  Created on: Jun 17, 2021
 *      Author: utnso
 */

#include "sabotaje.h"

void esperarARecibirSabotajes(){
	pthread_t sabotaje;

	execOrdenado = list_create();
	readyOrdenado = list_create();
	sabotajeResuelto = false;

	pthread_create(&sabotaje, NULL, (void*)recibirSenialDeSabotaje, NULL);

	//pthread_join(sabotaje, NULL);
	pthread_detach(sabotaje);
}

void recibirSenialDeSabotaje()
{
	while(true){ // no hay espera activa porque el receive del socket es bloqueante

		sleep(1);

		socketSabotaje = crear_conexion(IPiMongo, puertoImongo);

		mandarMensajeSabotaje("Espero sabotajes");

		char* direccSabotaje = recibirUnPaqueteConUnMensaje(socketSabotaje);
		//sleep(20); // me quedo con estas dos lineas para pruebas sin mongo
		//char* direccSabotaje = "1|1";

		log_info(logger, "!!!!!!!! [SABOTAJE] !!!!!!!!");

		char** arrayConLas2Direcc = string_n_split(direccSabotaje, 2, "|"); //["1", "1"]

		posXsabotaje = (uint32_t)atoi(arrayConLas2Direcc[0]);
		posYsabotaje = (uint32_t)atoi(arrayConLas2Direcc[1]);


		moverAColaDeBlockEmergencia(); // se resuelve y finaliza en manejarSabotaje()

		Tripulante* tripMasCercanoASabotaje = calcularTripMasCercanoA(posXsabotaje, posYsabotaje);
		tripMasCercanoASabotaje->yoResuelvoSabotaje = true;


		/*for(int i = 0; i < list_size(estadoBlockedEmergencia); i++){
			sem_post(&sem_sabotaje_tripu_elegido);
		}*/

		/*void bloquearlos(Tripulante* tripu){
			sem_wait(&tripu->sabotaje);
		}*/

		//list_iterate(readyOrdenado, (void*)bloquearlos); // los readys nunca entran en manejarSabotaje?


		pthread_mutex_lock(&mutex_sabotaje);
		haySabotaje = true;

		void desbloquearMultitarea(Tripulante* tripu){
			sem_post(&sem_multitarea); // los desbloqueo para que avancen!!
		}

		list_iterate(readyOrdenado, (void*)desbloquearMultitarea);
		pthread_mutex_unlock(&mutex_sabotaje);

		//list_iterate(estadoBlockedEmergencia, (void*) desbloquearlos);


		sem_wait(&sem_sabotaje); // espero a que termine de resolverse

		//list_iterate(readyOrdenado, (void*)bloquearlos); // los readys nunca entran en manejarSabotaje?

		finalizarSabotaje();

		void desbloquearlos(Tripulante* tripu){
			sem_post(&tripu->sabotaje);
		}

		list_iterate(execOrdenado, (void*)desbloquearlos);
		list_iterate(readyOrdenado, (void*)desbloquearlos);

		char* confirmacionDeMongo = recibirUnPaqueteConUnMensaje(socketSabotaje);

		log_info(logger, "Mongo confirmo que termino el sabotaje: %s", confirmacionDeMongo);

		close(socketSabotaje);

		/*void bloquearMultitareaParaQueQuedeComoAntes(Tripulante* trip){
			sem_wait(&sem_multitarea);
		}

		list_iterate(readyOrdenado, (void*)bloquearMultitareaParaQueQuedeComoAntes);*/
	}
	// nunca se hace un close del socket sabotaje?
}

void manejarSabotaje(Tripulante* trip){ // esta funcion es llamada por todos los hilos tripulantes

	log_info(logger, "trip %d alistado para ver si le toca resolver le sabotaje", trip->TID);

	//sem_wait(&sem_sabotaje_tripu_elegido); // los cambiamos pd FS

	//sem_wait(&trip->sabotaje);
	// se desbloquea desde la funcion de arriba

	if(trip->yoResuelvoSabotaje){ // porque va a haber muchos hilos aca, no quiero mandar 20 veces el mismo mens a la bitacora y logguear
		//log_info(logger, "El tripulante %d es el mas cercano a la posicion del sabotaje (%d|%d)", trip->TID, posXsabotaje, posYsabotaje);

		sleep(1);

		mandarMensajeBitacora(trip, "Fui elegido para resolver el sabotaje \n");

		log_info(logger, "Muevo al trip %d a exec para resolver el sabotaje", trip->TID);
		cambiarDeEstado(estadoBlockedEmergencia, estadoExec, trip, 'E');

		moverEInformar(trip, posXsabotaje, posYsabotaje);

		mandarMensajeSabotaje("FSCK"); // invoco a esto en mongo TODO: descomentar
		// me tiene que devolver algo tipo un OK ya lo resolvi?

		sleep(duracionSabotaje);

		log_info(logger, "Muevo al trip %d al final de la cola de blocked de emergencia porque ya resolvio sabotaje", trip->TID);
		cambiarDeEstado(estadoExec, estadoBlockedEmergencia, trip, 'B');

		mandarMensajeBitacora(trip, "Termine de resolver el sabotaje \n");

		trip->yoResuelvoSabotaje = false;

		pthread_mutex_lock(&mutex_sabotaje);
		haySabotaje = false;
		pthread_mutex_unlock(&mutex_sabotaje);

		sem_post(&sem_sabotaje); // listo, ya esta resuelto
	}
	else { // si no voy a resolver el sabotaje, simplemente espero
		sleep(duracionSabotaje);
	}

	//me vuelvo a bloquear, asi me desbloquean cuando terminan
	log_info(logger, "trip %d ya espero el tiempo mientras se resolvia el sabotaje", trip->TID);

	sem_wait(&trip->sabotaje);
}

void moverAColaDeBlockEmergencia(){

	void pasarDeExecABlockEmergencia(Tripulante* tripu){
		log_info(logger, "TRIP %d BLOQUEADO POR EMERGENCIA", tripu->TID);
		cambiarDeEstado(estadoExec, estadoBlockedEmergencia, tripu, 'B');
		//sem_post(&sem_multitarea);
	}

	void pasarDeReadyABlockEmergencia(Tripulante* tripu){
		log_info(logger, "TRIP %d BLOQUEADO POR EMERGENCIA", tripu->TID);
		cambiarDeEstado(estadoReady, estadoBlockedEmergencia, tripu, 'B');
		//sem_post(&sem_multitarea); // los desbloqueo para que avancen!!
	}

	bool idEsMenor(Tripulante* unTrip, Tripulante* otroTrip){
		return unTrip->TID < otroTrip->TID;
	}

	execOrdenado = list_sorted(estadoExec, (void*)idEsMenor);
	readyOrdenado = list_sorted(estadoReady, (void*)idEsMenor);

	list_iterate(execOrdenado, (void*)pasarDeExecABlockEmergencia);
	list_iterate(readyOrdenado, (void*)pasarDeReadyABlockEmergencia);
}

void finalizarSabotaje(){
	void pasarDeBlockEmergenciaAExec(Tripulante* tripu){
		log_info(logger, "TRIP %d DESBLOQUEADO - FIN EMERGENCIA", tripu->TID);
		//sem_wait(&sem_multitarea);
		cambiarDeEstado(estadoBlockedEmergencia, estadoExec, tripu, 'E');
	}

	void pasarDeBlockEmergenciaAReady(Tripulante* tripu){
		log_info(logger, "TRIP %d DESBLOQUEADO - FIN EMERGENCIA", tripu->TID);
		cambiarDeEstado(estadoBlockedEmergencia, estadoReady, tripu, 'R');
	}

	list_iterate(execOrdenado, (void*)pasarDeBlockEmergenciaAExec);
	list_iterate(readyOrdenado, (void*)pasarDeBlockEmergenciaAReady);

	void liberarALosBloqueados(Tripulante* trip){ // los blockeados por IO los libero a que sigan con lo suyo
		sem_post(&trip->sabotaje);
	}

	list_iterate(estadoBlockedIO, (void*)liberarALosBloqueados);

	sabotajeResuelto = false;
	/*while(!list_is_empty(estadoBlockedEmergencia)){
		// quiero liberar a todos a la vez
	}*/
}

Tripulante* calcularTripMasCercanoA(uint32_t posX, uint32_t posY){

	Tripulante* tripulanteMasCercano = NULL;

	uint32_t menorDistancia = 999999999;

	void iteradorDeTripulantes(Tripulante* tripulante)
	{
		uint32_t cantMovTrip = calcularCantidadDeMovimientos(tripulante->posicionX, tripulante->posicionY, posX, posY);
		if(cantMovTrip < menorDistancia){
			menorDistancia = cantMovTrip;
			tripulanteMasCercano = tripulante;
		}
	}

	list_iterate(estadoBlockedEmergencia, (void*)iteradorDeTripulantes);

	if(tripulanteMasCercano != NULL){
		log_info(logger, "El tripulante %d es el mas cercano a la posicion del sabotaje (%d|%d) porque llega con solo %d movimientos", tripulanteMasCercano->TID, posX, posY, menorDistancia);
	}

	return tripulanteMasCercano;
}

uint32_t calcularCantidadDeMovimientos(uint32_t posXInicial, uint32_t posYInicial, uint32_t posXFinal, uint32_t posYFinal){
	uint32_t cantMovimientos = 0;

	// posicion x
	cantMovimientos += abs(posXFinal - posXInicial);

	// posicion y
	cantMovimientos += abs(posYFinal - posYInicial);

	return cantMovimientos;
}

void mandarMensajeSabotaje(char* mensaje){
	uint32_t size = sizeof(uint32_t) + strlen(mensaje) + 1;

	paquete(socketSabotaje, mensaje, size, SABOTAJE, true);
}

void moverEInformar(Tripulante* trip, uint32_t posXFinal, uint32_t posYFinal){
	char* movimientoParaBitacora = string_new();
	char* posiInicial = string_new();
	char* posiFinal = string_new();
	char* rta;
	uint32_t cantMovimientos = 0;

	Tripulante* tripulanteMapa = trip;
	uint32_t tam = 4 * sizeof(uint32_t); // id, x, y, PID

	string_append(&posiInicial, string_itoa(trip->posicionX));
	string_append(&posiInicial, "|");
	string_append(&posiInicial, string_itoa(trip->posicionY));

	string_append(&posiFinal, string_itoa(posXFinal));
	string_append(&posiFinal, "|");
	string_append(&posiFinal, string_itoa(posYFinal));

	string_append(&movimientoParaBitacora, "MOVIMIENTO HACIA EL LUGAR DEL SABOTAJE \n");

	// posicion x

	if(posXFinal > trip->posicionX){
		for(int x = trip->posicionX; x < posXFinal; x++){
			string_append(&movimientoParaBitacora, "Me muevo de ");
			string_append(&movimientoParaBitacora, string_itoa(trip->posicionX));
			string_append(&movimientoParaBitacora, "|");
			string_append(&movimientoParaBitacora, string_itoa(trip->posicionY));
			string_append(&movimientoParaBitacora, " a ");
			string_append(&movimientoParaBitacora, string_itoa(x + 1));
			string_append(&movimientoParaBitacora, "|");
			string_append(&movimientoParaBitacora, string_itoa(trip->posicionY));
			string_append(&movimientoParaBitacora, "\n");
			trip->posicionX = x + 1;
			cantMovimientos ++;

			tripulanteMapa->posicionX = trip->posicionX;
			tripulanteMapa->posicionY = trip->posicionY;

			pthread_mutex_lock(&trip->mutex_socket);
			conectarseA(trip, "RAM");
			paquete(trip->socket_RAM, tripulanteMapa, tam, RECIBIR_UBICACION_TRIPULANTE, true);
			rta = recibirUnPaqueteConUnMensaje(trip->socket_RAM);
			pthread_mutex_unlock(&trip->mutex_socket);
		}
	}
	else {
		for(int x = trip->posicionX; x > posXFinal; x--){
			string_append(&movimientoParaBitacora, "Me muevo de ");
			string_append(&movimientoParaBitacora, string_itoa(trip->posicionX));
			string_append(&movimientoParaBitacora, "|");
			string_append(&movimientoParaBitacora, string_itoa(trip->posicionY));
			string_append(&movimientoParaBitacora, " a ");
			string_append(&movimientoParaBitacora, string_itoa(x - 1));
			string_append(&movimientoParaBitacora, "|");
			string_append(&movimientoParaBitacora, string_itoa(trip->posicionY));
			string_append(&movimientoParaBitacora, "\n");
			trip->posicionX = x - 1;
			cantMovimientos ++;

			tripulanteMapa->posicionX = trip->posicionX;
			tripulanteMapa->posicionY = trip->posicionY;

			pthread_mutex_lock(&trip->mutex_socket);
			conectarseA(trip, "RAM");
			paquete(trip->socket_RAM, tripulanteMapa, tam, RECIBIR_UBICACION_TRIPULANTE, true);
			rta = recibirUnPaqueteConUnMensaje(trip->socket_RAM);
			pthread_mutex_unlock(&trip->mutex_socket);
		}
	}

	// posicion Y

	if(posYFinal > trip->posicionY){
		for(int y = trip->posicionY; y < posYFinal; y++){
			string_append(&movimientoParaBitacora, "Me muevo de ");
			string_append(&movimientoParaBitacora, string_itoa(trip->posicionX));
			string_append(&movimientoParaBitacora, "|");
			string_append(&movimientoParaBitacora, string_itoa(trip->posicionY));
			string_append(&movimientoParaBitacora, " a ");
			string_append(&movimientoParaBitacora, string_itoa(trip->posicionX));
			string_append(&movimientoParaBitacora, "|");
			string_append(&movimientoParaBitacora, string_itoa(y + 1));
			string_append(&movimientoParaBitacora, "\n");
			trip->posicionY = y + 1;
			cantMovimientos ++;

			tripulanteMapa->posicionX = trip->posicionX;
			tripulanteMapa->posicionY = trip->posicionY;

			pthread_mutex_lock(&trip->mutex_socket);
			conectarseA(trip, "RAM");
			paquete(trip->socket_RAM, tripulanteMapa, tam, RECIBIR_UBICACION_TRIPULANTE, true);
			rta = recibirUnPaqueteConUnMensaje(trip->socket_RAM);
			pthread_mutex_unlock(&trip->mutex_socket);
		}
	}
	else {
		for(int y = trip->posicionY; y > posYFinal; y--){
			string_append(&movimientoParaBitacora, "Me muevo de ");
			string_append(&movimientoParaBitacora, string_itoa(trip->posicionX));
			string_append(&movimientoParaBitacora, "|");
			string_append(&movimientoParaBitacora, string_itoa(trip->posicionY));
			string_append(&movimientoParaBitacora, " a ");
			string_append(&movimientoParaBitacora, string_itoa(trip->posicionX));
			string_append(&movimientoParaBitacora, "|");
			string_append(&movimientoParaBitacora, string_itoa(y - 1));
			string_append(&movimientoParaBitacora, "\n");
			trip->posicionY = y - 1;
			cantMovimientos ++;

			tripulanteMapa->posicionX = trip->posicionX;
			tripulanteMapa->posicionY = trip->posicionY;

			pthread_mutex_lock(&trip->mutex_socket);
			conectarseA(trip, "RAM");
			paquete(trip->socket_RAM, tripulanteMapa, tam, RECIBIR_UBICACION_TRIPULANTE, true);
			rta = recibirUnPaqueteConUnMensaje(trip->socket_RAM);
			pthread_mutex_unlock(&trip->mutex_socket);
		}
	}

	mandarMensajeBitacora(trip, movimientoParaBitacora);

	log_info(logger, "Se movio el tripulante %d de %s a %s para resolver el sabotaje. Se aviso a RAM y Mongo", trip->TID, posiInicial, posiFinal);
}
