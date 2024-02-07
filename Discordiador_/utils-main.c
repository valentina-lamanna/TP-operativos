/*
 * utils-main.c
 *
 *  Created on: May 22, 2021
 *      Author: utnso
 */


#include "utils-main.h"

void paquete(int conexion, void* contenidoPaquete, uint32_t tam, op_code operacion, bool esTripulante)
{
	t_paquete* paquete = crear_paquete(operacion);

	popular_paquete(paquete, contenidoPaquete, tam, esTripulante);

	enviar_paquete(paquete,conexion);
	//log_info(logger, "PAQUETE ENVIADO - SOCKET: %d, OPERACION: %s ", conexion, string_itoa(operacion));

	eliminar_paquete(paquete);
}

void* serializar_paquete(t_paquete* paquete, int bytes)
{
	void * magic = malloc(bytes);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(op_code));
	desplazamiento+= sizeof(op_code);
	memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(paquete->buffer->size));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);

	return magic;
}

int crear_conexion(char *ip, char* puerto)
{
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &server_info);

	int socket_cliente = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

	while(connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen) == -1)
		sleep(3);

	freeaddrinfo(server_info);

	return socket_cliente;
}


void enviar_mensaje(char* mensaje, int socket_cliente, op_code operacion)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = operacion;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(mensaje) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

	int bytes = paquete->buffer->size + 2*sizeof(int);

	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
	eliminar_paquete(paquete);
}


void crear_buffer(t_paquete* paquete)
{
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 0;
	paquete->buffer->stream = NULL;
}

t_paquete* crear_paquete(op_code operacion)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = operacion;
	crear_buffer(paquete);
	return paquete;
}

void popular_paquete(t_paquete* paquete, void* contenido, uint32_t tamanio, bool esTripulante)
{
	paquete->buffer->size = tamanio;


	pthread_mutex_lock(&mutex_popular_paquetes); // que no se pise como se populan dos paquetes distintos!

	void* stream = malloc(paquete->buffer->size);
	int offset = 0; // Desplazamiento

	uint32_t tamMensaje = 0;

	if(paquete->codigo_operacion == INICIAR_PATOTA && !esTripulante){
		Patota* patota = malloc(sizeof(Patota));
		patota = contenido;

		memcpy(stream + offset, &(patota->PID), sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(stream + offset, &(patota->cantTripulantes), sizeof(uint32_t));
		offset += sizeof(uint32_t);

		tamMensaje = strlen(patota->todasLasTareas) + 1;

		memcpy(stream + offset, &tamMensaje, sizeof(uint32_t));
		offset += sizeof(uint32_t);

		memcpy(stream + offset, patota->todasLasTareas, tamMensaje);
	}

	else if(paquete->codigo_operacion == INICIAR_PATOTA && esTripulante) {

		Tripulante* tripAIniciar = (Tripulante*)contenido;

		memcpy(stream + offset, &(tripAIniciar->TID), sizeof(uint32_t));
		offset += sizeof(uint32_t);
		//memcpy(stream + offset, &(trip->estado), sizeof(char)); A RAM NO LE INTERESA
		//offset += sizeof(char);
		memcpy(stream + offset, &(tripAIniciar->posicionX), sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(stream + offset, &(tripAIniciar->posicionY), sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(stream + offset, tripAIniciar->punteroPID, sizeof(uint32_t));
	}

	else if(paquete->codigo_operacion == PROX_TAREA){
		Tripulante* tripPideTarea = (Tripulante*)contenido;
		memcpy(stream + offset, &(tripPideTarea->TID), sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(stream + offset, tripPideTarea->punteroPID, sizeof(uint32_t));
	}

	else if(paquete->codigo_operacion == ESCRIBIR_BITACORA || paquete->codigo_operacion == NUEVO_TRIP || paquete->codigo_operacion == OBTENER_BITACORA){
		bitacora_mensaje* bitacora = (bitacora_mensaje*)contenido;
		memcpy(stream + offset, &(bitacora->idTrip), sizeof(uint32_t));
		offset += sizeof(uint32_t);

		tamMensaje = strlen(bitacora->accion) + 1;

		memcpy(stream + offset, &tamMensaje, sizeof(uint32_t));
		offset += sizeof(uint32_t);

		memcpy(stream + offset, bitacora->accion, tamMensaje);
	}

	else if(paquete->codigo_operacion == RECIBIR_UBICACION_TRIPULANTE){
		Tripulante* tripAMover = (Tripulante*)contenido;
		memcpy(stream + offset, &(tripAMover->TID), sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(stream + offset, &(tripAMover->posicionX), sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(stream + offset, &(tripAMover->posicionY), sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(stream + offset, tripAMover->punteroPID, sizeof(uint32_t));
	}

	else if(paquete->codigo_operacion == SABOTAJE){
		char* mensajeSabotaje = (char*)contenido;

		tamMensaje = strlen(mensajeSabotaje) + 1;

		memcpy(stream + offset, &tamMensaje, sizeof(uint32_t));
		offset += sizeof(uint32_t);

		memcpy(stream + offset, mensajeSabotaje, tamMensaje);
	}

	else if(paquete->codigo_operacion == AGREGAR || paquete->codigo_operacion == CONSUMIR){
		recurso_mensaje* recurso = (recurso_mensaje*)contenido;

		tamMensaje = strlen(recurso->recurso) + 1;

		memcpy(stream + offset, recurso->caracter, 2); // un char* de un char y un \0
		offset += 2;

		memcpy(stream + offset, &(recurso->cantidad), sizeof(int));
		offset += sizeof(int);

		memcpy(stream + offset, &tamMensaje, sizeof(uint32_t));
		offset += sizeof(uint32_t);

		memcpy(stream + offset, recurso->recurso, tamMensaje);
	}

	else if(paquete->codigo_operacion == ACTUALIZAR_ESTADO){
		Tripulante* nuevoEstadoTrip = (Tripulante*)contenido;
		memcpy(stream + offset, &(nuevoEstadoTrip->TID), sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(stream + offset, &(nuevoEstadoTrip->estado), sizeof(char));
		offset += sizeof(char);
		memcpy(stream + offset, nuevoEstadoTrip->punteroPID, sizeof(uint32_t));
	}

	else if(paquete->codigo_operacion == EXPULSAR_TRIPULANTE){
		Tripulante* tripAExpulsar = (Tripulante*)contenido;
		memcpy(stream + offset, &(tripAExpulsar->TID), sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(stream + offset, tripAExpulsar->punteroPID, sizeof(uint32_t));
	}

	paquete->buffer->stream = stream;

	pthread_mutex_unlock(&mutex_popular_paquetes);

	//free(stream); VER, CREO QUE ESTARIA BIEN ESTE FREE
}

char* recibirUnPaqueteConUnMensaje(int socketConexion){
	uint32_t cod_op = recibir_operacion(socketConexion); // no lo vamos a usar
	int size = 0;
	return recibir_buffer_simple(&size, socketConexion);
}

uint32_t recibir_operacion(int socket_utilizado)
{
	op_code cod_op = -1;
	if(recv(socket_utilizado, &cod_op, sizeof(int), MSG_WAITALL) != 0)
		return cod_op;
	else
	{
		close(socket_utilizado);
		return -1;
	}
}

void pedirTarea (Tripulante* trip){

	pthread_mutex_lock(&trip->mutex_socket);
	conectarseA(trip, "RAM");
	paquete(trip->socket_RAM, trip, 2 * sizeof(uint32_t), PROX_TAREA, true); //id, PID

	char* buffer = recibirUnPaqueteConUnMensaje(trip->socket_RAM);
	pthread_mutex_unlock(&trip->mutex_socket);

	log_info(logger, "El tripulante %d recibio la tarea %s", trip->TID, buffer);

	Tarea* tarea = malloc(sizeof(Tarea));
	char**  tareaArray = string_split(buffer, ";");
	//GENERAR_OXIGENO 12;2;3;5
	// ["GENERAR_OXIGENO 12", "2", "3", "5"]


	char ** nombreYParams = string_split(tareaArray[0], " ");
	tarea->nombreTarea = nombreYParams[0];

	if(strcmp(tarea->nombreTarea, "FIN") != 0){
		tarea->parametros = nombreYParams[1] != NULL ? atoi(nombreYParams[1]) : 0;
		tarea->posX = atoi(tareaArray[1]);
		tarea->posY = atoi(tareaArray[2]);
		tarea->tiempo = atoi(tareaArray[3]);
	}

	if(string_contains(tareasDeIO, tarea->nombreTarea)){
		tarea->esDeIO = true;
		if(strcmp(tarea->nombreTarea, "GENERAR_OXIGENO") == 0){
			tarea->recurso = "OXIGENO";
			tarea->caracterDelRecurso = "O";
			tarea->agregar = true;
		}
		if(strcmp(tarea->nombreTarea, "CONSUMIR_OXIGENO") == 0){
			tarea->recurso = "OXIGENO";
			tarea->caracterDelRecurso = "O";
			tarea->agregar = false;
		}

		if(strcmp(tarea->nombreTarea, "GENERAR_COMIDA") == 0){
			tarea->recurso = "COMIDA";
			tarea->caracterDelRecurso = "C";
			tarea->agregar = true;
		}
		if(strcmp(tarea->nombreTarea, "CONSUMIR_COMIDA") == 0){
			tarea->recurso = "COMIDA";
			tarea->caracterDelRecurso = "C";
			tarea->agregar = false;
		}

		if(strcmp(tarea->nombreTarea, "GENERAR_BASURA") == 0){
			tarea->recurso = "BASURA";
			tarea->caracterDelRecurso = "B";
			tarea->agregar = true;
		}
		if(strcmp(tarea->nombreTarea, "DESCARTAR_BASURA") == 0){
			tarea->recurso = "BASURA";
			tarea->caracterDelRecurso = "B";
			tarea->agregar = false;
		}
	}
	else {
		tarea->esDeIO = false;
		tarea->recurso = "NADA";
		tarea->caracterDelRecurso = "x";
		tarea->agregar = false;
	}

	trip->tareaActual = malloc(sizeof(Tarea));
	trip->tareaActual = tarea;
	int tiempoTarea = tarea->tiempo;
	trip->tiempoRestanteTarea = tiempoTarea;
}

// que cosas puede recibir?
// - sabotajes de mongo --> while infinito, cuando llega mensaje de mongo signal semaforo sabotaje
// - recibir la bitacora (y calculo que imprimirla por pantalla) de mongo
// - avisa ram si se puede crear una patota? no me acuerdo bien eso
// - avisa ram si no hay memoria suficiente? o solo lo loguea y no manda nada a discordiador?
// - algo mas que reciba...


void* recibir_buffer_simple(int* size, int socket_cliente)
{
	void * buffer;

	recv(socket_cliente, size, sizeof(int), MSG_WAITALL); // el resultado se guarda en size
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	// el MSG_WAITALL significa 'hasta que no recibas t o d o el espacio que te dije, no retornes de la llamada al sistema'

	return buffer;
}

void enviar_paquete(t_paquete* paquete, int socket_cliente)
{
	// size + codigo op + lo que ocupa en si mandar el size, que es un int
	int bytes = paquete->buffer->size + sizeof(op_code) + sizeof(int);
	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
}

void eliminar_paquete(t_paquete* paquete)
{
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

void liberar_conexion(int socket_cliente)
{
	close(socket_cliente);
}

Patota* eliminarPatota(Patota* patota){

	bool esLaPatota(Patota* unaPatota) {
		return unaPatota == patota;
	}

	pthread_mutex_lock(&mutex_patota_list);
	Patota* eliminada = list_remove_by_condition(listaDePatotas, (void*)esLaPatota);
	pthread_mutex_unlock(&mutex_patota_list);

	return eliminada;
}

void agregarPatota(Patota* patota){
	pthread_mutex_lock(&mutex_patota_list);
	list_add(listaDePatotas, patota);
	pthread_mutex_unlock(&mutex_patota_list);
}

bool estaEnEstado(Tripulante* trip, t_list* estado){

	bool esElTripulante(Tripulante* tripulante){
			return tripulante == trip;
	}

	Tripulante* unTrip = list_find(estado, (void*)esElTripulante);

	if(unTrip != NULL){
		return true;
	}

	return false;
}

void cambiarSemaforos(void){
	void cambiarSemaforosTripulantes(Tripulante* tripulante)
	{
		if(tripulante->estaVivo){
			if(planificacionActivada){
				sem_post(&tripulante->sem_planificador);
			}
			else{
				sem_wait(&tripulante->sem_planificador);
			}
		}
	}

	void iterarPatotas(Patota* patota)
	{
		bool tieneTripulantesVivos(Tripulante* tripulante){
			return tripulante->estaVivo;
		}

		if(list_any_satisfy(patota->tripulantes, (void*)tieneTripulantesVivos)){
			pthread_mutex_lock(&mutex_lista_semaforos);
			list_iterate(patota->tripulantes, (void*)cambiarSemaforosTripulantes);
			pthread_mutex_unlock(&mutex_lista_semaforos);
		}
	}

	if(!list_is_empty(listaDePatotas)){
		/*pthread_mutex_lock(&mutex_lista_semaforos);
		for(int j = 0; j < list_size(listaDeSemaforosTrips); j++){
			sem_t semaforo = list_get(listaDeSemaforosTrips, j);
			sem_post(&semaforo);
		}
		pthread_mutex_unlock(&mutex_lista_semaforos);*/
		list_iterate(listaDePatotas, (void*)iterarPatotas);
	}
	else {
		printf("No hay patotas en la Nave \n");
	}
}

void mandarMensajeBitacora(Tripulante* trip, char* mensaje){
	bitacora_mensaje* mensajeBitacora = malloc(sizeof(bitacora_mensaje));
	uint32_t idTrip = trip->TID;
	mensajeBitacora->idTrip = idTrip;
	mensajeBitacora->accion = mensaje;

	uint32_t sizeBita = 2 * sizeof(uint32_t) + strlen(mensaje) + 1;

	conectarseA(trip, "MONGO");
	paquete(trip->socket_Mongo, mensajeBitacora, sizeBita, ESCRIBIR_BITACORA, true);
	char* buffer = recibirUnPaqueteConUnMensaje(trip->socket_Mongo);

	free(buffer);
	free(mensajeBitacora);
}

void mandarMensajeIO(Tripulante* trip, uint32_t tamTotal, int cantidad, char* recurso, char* caracter, bool hayQueAgregar){
	recurso_mensaje* recursoMensaje = malloc(tamTotal - sizeof(uint32_t)); // xq el int del tam del recurso no va aca

	recursoMensaje->cantidad = cantidad;
	recursoMensaje->recurso = recurso;
	recursoMensaje->caracter = caracter;

	conectarseA(trip, "MONGO");
	if(hayQueAgregar){
		paquete(trip->socket_Mongo, recursoMensaje, tamTotal, AGREGAR, true);
	}
	else {
		paquete(trip->socket_Mongo, recursoMensaje, tamTotal, CONSUMIR, true);
	}

	char* buffer = recibirUnPaqueteConUnMensaje(trip->socket_Mongo);

	free(recursoMensaje);
}

void quieroLaBitacora(Tripulante* trip, char* mensaje){
	bitacora_mensaje* mensajeBitacora = malloc(sizeof(bitacora_mensaje));
	mensajeBitacora->idTrip = trip->TID;
	mensajeBitacora->accion = mensaje;

	uint32_t sizeBita = 2 * sizeof(uint32_t) + strlen(mensaje) + 1;

	conectarseA(trip, "MONGO");
	paquete(trip->socket_Mongo, mensajeBitacora, sizeBita, OBTENER_BITACORA, true);

	free(mensajeBitacora);
}

void conectarseA(Tripulante* tripulante, char* modulo) {

	if(strcmp(modulo,"RAM") == 0){
		if(tripulante->socket_RAM == -1){
			//close(tripulante->socket_RAM);
			tripulante->socket_RAM = crear_conexion(IPRAM, puertoRAM);
		}
	}
	else if (strcmp(modulo,"MONGO") == 0){
		if(tripulante->socket_Mongo == -1){
			//close(tripulante->socket_Mongo);
			tripulante->socket_Mongo = crear_conexion(IPiMongo, puertoImongo);
		}
	}
}

// POR AHORA NO USAMOS ESTAS FUNCIONES
void agregar_a_paquete(t_paquete* paquete, Tripulante* trip, int tamanio)
{
	//cambia el tamanio de stream a lo que ya tenia + el tam de lo que se agrega + op_code?
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(op_code));

	memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), trip, tamanio);
	paquete->buffer->size += tamanio + sizeof(int);
}


void reaccionarAMensaje(uint32_t socket){
	uint32_t cod_op = -1;
	cod_op = recibir_operacion(socket);

	switch(cod_op)
	{
	case OBTENER_BITACORA:; //es un ejemplo, esta pensado asi nomas
		int size = 0;
		char* buffer = recibir_buffer_simple(&size, socket);
		log_info(logger, "[RESPUESTA] OBTENER_BITACORA");
		//log_info(logger, "La respuesta fue %s", buffer);
		printf("La bitacora del tripulante solicitado es la siguiente: \n %s", buffer);
		free(buffer);
		break;
	case -1:;
		log_error(logger, "Hubo una desconexion recibiendo mensajes. Terminando conexion");
		break;
	default:;
		log_warning(logger, "Operacion desconocida. No quieras meter la pata");
		break;
	}
}
