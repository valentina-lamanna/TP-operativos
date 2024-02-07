/*
 * utils-i-mongo-store.c
 *
 *  Created on: May 22, 2021
 *      Author: utnso
 */

#include "utils-i-mongo-store.h"

int iniciar_servidor(void)
{
	int socket_servidor;

    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(IP, puerto, &hints, &servinfo);

    for (p=servinfo; p != NULL; p = p->ai_next)
    {
        if ((socket_servidor = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
            continue;
        uint32_t flag =1;
        setsockopt(socket_servidor,SOL_SOCKET,SO_REUSEPORT,&flag, sizeof(flag));
        if (bind(socket_servidor, p->ai_addr, p->ai_addrlen) == -1) {
            close(socket_servidor);
            continue;
        }
        break;
    }

	listen(socket_servidor, SOMAXCONN);

    freeaddrinfo(servinfo);

    log_trace(logger, "Listo para escuchar a mi cliente");

    return socket_servidor;
}

int esperar_cliente(int socket_servidor)
{
	struct sockaddr_in dir_cliente;
	int tam_direccion = sizeof(struct sockaddr_in);

	int socket_cliente = accept(socket_servidor, (void*) &dir_cliente, &tam_direccion);

	log_info(logger, "Se conecto un cliente con socket %d", socket_cliente);

	return socket_cliente;
}




void* recibir_buffer_simple(int* size, int socket_cliente)
{
	void * buffer;

	recv(socket_cliente, size, sizeof(int), MSG_WAITALL); // el resultado se guarda en size
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	// el MSG_WAITALL significa 'hasta que no recibas todo el espacio que te dije, no retornes de la llamada al sistema'

	return buffer;
}


void eliminar_paquete(t_paquete* paquete)
{
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}
/*
void enviar_mensaje(char* mensaje, int socket_cliente)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = MENSAJE;
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
*/
void enviar_respuesta_a_paquete(char* mensaje, int socket_cliente, op_code operacion)
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

void* parsear(char** datos_de_config) { // se usa para traer del config el array de posiciones de sabotaje
	t_list* lista = list_create();
	t_list* lista_lista = list_create();

	while(*datos_de_config != NULL) {
		char** cadena = string_split(*datos_de_config, "|");

		for(char* d = *cadena; d; d=*++cadena) {
			list_add(lista, d);
		}

		list_add(lista_lista, list_duplicate(lista));
		list_clean(lista);
		datos_de_config++;
	}
	list_destroy(lista);
	return lista_lista;
}

//------------------------------------------DESCERIALIZAR UN PAQUETE--------------------------------------------

char* recibirUnPaqueteDeSabotajes(int socketConexion){
	uint32_t cod_op = -1;
	cod_op = recibir_operacion(socketConexion); // no lo vamos a usar
	return recibir_mensajeSabotaje(socketConexion);
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
void crear_buffer(t_paquete* paquete)
{
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 0;
	paquete->buffer->stream = NULL;
}

char* recibir_mensaje(int socket_cliente)
{
	int size;
	char* buffer = recibir_buffer_simple(&size, socket_cliente);
	log_info(logger, "Me llego el mensaje %s", buffer);

	free(buffer);
	return buffer;
}

bitacora_mensaje* recibir_mensajeBitacora(int socket_cliente)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->buffer = malloc(sizeof(t_buffer));

	recv(socket_cliente, &(paquete->buffer->size), sizeof(int), MSG_WAITALL);

	paquete->buffer->stream = malloc(paquete->buffer->size);
	recv(socket_cliente, paquete->buffer->stream, paquete->buffer->size, MSG_WAITALL);

	bitacora_mensaje* bitacoraMensaje = deserializar_bitacora(paquete->buffer);

	log_info(logger, "Ya deserialice a la accion que va a la bitacora %d \n",bitacoraMensaje->idTrip );

	eliminar_paquete(paquete);
	return bitacoraMensaje;
}


bitacora_mensaje* deserializar_bitacora(t_buffer* buffer)
{
	bitacora_mensaje* bitacora = malloc(sizeof(bitacora_mensaje));

	void* stream = buffer->stream;
	uint32_t tamanio_accion = 0; // a todos los char* los deserializo sabiendo su tam

	memcpy(&( bitacora->idTrip ), stream, sizeof(uint32_t));
	stream+= sizeof(uint32_t);

	memcpy(&tamanio_accion, stream, sizeof(uint32_t));
	stream+= sizeof(uint32_t);

	bitacora->accion = malloc(tamanio_accion + 1);

	memcpy(bitacora->accion, stream, tamanio_accion);


	return bitacora;
}

////////-----------------------------------DESERIALIZAR EL MENSAJE INICIAL DE SABOTAJES
char* recibir_mensajeSabotaje(int socket_cliente)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->buffer = malloc(sizeof(t_buffer));

	recv(socket_cliente, &(paquete->buffer->size), sizeof(int), MSG_WAITALL);

	paquete->buffer->stream = malloc(paquete->buffer->size);
	recv(socket_cliente, paquete->buffer->stream, paquete->buffer->size, MSG_WAITALL);

	void* stream = paquete->buffer->stream;
	uint32_t tamanio_mens = 0; // a todos los char* los deserializo sabiendo su tam

	memcpy(&tamanio_mens, stream, sizeof(uint32_t));
	stream += sizeof(uint32_t);

	char* mensaje = malloc(tamanio_mens);

	memcpy(mensaje, stream, tamanio_mens);

	eliminar_paquete(paquete);
	return mensaje;
}

////////-----------------------------------DESERIALIZAR UN RECURSO
recurso_mensaje* recibir_mensajeRecurso(int socket_cliente)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->buffer = malloc(sizeof(t_buffer));

	recv(socket_cliente, &(paquete->buffer->size), sizeof(int), MSG_WAITALL);

	paquete->buffer->stream = malloc(paquete->buffer->size);
	recv(socket_cliente, paquete->buffer->stream, paquete->buffer->size, MSG_WAITALL);

	recurso_mensaje* recursoMens = deserializar_recurso(paquete->buffer);

	eliminar_paquete(paquete);
	return recursoMens;
}


recurso_mensaje* deserializar_recurso(t_buffer* buffer)
{
	recurso_mensaje* recurso = (recurso_mensaje*)malloc(sizeof(recurso_mensaje));

	void* stream = buffer->stream;
	uint32_t tamanio_recurso = 0; // a todos los char* los deserializo sabiendo su tam

	recurso->caracter= malloc(2); // 1 del caracter , 1 del \0

	memcpy(recurso->caracter, stream, 2);
	stream+= 2;

	memcpy(&(recurso->cantidad), stream, sizeof(int));
	stream+= sizeof(int);

	memcpy(&tamanio_recurso, stream, sizeof(uint32_t));
	stream+= sizeof(uint32_t);

	recurso->recurso = malloc(tamanio_recurso +1);

//	memcpy(recurso->recurso, stream, tamanio_recurso);

	if(recurso->caracter[0] == 'O'){
		recurso->recurso = "OXIGENO";
	}
	else if(recurso->caracter[0]=='C'){
			recurso->recurso = "COMIDA";
	}
	if(recurso->caracter[0]=='B' ){
			recurso->recurso = "BASURA";
		}

	log_info(logger, "Ya deserialice a la accion que va a modificar el recurso %s con el caracter %c \n", recurso->recurso, recurso->caracter[0]);

	return recurso;
}

//------------------------------------------SERIALIZAR UN PAQUETE--------------------------------------------
void paquete(int conexion, void* contenidoPaquete, uint32_t tam, op_code operacion)
{
	t_paquete* paquete = crear_paquete(operacion);

//	serializarSB(paquete, contenidoPaquete, tam);

	enviar_paquete(paquete,conexion);
	log_info(logger, "PAQUETE ENVIADO - OPERACION: %s ", string_itoa(operacion));

	eliminar_paquete(paquete);
}// no se usa

t_paquete* crear_paquete(op_code operacion)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = operacion;
	crear_buffer(paquete);
	return paquete;
}

void enviar_paquete(t_paquete* paquete, int socket_cliente)
{
	// size + codigo op + lo que ocupa en si mandar el size, que es un int
	int bytes = paquete->buffer->size + sizeof(op_code) + sizeof(int);
	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
}

void* serializar_paquete(t_paquete* paquete, int bytes)
{
	void * magic = malloc(bytes);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(paquete->buffer->size));
	desplazamiento+= sizeof(paquete->buffer->size);
	memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
	desplazamiento+= paquete->buffer->size;

	return magic;
}


/*void* serializar_bitacora(bitacora_mensaje* bitacoraMensaje, int bytes)
{

		void * magic = malloc(bytes);
		int desplazamiento = 0;

		memcpy(magic + desplazamiento, &(bitacoraMensaje->idTrip), sizeof(int));
		desplazamiento+= sizeof(int);
		memcpy(magic + desplazamiento, &(bitacoraMensaje->accion), sizeof(bitacoraMensaje->accion));
		desplazamiento+= sizeof(bitacoraMensaje->accion);


		return magic;
}*/
//-----------------------------------------------TRIPULANTES
/*
Tripulante* recibir_tripulante(int socket_cliente)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->buffer = malloc(sizeof(t_buffer));

	// EL CODIGO DE OPERACION YA SE RECIBIO

	// RECIBO BUFFER (Primero su tamaño seguido del contenido)
	recv(socket_cliente, &(paquete->buffer->size), sizeof(int), MSG_WAITALL);

	paquete->buffer->stream = malloc(paquete->buffer->size);
	recv(socket_cliente, paquete->buffer->stream, paquete->buffer->size, MSG_WAITALL);

	// ACA LA IDEA SERIA HACER ALGO CON EL STREAM, DEPENDE EL OP_CODE SE SUPONE QUE SABEMOS
	// QUE COSA VAMOS A RECIBIR (switch con op_code que esta en el archivo que se llama como este pero sin 'utils')

	// PRUEBO DESERIALIZAR UN TRIPULANTE PARA SEGUIR LO QUE PROBAMOS

	Tripulante* tripulante = deserializar_tripulante(paquete->buffer);

	printf("Soy Mongo, ya deserialice al tripulante, su TID es %zu \n", tripulante->TID);

	eliminar_paquete(paquete);
	return tripulante;
}
NO LO USO*/
/*
Tripulante* deserializar_tripulante(t_buffer* buffer)
{
	Tripulante* tripulante = malloc(sizeof(Tripulante));
	tripulante->punteroPCB = malloc(sizeof(uint32_t));

	void* stream = buffer->stream;

	memcpy(&(tripulante->TID), stream, sizeof(uint32_t));
	stream+= sizeof(uint32_t);
	memcpy(&(tripulante->estado), stream, sizeof(char));
	stream+= sizeof(char);
	memcpy(&(tripulante->posicionX), stream, sizeof(uint32_t));
	stream+= sizeof(uint32_t);
	memcpy(&(tripulante->posicionY), stream, sizeof(uint32_t));
	stream+= sizeof(uint32_t);
	memcpy(&(tripulante->proximaInstruccion), stream, sizeof(uint32_t));
	stream+= sizeof(uint32_t);
	memcpy(tripulante->punteroPCB, stream, sizeof(uint32_t));
	stream+= sizeof(uint32_t);

	return tripulante;
}
NO LO USAMOS*/
