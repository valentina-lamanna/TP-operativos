/*
 * planificacion.c
 *
 *  Created on: May 24, 2021
 *      Author: utnso
 */
#include "planificacion.h"

void crear_planificacion(){
	pthread_create(&planificador, NULL, (void*)ejecutarAlgoritmo, modoDePlanificacion);
	pthread_detach(planificador); // si le pones join, ni siquiera empieza el programa
}

void ejecutarAlgoritmo(char* modoDePlanificacion){
	while(1){

		void pasarAExec(Tripulante* tripulante)
		{
			if(haySabotaje){ // CHECKEO SABOTAJE
				manejarSabotaje(tripulante);
			}

			sem_post(&(tripulante->ejecutando));

			//sem_post(&sem_planificador);
			sem_wait(&tripulante->sem_planificador);
			sem_post(&tripulante->sem_planificador);
			//log_info(logger, "acabo de hacer post del ejecutando del trip %d", tripulante->TID);
		}

		sem_wait(&sem_tripulante_ready);
		list_iterate(estadoReady, (void*)pasarAExec);
	}
}

void ejecutarTarea(Tripulante* tripulante)
{
	if(planificacionActivada){
		sem_wait(&tripulante->yaFueCreadoEnRAM);
		cambiarDeEstado(estadoNew, estadoReady, tripulante, 'R');
		sem_post(&sem_tripulante_ready); // aviso que hay alguien en ready
		//sem_post(&sem_planificador); // chequear
		//sem_post(&tripulante->sem_planificador); //YA EMPIEZA EN 1 SI ESTA ACTIVADA
	}

	while(tripulante->estaVivo){

		//sem_wait(&sem_planificador);

		sem_wait(&(tripulante->ejecutando));

		// ACA ES LA TRANSICION READY -> EXEC //
		if(tripulante->estado == 'R' && tripulante->estaVivo){
			pasarTripAExec(tripulante);
		}
		// ACA ES LA TRANSICION READY -> EXEC //

		if(tripulante->tareaActual == NULL){
			log_info(logger, "pido una tarea para el trip %d", tripulante->TID);
			pedirTarea(tripulante);
			/*Tarea* tarea = malloc(sizeof(Tarea));
			tarea->nombreTarea = "PROBANDO";
			tarea->posX = 1;
			tarea->posY = 3;
			tarea->esDeIO = false;
			tarea->tiempo = 5;
			tripulante->tareaActual = tarea;*/
		}
		//sem_post(&sem_planificador);

		if(haySabotaje){ // CHECKEO SABOTAJE
			manejarSabotaje(tripulante);
		}
		if(!tripulante->estaVivo){
			expulsarTripulante(tripulante, false);
		}

		if(strcmp(tripulante->tareaActual->nombreTarea, "FIN") == 0){
			expulsarTripulante(tripulante, true); // porque termino sus tareas!
		}
		else {
			informarYHacerMovimientoDePosiciones(tripulante);

			if(tripulante->estado == 'E'){ // o sea, si no fue desalojado
				if(tripulante->tareaActual->esDeIO){
					//sem_post(&sem_planificador); // para equilibrar el wait de arriba
					irABlockedIO(tripulante);
				}
				else{
					//sem_post(&sem_planificador); // para equilibrar el wait de arriba
					ejecutarTareaEnExec(tripulante);
				}

				if(tripulante->estado == 'E'){ // o sea, si no fue desalojado
					sem_post(&(tripulante->ejecutando)); // para no trabar la segunda vez que pide una tarea dentro de exec
				}
			}
		}
	}
}

void pasarTripAExec(Tripulante* tripulante){
	//sem_wait(&sem_planificador);
	sem_wait(&tripulante->sem_planificador);
	sem_post(&tripulante->sem_planificador);

	sem_wait(&sem_multitarea);
	//copiaDeGradoMultitarea --;
	//log_info(logger, "GRADO DE MULTITAREA: %d \n", copiaDeGradoMultitarea);

	if(haySabotaje){ // CHECKEO SABOTAJE
		manejarSabotaje(tripulante);
		log_info(logger, "Al estar volviendo de un sabotaje, el tripulante %d sigue esperando en la cola de Ready", tripulante->TID);
		sem_wait(&sem_multitarea);
		if(haySabotaje){ // CHECKEO SABOTAJE vol2
			manejarSabotaje(tripulante);
			log_info(logger, "Al estar volviendo de un sabotaje, el tripulante %d sigue esperando en la cola de Ready", tripulante->TID);
			sem_wait(&sem_multitarea);
			if(haySabotaje){ // CHECKEO SABOTAJE vol 3
				manejarSabotaje(tripulante);
				log_info(logger, "Al estar volviendo de un sabotaje, el tripulante %d sigue esperando en la cola de Ready", tripulante->TID);
				sem_wait(&sem_multitarea);
				if(haySabotaje){ // CHECKEO SABOTAJE vol 4
					manejarSabotaje(tripulante);
					log_info(logger, "Al estar volviendo de un sabotaje, el tripulante %d sigue esperando en la cola de Ready", tripulante->TID);
					sem_wait(&sem_multitarea);
					if(haySabotaje){ // CHECKEO SABOTAJE vol 5
						manejarSabotaje(tripulante);
						log_info(logger, "Al estar volviendo de un sabotaje, el tripulante %d sigue esperando en la cola de Ready", tripulante->TID);
						sem_wait(&sem_multitarea);
					}
				}
			}
		}
	}

	cambiarDeEstado(estadoReady, estadoExec, tripulante, 'E');
}

void cambiarDeEstado(t_list* estadoViejo, t_list* estadoNuevo, Tripulante* tripulante, char estado) {

	if(!tripulante->estaVivo){
		return;
	}

	if(estaEnEstado(tripulante, estadoViejo)){
		eliminarDeEstado(estadoViejo, tripulante);
		agregarAEstado(estadoNuevo, tripulante);

		actualizarCharEstado(tripulante, estado);

		log_info(logger, "Se cambio al tripulante %d de estado. Ahora su estado es %c ", tripulante->TID, estado);

		Tripulante* nuevoEstadoTrip = tripulante;

		pthread_mutex_lock(&tripulante->mutex_socket);

		conectarseA(tripulante, "RAM");
		paquete(tripulante->socket_RAM, nuevoEstadoTrip, 2 * sizeof(uint32_t) + sizeof(char), ACTUALIZAR_ESTADO, true);
		char* buffer = recibirUnPaqueteConUnMensaje(tripulante->socket_RAM);

		pthread_mutex_unlock(&tripulante->mutex_socket);

		/*int socket_cambio = crear_conexion(IPRAM, puertoRAM);
		paquete(socket_cambio, nuevoEstadoTrip, 2 * sizeof(uint32_t) + sizeof(char), ACTUALIZAR_ESTADO, true);
		char* buffer = recibirUnPaqueteConUnMensaje(socket_cambio);
		close(socket_cambio);*/
	}

	if(!tripulante->estaVivo){
		return;
	}
	/*if(haySabotaje){ // CHECKEO SABOTAJE no lo puedo hacer aca porque esto se llama desde sabotajes tambien!
		manejarSabotaje(tripulante);
	}*/
}

void actualizarCharEstado(Tripulante* trip, char nuevoEstado){
	pthread_mutex_lock(&mutex_cambio_est);
	trip->estado = nuevoEstado;
	pthread_mutex_unlock(&mutex_cambio_est);
}

void agregarAEstado(t_list* estado, Tripulante* tripulante) {
	pthread_mutex_lock(&mutex_cambio_est);
	list_add(estado, tripulante);
	pthread_mutex_unlock(&mutex_cambio_est);
}

void eliminarDeEstado(t_list* estado, Tripulante* tripulante) {

	bool esElTripulante(void* unTrip) {
		Tripulante* otroTrip = unTrip;
		return otroTrip->TID == tripulante->TID;
	}

	pthread_mutex_lock(&mutex_cambio_est);
	list_remove_by_condition(estado, esElTripulante);
	pthread_mutex_unlock(&mutex_cambio_est);
}

void irABlockedIO(Tripulante* trip){
	Tarea* tareaActual = trip->tareaActual;

	log_info(logger, "El tripulante %d tiene la tarea %s de IO", trip->TID, tareaActual->nombreTarea);

	if(!trip->estaVivo){
			return;
	}

	char* mens = string_new();
	string_append(&mens, "Estoy iniciando la tarea ");
	string_append(&mens, tareaActual->nombreTarea);
	string_append(&mens, "\n");

	mandarMensajeBitacora(trip, mens);


	cambiarDeEstado(estadoExec, estadoBlockedIO, trip,'B');
	sem_post(&sem_multitarea); // si sacamos de exec, entonces se libera un espacio
	//copiaDeGradoMultitarea++;
	//log_info(logger, "GRADO DE MULTITAREA: %d \n", copiaDeGradoMultitarea);

	//sleep(retardoCicloCPU * tarea.tiempo);
	if(!trip->estaVivo){
			return;
	}

	sleep(retardoCicloCPU); // consumir 1 ciclo para peticion de I/O

	pthread_mutex_lock(&mutex_bloqueados); // hay un solo dispositivo de IO!

	/* MANDAR A MONGO EL IO */

	uint32_t sizeRecursoMensaje = sizeof(int) + 2*sizeof(char) + sizeof(uint32_t) + strlen(tareaActual->recurso) + 1;

	mandarMensajeIO(trip, sizeRecursoMensaje, tareaActual->parametros, tareaActual->recurso, tareaActual->caracterDelRecurso, tareaActual->agregar);

	for(int i = 0; i < tareaActual->tiempo; i++){ // EJECUTAR DE A UNO!
		if(!trip->estaVivo){
			pthread_mutex_unlock(&mutex_bloqueados);
			return;
		}

		//sem_wait(&sem_planificador);

		//sem_post(&sem_planificador);
		sem_wait(&trip->sem_planificador);
		sem_post(&trip->sem_planificador);

		sleep(retardoCicloCPU);

		if(haySabotaje){ // CHECKEO SABOTAJE
			sem_wait(&trip->sabotaje); // reveer!
		}

		if(!trip->estaVivo){
			pthread_mutex_unlock(&mutex_bloqueados);
			return;
		}
	}

	pthread_mutex_unlock(&mutex_bloqueados); // hay un solo dispositivo de IO!

	//sem_wait(&sem_planificador);
	sem_wait(&trip->sem_planificador);
	sem_post(&trip->sem_planificador);
	log_info(logger, "El tripulante %d termino de estar bloqueado por IO y vuelve a ready", trip->TID);
	cambiarDeEstado(estadoBlockedIO, estadoReady, trip,'R');
	sem_post(&sem_tripulante_ready); // aviso que hay alguien en ready

	trip->quantumRestante = quantum; // reinicio el quantum!
	trip->terminoTareaActual = true;

	mens = string_new();
	string_append(&mens, "Termine la tarea ");
	string_append(&mens, tareaActual->nombreTarea);
	string_append(&mens, "\n");

	mandarMensajeBitacora(trip, mens);

	free(tareaActual);
	trip->tareaActual = NULL;

	//sem_post(&sem_planificador);
	sem_wait(&trip->sem_planificador);
	sem_post(&trip->sem_planificador);

	if(!trip->estaVivo){
			return;
	}
	if(haySabotaje){ // CHECKEO SABOTAJE
		manejarSabotaje(trip);
	}
}

void ejecutarTareaEnExec(Tripulante* trip){

	Tarea* tareaActual = trip->tareaActual;

	log_info(logger, "El tripulante %d esta ejecutando la tarea %s en la cola de EXEC", trip->TID, tareaActual->nombreTarea);

	if(!trip->estaVivo){
			return;
	}
	if(haySabotaje){ // CHECKEO SABOTAJE
		manejarSabotaje(trip);
	}

	int tiempoRestante = trip->tiempoRestanteTarea;
	char* mens = string_new();

	if(tiempoRestante == tareaActual->tiempo){

		string_append(&mens, "Estoy iniciando la tarea ");
		string_append(&mens, tareaActual->nombreTarea);
		string_append(&mens, "\n");

		mandarMensajeBitacora(trip, mens);
	}

	for(int i = 0; i < tiempoRestante; i++){ // EJECUTAR DE A UNO!
		//sem_wait(&sem_planificador);

		//sem_post(&sem_planificador);
		if(!trip->estaVivo){
			return;
		}

		sem_wait(&trip->sem_planificador);
		sem_post(&trip->sem_planificador);

		sleep(retardoCicloCPU);

		if(!trip->estaVivo){
				return;
		}

		if(strcmp(modoDePlanificacion ,"RR") == 0){// && trip->quantumRestante <= 0){
			trip->tiempoRestanteTarea -= 1;
			trip->quantumRestante --;
			log_info(logger, "quantum restante del trip %d: %d, le faltan %d ciclos de la tarea", trip->TID, trip->quantumRestante, trip->tiempoRestanteTarea);
			trip->terminoTareaActual = (trip->tiempoRestanteTarea == 0);
			if(trip->quantumRestante <= 0){
				desalojar(trip);
				break;
			}
		}
		if(haySabotaje){ // CHECKEO SABOTAJE
			manejarSabotaje(trip);
			log_info(logger, "Al estar volviendo de un sabotaje, el tripulante %d vuelve a empezar la tarea que estaba haciendo", trip->TID);
			int tiempoDeLaTarea = tareaActual->tiempo;
			trip->tiempoRestanteTarea = tiempoDeLaTarea;
			ejecutarTareaEnExec(trip);
			break;
		}
		if(!trip->estaVivo){
			return;
		}
	}

	if(strcmp(modoDePlanificacion ,"FIFO") == 0){
		trip->terminoTareaActual = true;
	}

	mens = string_new();
	if(trip->terminoTareaActual){

		string_append(&mens, "Termine la tarea ");
		string_append(&mens, tareaActual->nombreTarea);
		string_append(&mens, "\n");

		mandarMensajeBitacora(trip, mens);

		//free(tareaActual);
		trip->tareaActual = NULL;
	}

	if(!trip->estaVivo){
			return;
	}
	if(haySabotaje){ // CHECKEO SABOTAJE
		manejarSabotaje(trip);
	}
}

void desalojar(Tripulante* trip){
	sem_wait(&trip->sem_planificador);
	sem_post(&trip->sem_planificador);

	if(!trip->estaVivo){
		return;
	}

	//sem_wait(&sem_planificador);
	log_info(logger, "El tripulante %d termino su quantum y vuelve a ready", trip->TID);
	trip->quantumRestante = quantum; // reinicio el quantum!
	cambiarDeEstado(estadoExec, estadoReady, trip,'R');
	sem_post(&sem_multitarea); // si sacamos de exec, entonces se libera un espacio
	//copiaDeGradoMultitarea++;
	//log_info(logger, "GRADO DE MULTITAREA: %d \n", copiaDeGradoMultitarea);
	sem_post(&sem_tripulante_ready); // aviso que hay alguien en ready
	//sem_post(&sem_planificador);
	sem_wait(&trip->sem_planificador);
	sem_post(&trip->sem_planificador);
	if(haySabotaje){ // CHECKEO SABOTAJE
		manejarSabotaje(trip);
	}

	if(!trip->estaVivo){
		return;
	}
}

void manejarDesalojo(Tripulante* trip){

	if(strcmp(modoDePlanificacion ,"RR") == 0){ // si es FIFO no hace nada
		log_info(logger, "Como el algoritmo es RR cuento el quantum para el tripulante %d, por ahora le queda %d", trip->TID, trip->quantumRestante);
		if(haySabotaje){ // CHECKEO SABOTAJE
			manejarSabotaje(trip);
		}
		log_info(logger, "DESALOJO TRIP %d", trip->TID);
		desalojar(trip);
	}
}

//------ya no se usa----------------//
void contarQuantum(Tripulante* trip){
	int quantumRestante = trip->quantumRestante;
	log_info(logger, "quantum restante de %d", quantumRestante);

	for(int i = 0; i < quantumRestante; i++){
		log_info(logger, "sleep de retardo ciclo de %d", retardoCicloCPU);
		sleep(retardoCicloCPU);
		(trip->quantumRestante) --;
		if(haySabotaje){ // CHECKEO SABOTAJE
			manejarSabotaje(trip);
			break;
		}
	}

	log_info(logger, "Termino el quantum del tripulante %d", trip->TID);
}
//----------------------------------//

void informarYHacerMovimientoDePosiciones(Tripulante* trip){
	sem_wait(&trip->sem_planificador);
	sem_post(&trip->sem_planificador);
	//sem_wait(&sem_planificador);
	trip->movimientoParaBitacora = string_new();
	char* posiInicial = string_new();
	char* posiFinal = string_new();
	Tarea* tarea = trip->tareaActual; // si hago esto y no un memcpy lo que hago es que esten apuntando al mismo lugar
	uint32_t cantMovimientos = 0;
	char* rta;

	Tripulante* tripulanteMapa = trip;
	uint32_t tam = 4 * sizeof(uint32_t); // id, x, y, PID

	string_append(&posiInicial, string_itoa(trip->posicionX));
	string_append(&posiInicial, "|");
	string_append(&posiInicial, string_itoa(trip->posicionY));

	string_append(&posiFinal, string_itoa(tarea->posX));
	string_append(&posiFinal, "|");
	string_append(&posiFinal, string_itoa(tarea->posY));

	if(!trip->estaVivo){
		return;
	}

	if(haySabotaje){ // CHECKEO SABOTAJE
		manejarSabotaje(trip);
	}

	// posicion x

	if(tarea->posX > trip->posicionX){
		for(int x = trip->posicionX; x < tarea->posX; x++){
			string_append(&trip->movimientoParaBitacora, "Me muevo de ");
			string_append(&trip->movimientoParaBitacora, string_itoa(trip->posicionX));
			string_append(&trip->movimientoParaBitacora, "|");
			string_append(&trip->movimientoParaBitacora, string_itoa(trip->posicionY));
			string_append(&trip->movimientoParaBitacora, " a ");
			string_append(&trip->movimientoParaBitacora, string_itoa(x + 1));
			string_append(&trip->movimientoParaBitacora, "|");
			string_append(&trip->movimientoParaBitacora, string_itoa(trip->posicionY));
			string_append(&trip->movimientoParaBitacora, "\n");
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
		for(int x = trip->posicionX; x > tarea->posX; x--){
			string_append(&trip->movimientoParaBitacora, "Me muevo de ");
			string_append(&trip->movimientoParaBitacora, string_itoa(trip->posicionX));
			string_append(&trip->movimientoParaBitacora, "|");
			string_append(&trip->movimientoParaBitacora, string_itoa(trip->posicionY));
			string_append(&trip->movimientoParaBitacora, " a ");
			string_append(&trip->movimientoParaBitacora, string_itoa(x - 1));
			string_append(&trip->movimientoParaBitacora, "|");
			string_append(&trip->movimientoParaBitacora, string_itoa(trip->posicionY));
			string_append(&trip->movimientoParaBitacora, "\n");
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

	if(tarea->posY > trip->posicionY){
		for(int y = trip->posicionY; y < tarea->posY; y++){
			string_append(&trip->movimientoParaBitacora, "Me muevo de ");
			string_append(&trip->movimientoParaBitacora, string_itoa(trip->posicionX));
			string_append(&trip->movimientoParaBitacora, "|");
			string_append(&trip->movimientoParaBitacora, string_itoa(trip->posicionY));
			string_append(&trip->movimientoParaBitacora, " a ");
			string_append(&trip->movimientoParaBitacora, string_itoa(trip->posicionX));
			string_append(&trip->movimientoParaBitacora, "|");
			string_append(&trip->movimientoParaBitacora, string_itoa(y + 1));
			string_append(&trip->movimientoParaBitacora, "\n");
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
		for(int y = trip->posicionY; y > tarea->posY; y--){
			string_append(&trip->movimientoParaBitacora, "Me muevo de ");
			string_append(&trip->movimientoParaBitacora, string_itoa(trip->posicionX));
			string_append(&trip->movimientoParaBitacora, "|");
			string_append(&trip->movimientoParaBitacora, string_itoa(trip->posicionY));
			string_append(&trip->movimientoParaBitacora, " a ");
			string_append(&trip->movimientoParaBitacora, string_itoa(trip->posicionX));
			string_append(&trip->movimientoParaBitacora, "|");
			string_append(&trip->movimientoParaBitacora, string_itoa(y - 1));
			string_append(&trip->movimientoParaBitacora, "\n");
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

	//free(tripulanteMapa); // estoy haciendo un free del trip en si? checkear
	if(!trip->estaVivo){
		return;
	}
	if(haySabotaje){ // CHECKEO SABOTAJE
		manejarSabotaje(trip);
	}

	if(strcmp(posiInicial, posiFinal) == 0){
		log_info(logger, "No hizo falta mover al tripulante %d porque ya se encontraba en la posicion %s.", trip->TID, posiFinal);
	}
	else {
		mandarMensajeBitacora(trip, trip->movimientoParaBitacora);
		log_info(logger, "Se movio el tripulante %d de %s a %s. Se aviso a RAM y Mongo", trip->TID, posiInicial, posiFinal);
	}

	free(trip->movimientoParaBitacora);

	trip->quantumRestante = trip->quantumRestante - cantMovimientos; // cada movimiento consume 1

	//sem_post(&sem_planificador); //equilibra el de arriba
	sem_wait(&trip->sem_planificador);
	sem_post(&trip->sem_planificador);

	if(!trip->estaVivo){
		return;
	}
	if(haySabotaje){ // CHECKEO SABOTAJE
		manejarSabotaje(trip);
	}

	if (trip->quantumRestante <= 0){
		manejarDesalojo(trip);
	}
}

void matarTripYCancelarHiloSiCorresponde(Tripulante* tripulante) {
	tripulante->estaVivo = false;

	//free(tripulante->tareaActual);

	bool patotaIndicada(void* unaPatota) {
		Patota* patota = unaPatota;
		return patota->PID == *(tripulante->punteroPID);
	}

	pthread_mutex_lock(&mutex_patota_list);
	Patota* patotaDelTrip = list_find(listaDePatotas, patotaIndicada);
	pthread_mutex_unlock(&mutex_patota_list);

	//log_info(logger, "El tripulante %d se encontro en la patota %d", tripulante->TID, patotaDelTrip->PID);

	pthread_cancel(tripulante->hiloTrip);
	pthread_join(tripulante->hiloTrip, NULL); // no se puede cancelar un hilo detach! espero a que se termine de cancelar
	//free(tripulante); // LIBERO AL TRIP?

	bool nadieEstaVivo(Tripulante* tripu){
		return !tripu->estaVivo;
	}

	if(list_all_satisfy(patotaDelTrip->tripulantes, (void*)nadieEstaVivo)){ //si no quedan mas tripulantes, elimino la patota entera!

		void liberarATodos(Tripulante* trip){
			//pthread_cancel(trip->hiloTrip);
			//pthread_join(trip->hiloTrip, NULL);
			free(trip); // LIBERO AL TRIP?
		}

		Patota* eliminada = eliminarPatota(patotaDelTrip);
		list_iterate(eliminada->tripulantes, (void*)liberarATodos);
		free(eliminada);
	}
}

void expulsarTripulante(Tripulante* trip, bool terminoTareas){
	uint32_t idDelTrip = trip->TID;

	pthread_mutex_lock(&trip->mutex_socket);
	conectarseA(trip, "RAM");
	paquete(trip->socket_RAM, trip, 2 * sizeof(uint32_t), EXPULSAR_TRIPULANTE, true); //id trip y id patota

	switch(trip->estado){
		case 'N':;
				eliminarDeEstado(estadoNew, trip);
				agregarAEstado(estadoExit, trip);
				break;
		case 'R':;
				eliminarDeEstado(estadoReady, trip);
				agregarAEstado(estadoExit, trip);
				break;
		case 'E':;
				eliminarDeEstado(estadoExec, trip);
				sem_post(&sem_multitarea); // si sacamos de exec, entonces se libera un espacio
				//copiaDeGradoMultitarea++;
				//log_info(logger, "GRADO DE MULTITAREA: %d \n", copiaDeGradoMultitarea);
				agregarAEstado(estadoExit, trip);
				break;
		case 'B':;
				eliminarDeEstado(estadoBlockedIO, trip);
				agregarAEstado(estadoExit, trip);
				break;
	}

	actualizarCharEstado(trip, 'X');

	char* okDeRam = recibirUnPaqueteConUnMensaje(trip->socket_RAM);
	pthread_mutex_unlock(&trip->mutex_socket);

	close(trip->socket_Mongo);
	close(trip->socket_RAM);

	if(terminoTareas){
		log_info(logger, "El tripulante %d termino todas sus tareas, por lo que es expulsado de la nave", idDelTrip);
	}
	else{
		log_info(logger, "El tripulante %d fue expulsado de la nave", idDelTrip);
	}

	free(okDeRam);

	matarTripYCancelarHiloSiCorresponde(trip);
}
