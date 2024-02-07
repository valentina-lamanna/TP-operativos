/*
 * util-mi-ram-hq.c
 *
 *  Created on: May 22, 2021
 *      Author: utnso
 */

#include "utils-mi-ram-hq.h"


void atenderCliente(int socket_cliente){
	int cod_op = 0;
	while(cod_op != -1){
		cod_op = recibir_operacion(socket_cliente);
		switch(cod_op)
		{
		case INICIAR_PATOTA:;
			log_info(logger, "[OPERACION] INICIAR_PATOTA"); //xq el [operacion]
			Patota* patota = NULL;
			patota = recibir_patota(socket_cliente); //en algun momento se le asignan los diferentes valores del struct Patota ?
			log_info(logger, "Me llego correctamente la patota. \n");

			//hago chequeo de memoria: tamanio de PCB + todasLasTareas + cantTripualtnes * tamanio TCB
			uint32_t tamAGuardar = 9999999;
			if(patota != NULL){
				tamAGuardar = tamPCB + strlen(patota->todasLasTareas) + patota->cantTripulantes * tamTCB;
			}

			if(hayTamSuficienteEnMemoria(tamAGuardar)) {
				enviar_mensaje("OK", socket_cliente, INICIAR_PATOTA);
			}
			else {
				enviar_mensaje("NO HAY ESPACIO SUFICIENTE", socket_cliente, INICIAR_PATOTA);
				break; // si no hay lugar ya ta
			}

			agregarPatotaDeListaDePatotas(patota);

			t_list* tripulantes = list_create();

			if(strcmp(esquemaMemoria,"PAGINACION") == 0){

				for(int i=0; i < patota->cantTripulantes; i++){
					Tripulante* tripulante = NULL; // variable local
					recibir_operacion(socket_cliente); //no me interesa igual, pero tengo que hacer el recive
					tripulante = recibir_tripulante(socket_cliente);
					log_info(logger, "Me llego correctamente el tripulante y fue deserializado\n");
					list_add(tripulantes, tripulante);
					dibujarTripulante(tripulante->TID, tripulante->posicionX, tripulante->posicionY);
				}

				iniciarPatotaPaginacion(patota, tripulantes, tamAGuardar);
				enviar_mensaje("CREADOS", socket_cliente, INICIAR_PATOTA);
			}
			else if(strcmp(esquemaMemoria,"SEGMENTACION") == 0){
				//uint32_t direccPCB = 0;
				//direccPCB = iniciarPatotaSegmentacion(patota); // ahi adentro esta la logica para hacerlo con paginacion o segmentacion

				log_info(logger, "INICIO_TRIPULANTES");
				for(int i=0; i < patota->cantTripulantes; i++){
					// guardar TCB
					Tripulante* tripulante; // variable local
					recibir_operacion(socket_cliente); //no me interesa igual, pero tengo que hacer el recive
					tripulante = recibir_tripulante(socket_cliente);
					log_info(logger, "Me llego correctamente el tripulante y fue deserializado\n");
					list_add(tripulantes, tripulante);
					//iniciarTripulanteSegmentacion(tripulante, direccPCB, patota->PID);
					dibujarTripulante(tripulante->TID, tripulante->posicionX, tripulante->posicionY);
				}

				iniciarPatotaConTripsSegmentacion(patota, tripulantes);
				enviar_mensaje("CREADOS", socket_cliente, INICIAR_PATOTA);
			}


			break;
		case PROX_TAREA:;
			log_info(logger, "[OPERACION] PROX_TAREA");
			IdsPatotaYTrip ids = recibirTIDyPID(socket_cliente);
			Patota* patotaElegida = buscarPatotaPorId(listaPatotas, ids.PID);

			pthread_mutex_lock(&patotaElegida->mutex_tablas);
			TCB* tripABuscar = encontrarOActualizarTCB(patotaElegida, ids.TID, NULL, false);
			pthread_mutex_unlock(&patotaElegida->mutex_tablas);

			uint32_t indexEnTareas = tripABuscar->proximaTarea;

			if(strcmp(esquemaMemoria,"SEGMENTACION") == 0){
				pthread_mutex_lock(&patotaElegida->mutex_tablas);
				char* tareas = encontrarTareasSegunEstrucAdmin(patotaElegida, 0);
				pthread_mutex_unlock(&patotaElegida->mutex_tablas);

				/*int i = 0;
				char** arrayTareas; //string_split(tareas, "\n");

				arrayTareas[0] = strtok(tareas, "\n");

				while(arrayTareas[i] != NULL){
					i++;
					arrayTareas[i] = strtok(NULL, "\n");
					log_info(logger, "\n tarea: %s \n", arrayTareas[i]);
				}*/
				tareas[patotaElegida->longitudTareas] = '\0';

				char** arrayTareas = string_split(tareas, "\n");
				//char* tarea = malloc(100);
				//tarea = "FIN";

				if(indexEnTareas >= patotaElegida->cuantasTareasHay){
					//tarea = "FIN"; // ya no hay mas tareas!
					enviar_mensaje("FIN", socket_cliente, PROX_TAREA);
					log_info(logger, "Envie correctamente la proxima tarea\n");
				}
				else{
					if(arrayTareas[indexEnTareas] != NULL){
						//strcpy(tarea, arrayTareas[indexEnTareas]);
						enviar_mensaje(arrayTareas[indexEnTareas], socket_cliente, PROX_TAREA);
						log_info(logger, "Envie correctamente la proxima tarea\n");
					}
					else{ // no deberia entrar aca
						enviar_mensaje("FIN", socket_cliente, PROX_TAREA);
						log_info(logger, "Envie correctamente la proxima tarea\n");
					}

					indexEnTareas ++;

					tripABuscar->proximaTarea = indexEnTareas;

					// no me gusta mucho esto porque hace mucho laburo, pero no encontre otra solucion por ahora
					pthread_mutex_lock(&patotaElegida->mutex_tablas);
					encontrarOActualizarTCB(patotaElegida, ids.TID, tripABuscar, true);
					pthread_mutex_unlock(&patotaElegida->mutex_tablas);
				}



				string_iterate_lines(arrayTareas, (void*) free);
				free(arrayTareas);
				//free(tarea);
				free(tareas);
			}
			else if(strcmp(esquemaMemoria,"PAGINACION") == 0){

				pthread_mutex_lock(&patotaElegida->mutex_tablas);
				char* tarea = encontrarTareasSegunEstrucAdmin(patotaElegida, indexEnTareas);
				pthread_mutex_unlock(&patotaElegida->mutex_tablas);

				log_info(logger, "La tarea pedida fue: %s", tarea);

				if(strcmp(tarea, "FIN") != 0){
					indexEnTareas ++;

					tripABuscar->proximaTarea = indexEnTareas;

					// no me gusta mucho esto porque hace mucho laburo, pero no encontre otra solucion por ahora
					pthread_mutex_lock(&patotaElegida->mutex_tablas);
					encontrarOActualizarTCB(patotaElegida, ids.TID, tripABuscar, true);
					pthread_mutex_unlock(&patotaElegida->mutex_tablas);
				}

				enviar_mensaje(tarea, socket_cliente, PROX_TAREA);
				log_info(logger, "Envie correctamente la proxima tarea\n");
			}

			break;

		case EXPULSAR_TRIPULANTE:;
			log_info(logger, "[OPERACION] EXPULSAR_TRIPULANTE");
			//no hay que hacer un free, hay que poner el flag de ocupado en 0 (o sea indicar que esta libre)
			IdsPatotaYTrip idsAExpulsar = recibirTIDyPID(socket_cliente);
			Patota* patotaDelTripAExpulsar = buscarPatotaPorId(listaPatotas, idsAExpulsar.PID);

			pthread_mutex_lock(&mutex_expulsar_trip);

			if(patotaDelTripAExpulsar != NULL){ // si es null es un bajon, no deberia pasar, pero bueno la validacion no esta de mas
				if(strcmp(esquemaMemoria,"SEGMENTACION") == 0)
				{
						pthread_mutex_lock(&patotaDelTripAExpulsar->mutex_tablas);
						Segmento* segmentoDelTrip = recorrerListaSegmentos(patotaDelTripAExpulsar->tablaSegmentos, idsAExpulsar.PID, idsAExpulsar.TID);
						patotaDelTripAExpulsar->cantTripulantes --;

						if(segmentoDelTrip != NULL){
							if(patotaDelTripAExpulsar->cantTripulantes == 0){
								log_info(logger, "Al quedarse sin tripulantes, se elimina la patota %d", idsAExpulsar.PID);
								TCB* tripAExpulsar = encontrarOActualizarTCB(patotaDelTripAExpulsar, idsAExpulsar.TID, NULL, false);
								Segmento* segTareas = recorrerListaSegmentos(patotaDelTripAExpulsar->tablaSegmentos, idsAExpulsar.PID, 0);
								Segmento* segPCB = devolverSegmentoPCB(idsAExpulsar.PID);
								if(segPCB!= NULL){
									log_info(logger, "Se eliminara el segmento de tareas que iba de %d a %d", segTareas->base, (segTareas->base + segTareas->tamanio - 1));
									log_info(logger, "Se eliminara el segmento del PCB que iba de %d a %d", segPCB->base, (segPCB->base + segPCB->tamanio - 1));
									marcarSegmentoComoLibre(segPCB);
									marcarSegmentoComoLibre(segTareas);
									eliminarPatotaDeListaDePatotas(patotaDelTripAExpulsar);
								}
								else{
									log_error(logger, "No existe el segmento de la patota pedida...");
								}
							}
							marcarSegmentoComoLibre(segmentoDelTrip);
							log_info(logger, "Se eliminara el segmento del TCB que iba de %d a %d", segmentoDelTrip->base, (segmentoDelTrip->base + segmentoDelTrip->tamanio - 1));
						}
						else {
							log_error(logger, "No existe el segmento del tripulante pedido...");
						}

						eliminarTripDelMapa(idsAExpulsar.TID);

						pthread_mutex_unlock(&patotaDelTripAExpulsar->mutex_tablas);
				}
				else if(strcmp(esquemaMemoria,"PAGINACION") == 0)
				{
					expulsarTripEnPaginacion(patotaDelTripAExpulsar, idsAExpulsar.TID); // ahi adentro esta la logica de eliminar patota si es necesario
					eliminarTripDelMapa(idsAExpulsar.TID);
				}

				enviar_mensaje("ELIMINADO", socket_cliente, EXPULSAR_TRIPULANTE);
				pthread_mutex_unlock(&mutex_expulsar_trip);
			}
			else{
				log_error(logger, "No existe la patota pedida...");
			}
			break;
		case ACTUALIZAR_ESTADO:;
			log_info(logger, "[OPERACION] ACTUALIZAR_ESTADO");
			//con puntero.
			EstadoTrip infoNuevoEstado = recibirActualizarEstado(socket_cliente);
			Patota* patotaElegida2 = buscarPatotaPorId(listaPatotas, infoNuevoEstado.PID); // para que no tenga el mismo nombre que arriba le puse patotaElegida2

			//pthread_mutex_lock(&patotaElegida2->mutex_tablas);
			TCB* tripAActualizar = encontrarOActualizarTCB(patotaElegida2, infoNuevoEstado.TID, NULL, false);
			//pthread_mutex_unlock(&patotaElegida2->mutex_tablas);

			log_info(logger, "El tripulante %d, se actualiza del estado %c al %c", tripAActualizar->TID, tripAActualizar->estado, infoNuevoEstado.nuevoEstado);

			tripAActualizar->estado = infoNuevoEstado.nuevoEstado;

			// no me gusta mucho esto porque hace mucho laburo, pero no encontre otra solucion por ahora
			//pthread_mutex_lock(&patotaElegida2->mutex_tablas);
			encontrarOActualizarTCB(patotaElegida2, infoNuevoEstado.TID, tripAActualizar, true);
			//pthread_mutex_unlock(&patotaElegida2->mutex_tablas);

			enviar_mensaje("ACTUALIZADO", socket_cliente, ACTUALIZAR_ESTADO);

			break;

		case RECIBIR_UBICACION_TRIPULANTE:;
			log_info(logger, "[OPERACION] RECIBIR_UBICACION_TRIPULANTE");
			//recibo ubicacion x|y un PCB y un PCB
			UbicacionTrip nuevaUbicacion = recibirNuevaUbicacion(socket_cliente);
			Patota* patotaDelTrip = buscarPatotaPorId(listaPatotas, nuevaUbicacion.PID);

			pthread_mutex_lock(&patotaDelTrip->mutex_tablas);
			TCB* tripConNuevaUbicacion = encontrarOActualizarTCB(patotaDelTrip, nuevaUbicacion.TID, NULL, false);
			pthread_mutex_unlock(&patotaDelTrip->mutex_tablas);

			log_info(logger, "El tripulante %d, se mueve de %d|%d a %d|%d", tripConNuevaUbicacion->TID, tripConNuevaUbicacion->posicionX, tripConNuevaUbicacion->posicionY, nuevaUbicacion.posX, nuevaUbicacion.posY);

			tripConNuevaUbicacion->posicionX = nuevaUbicacion.posX;
			tripConNuevaUbicacion->posicionY = nuevaUbicacion.posY;
			actualizarUbicacionTripEnMapa(nuevaUbicacion.TID, nuevaUbicacion.posX, nuevaUbicacion.posY);

			log_info(logger, "Me llego correctamente el tripulante y se modifico en el mapa\n");

			// no me gusta mucho esto porque hace mucho laburo, pero no encontre otra solucion por ahora
			pthread_mutex_lock(&patotaDelTrip->mutex_tablas);
			encontrarOActualizarTCB(patotaDelTrip, nuevaUbicacion.TID, tripConNuevaUbicacion, true);
			pthread_mutex_unlock(&patotaDelTrip->mutex_tablas);

			enviar_mensaje("LISTO", socket_cliente, RECIBIR_UBICACION_TRIPULANTE);

			break;
		case -1:;
			log_error(logger, "el cliente se desconecto. Terminando servidor");
			break;
		default:;
			log_warning(logger, "Operacion desconocida. No quieras meter la pata");
			break;
		}
	}
}

void agregarPatotaDeListaDePatotas(Patota* patota){
	pthread_mutex_lock(&mutex_patotas);
	list_add(listaPatotas, patota);
	pthread_mutex_unlock(&mutex_patotas);
}

void eliminarPatotaDeListaDePatotas(Patota* patota){
	pthread_mutex_lock(&mutex_patotas);
	bool esLaPatota(Patota* pato){
		return pato->PID == patota->PID;
	}
	list_remove_by_condition(listaPatotas,(void*) esLaPatota);
	pthread_mutex_unlock(&mutex_patotas);
}

char** separarTareasPorTarea (char* tareas)
{
	// "GENERAR_OXIGENO 12;2;3;5\nCONSUMIR_OXIGENO 120;2;3;1\nGENERAR_COMIDA 4;2;3;1\nCONSUMIR_COMIDA 1;2;3;4\nGENERAR_BASURA 12;2;3;5\nDESCARTAR_BASURA;3;1;7"
	// devolver "GENERAR_OXIGENO 12;2;3;5"
	char** tareaString = string_split(tareas, "\n");

	return tareaString;
}


Patota* buscarPatotaPorId(t_list* listPatotas, uint32_t patotaId)
{
	Patota* patotaElegida;

	for(int i = 0; i < list_size(listPatotas); i++)
	{
		Patota* patotaDeLaLista = list_get(listPatotas,i);
		if(patotaId == patotaDeLaLista->PID)
		{
			patotaElegida = patotaDeLaLista;
			break;
		}
	}

	return patotaElegida;
}

uint64_t timestamp(void) { //lru
	struct timeval valor;
	gettimeofday(&valor, NULL);
	//unsigned long long result = ((unsigned long long )valor.tv_sec) * 1000 + ((unsigned long long) valor.tv_usec) / 1000;
	//uint64_t tiempo = result;
	return valor.tv_sec*(uint64_t)1000000 + valor.tv_usec;
}


t_list* deserializarTCBs(void* todosLosTCBs, uint32_t cantidadDeTCBs) {
	int offset = 0;
	t_list* listaDeTCBs = list_create();

	for(int i = 0; i < cantidadDeTCBs; i++){
		TCB* unTcb;

		unTcb = deserializarTCB(todosLosTCBs + offset);
		offset += tamTCB;

		list_add(listaDeTCBs, unTcb);
	}

	return listaDeTCBs;
}

TCB* deserializarTCB(void* unTCB) {
	int offset = 0;
	TCB* tcbDeserializado = malloc(tamTCB);

	memcpy(&(tcbDeserializado->TID), unTCB + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(&(tcbDeserializado->estado), unTCB + offset, sizeof(char));
	offset += sizeof(char);
	memcpy(&(tcbDeserializado->posicionX), unTCB + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(&(tcbDeserializado->posicionY), unTCB + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(&(tcbDeserializado->proximaTarea), unTCB + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(&(tcbDeserializado->punteroPCB), unTCB + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	return tcbDeserializado;
}

void* serializarTCB(TCB* unTCB) {
	int offset = 0;
	void* tcbSerializado = malloc(tamTCB);

	memcpy(tcbSerializado + offset, &(unTCB->TID), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(tcbSerializado + offset, &(unTCB->estado), sizeof(char));
	offset += sizeof(char);
	memcpy(tcbSerializado + offset, &(unTCB->posicionX), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(tcbSerializado + offset, &(unTCB->posicionY), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(tcbSerializado + offset, &(unTCB->proximaTarea), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(tcbSerializado + offset, &(unTCB->punteroPCB), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	return tcbSerializado;
}

/* 		//	POR SI EN ALGUN MOMENTO SE NECESITA COMO TAREA	//
Tarea separarTareasPorTarea (char* tareas) //tambien se podria hacer con string_substring ? VER
{
	// "GENERAR_OXIGENO 12;2;3;5\nCONSUMIR_OXIGENO 120;2;3;1\nGENERAR_COMIDA 4;2;3;1\nCONSUMIR_COMIDA 1;2;3;4\nGENERAR_BASURA 12;2;3;5\nDESCARTAR_BASURA;3;1;7\n"

	Tarea tarea;
	// si es "char**" es como un array de string sin longitud, si le queres poner longitud es "char* array[10]"
	// le saque los "[7]" y "[5]" solo porque no me compilaba
	char** tareaString;
	char** parametros;

	// chicas perdon soy lu, yo como discordiador estoy esperando que me pasen un string tipo "GENERAR_OXIGENO 12;2;3;5" o "DESCARTAR_BASURA;3;1;7"
	// y no algo del tipo Tarea :(

	tareaString = string_split(tareas, '\n');
	for(int i = 0; i < string_length(tareas); i++) //o lo ponemos como uint32_t mejor?
	{
		tarea.nombreTarea = string_split(tareaString[i], ' ');
		parametros = string_split(tareaString[i], ';');
		tarea.parametros = parametros[0];
		tarea.posX = parametros[1];
		tarea.posY = parametros[2];
		tarea.tiempo = parametros[3];

		if(tarea.nombreTarea == 'DESCARTAR_BASURA')
		{
			tarea.posX = parametros[0];
			tarea.posY = parametros[1];
			tarea.tiempo = parametros[2];
		}
	}
	return tarea;
}
*/


int esperar_cliente(int socket_servidor)
{
	struct sockaddr_in dir_cliente;
	int tam_direccion = sizeof(struct sockaddr_in);

	int socket_cliente = accept(socket_servidor, (void*) &dir_cliente, &tam_direccion);

	log_info(logger, "Se conecto un cliente con socket %d", socket_cliente);

	return socket_cliente;
}

int recibir_operacion(int socket_cliente)
{
	int cod_op;
	if(recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) != 0)
		return cod_op;
	else
	{
		close(socket_cliente);
		return -1;
	}
}

void enviar_mensaje(char* mensaje, int socket_cliente, op_code operacion)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = operacion;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(mensaje) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

	int bytes = paquete->buffer->size + sizeof(int) + sizeof(op_code); // tam del mensaje en si + tam del parametro "size" + tam del parametro de op_code

	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
	eliminar_paquete(paquete);
}

void* recibir_buffer_simple(int* size, int socket_cliente)
{
	void * buffer;

	recv(socket_cliente, size, sizeof(int), MSG_WAITALL); // el resultado se guarda en size
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	// el MSG_WAITALL significa 'hasta que no recibas t o d o el espacio que te dije, no retornes de la llamada al sistema'

	return buffer;
}


char* recibir_mensaje(int socket_cliente)
{
	int size;
	char* buffer = recibir_buffer_simple(&size, socket_cliente);
	//log_info(logger, "Me llego el mensaje %s", buffer);
	//free(buffer);
	return buffer;
}

Tripulante* recibir_tripulante(int socket_cliente)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->buffer = malloc(sizeof(t_buffer));

	recv(socket_cliente, &(paquete->buffer->size), sizeof(int), MSG_WAITALL);

	paquete->buffer->stream = malloc(paquete->buffer->size);
	recv(socket_cliente, paquete->buffer->stream, paquete->buffer->size, MSG_WAITALL);

	Tripulante* tripulante = deserializar_tripulante(paquete->buffer);

	eliminar_paquete(paquete);
	return tripulante;
}

IdsPatotaYTrip recibirTIDyPID(int socket_cliente)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->buffer = malloc(sizeof(t_buffer));

	recv(socket_cliente, &(paquete->buffer->size), sizeof(int), MSG_WAITALL);

	paquete->buffer->stream = malloc(paquete->buffer->size);
	recv(socket_cliente, paquete->buffer->stream, paquete->buffer->size, MSG_WAITALL);

	uint32_t tripulanteId = 0;
	uint32_t patotaId = 0;

	void* stream = paquete->buffer->stream;

	memcpy(&tripulanteId, stream, sizeof(uint32_t));
	stream+= sizeof(uint32_t);

	memcpy(&(patotaId), stream, sizeof(uint32_t));

	log_info(logger, "Ya recibi al tripulante %d de la patota %d", tripulanteId, patotaId);

	IdsPatotaYTrip ids;

	ids.PID = patotaId;
	ids.TID = tripulanteId;

	eliminar_paquete(paquete);

	return ids;
}

EstadoTrip recibirActualizarEstado(int socket_cliente)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->buffer = malloc(sizeof(t_buffer));

	recv(socket_cliente, &(paquete->buffer->size), sizeof(int), MSG_WAITALL);

	paquete->buffer->stream = malloc(paquete->buffer->size);
	recv(socket_cliente, paquete->buffer->stream, paquete->buffer->size, MSG_WAITALL);

	uint32_t tripulanteId = 0;
	uint32_t patotaId = 0;
	char estado;

	void* stream = paquete->buffer->stream;

	memcpy(&tripulanteId, stream, sizeof(uint32_t));
	stream+= sizeof(uint32_t);

	memcpy(&(estado), stream, sizeof(char));
	stream+= sizeof(char);

	memcpy(&(patotaId), stream, sizeof(uint32_t));

	log_info(logger, "Ya recibi al tripulante %d de la patota %d para cambiar su estado a %c", tripulanteId, patotaId, estado);

	EstadoTrip infoNuevoEstado;

	infoNuevoEstado.PID = patotaId;
	infoNuevoEstado.TID = tripulanteId;
	infoNuevoEstado.nuevoEstado = estado;

	eliminar_paquete(paquete);

	return infoNuevoEstado;
}

UbicacionTrip recibirNuevaUbicacion(int socket_cliente)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->buffer = malloc(sizeof(t_buffer));

	recv(socket_cliente, &(paquete->buffer->size), sizeof(int), MSG_WAITALL);

	paquete->buffer->stream = malloc(paquete->buffer->size);
	recv(socket_cliente, paquete->buffer->stream, paquete->buffer->size, MSG_WAITALL);

	uint32_t tripulanteId = 0;
	uint32_t patotaId = 0;
	uint32_t posicionX = 0;
	uint32_t posicionY = 0;

	void* stream = paquete->buffer->stream;

	memcpy(&tripulanteId, stream, sizeof(uint32_t));
	stream+= sizeof(uint32_t);

	memcpy(&(posicionX), stream, sizeof(uint32_t));
	stream+= sizeof(uint32_t);

	memcpy(&(posicionY), stream, sizeof(uint32_t));
	stream+= sizeof(uint32_t);

	memcpy(&(patotaId), stream, sizeof(uint32_t));

	log_info(logger, "Ya recibi al tripulante %d de la patota %d para actualizar su ubicacion a %d | %d", tripulanteId, patotaId, posicionX, posicionY);

	UbicacionTrip infoNuevaUbicacion;

	infoNuevaUbicacion.PID = patotaId;
	infoNuevaUbicacion.TID = tripulanteId;
	infoNuevaUbicacion.posX = posicionX;
	infoNuevaUbicacion.posY = posicionY;

	eliminar_paquete(paquete);

	return infoNuevaUbicacion;
}

Patota* recibir_patota(int socket_cliente)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->buffer = malloc(sizeof(t_buffer));

	recv(socket_cliente, &(paquete->buffer->size), sizeof(int), MSG_WAITALL);

	paquete->buffer->stream = malloc(paquete->buffer->size);
	recv(socket_cliente, paquete->buffer->stream, paquete->buffer->size, MSG_WAITALL);

	Patota* patota = deserializar_patota(paquete->buffer);

	log_info(logger, "Ya deserialice a la patota %d \n", patota->PID);

	eliminar_paquete(paquete);
	return patota;
}

Tripulante* deserializar_tripulante(t_buffer* buffer)
{
	Tripulante* tripulante = malloc(sizeof(Tripulante));

	void* stream = buffer->stream;

	memcpy(&(tripulante->TID), stream, sizeof(uint32_t));
	stream+= sizeof(uint32_t);
	//memcpy(&(tripulante->estado), stream, sizeof(char));
	//stream+= sizeof(char);

	tripulante->estado = 'N'; //apenas se recibe es NEW, si llegamos a necesitar esta funcion para otro caso, le pedimos al discordiador que nos mande esto

	memcpy(&(tripulante->posicionX), stream, sizeof(uint32_t));
	stream+= sizeof(uint32_t);
	memcpy(&(tripulante->posicionY), stream, sizeof(uint32_t));
	stream+= sizeof(uint32_t);
	memcpy(&(tripulante->patotaPID), stream, sizeof(uint32_t));
	stream+= sizeof(uint32_t);

	log_info(logger, "Ya deserialice al tripulante %d \n", tripulante->TID);
	return tripulante;
}

Patota* deserializar_patota(t_buffer* buffer)
{
	Patota* patota = malloc(sizeof(Patota));

	void* stream = buffer->stream;
	uint32_t tamanio_todasLasTareas = 0;

	memcpy(&(patota->PID), stream, sizeof(uint32_t));
	stream+= sizeof(uint32_t);

	memcpy(&(patota->cantTripulantes), stream, sizeof(uint32_t));
	stream+= sizeof(uint32_t);

	memcpy(&tamanio_todasLasTareas, stream, sizeof(uint32_t));
	stream+= sizeof(uint32_t);

	patota->todasLasTareas = malloc(tamanio_todasLasTareas);

	memcpy(patota->todasLasTareas, stream, tamanio_todasLasTareas);

	log_info(logger, "Ya deserialice a la patota %d \n", patota->PID);
	return patota;
}

void* serializar_paquete(t_paquete* paquete, int bytes)
{
	void * magic = malloc(bytes);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(op_code));
	desplazamiento+= sizeof(op_code);
	memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);

	return magic;
}

void crear_buffer(t_paquete* paquete)
{
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 0;
	paquete->buffer->stream = NULL;
}

void eliminar_paquete(t_paquete* paquete)
{
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

void esperarConexion(void){
	int socket_servidor = iniciar_servidor();
	log_info(logger, "Servidor listo para recibir al tripulante");
	while(1)
		{
			int socket_cliente = esperar_cliente(socket_servidor);
			pthread_t nueva_conexion;
			pthread_create(&nueva_conexion,NULL, (void*)atenderCliente, (void*)socket_cliente);
			pthread_detach(nueva_conexion);
		}
}

char uintToChar(uint32_t numero){

    char a;

    a = numero + '0';

    return a;

}




