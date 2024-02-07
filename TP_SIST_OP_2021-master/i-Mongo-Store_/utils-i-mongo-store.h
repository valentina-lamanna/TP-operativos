/*
 * utils-i-mongo-store.h
 *
 *  Created on: May 22, 2021
 *      Author: utnso
 */

#ifndef UTILS_I_MONGO_STORE_H_
#define UTILS_I_MONGO_STORE_H_

#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netdb.h>
#include<commons/log.h>
#include<commons/collections/list.h>
#include<string.h>
#include<semaphore.h>
#include <commons/string.h>
#include<commons/config.h>
#include<commons/bitarray.h>

#include <openssl/md5.h>
#include <sys/stat.h> //tamaños files

#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h> //mmap()

//#include <node.h> // diccionarios
#include <stdbool.h>

//para recibir la senal de sabotajes
#include <signal.h>




typedef enum
{
	INICIAR_PATOTA,
	LISTAR_TRIPULANTES,
	EXPULSAR_TRIPULANTE,
	INICIAR_PLANIFICACION,
	PAUSAR_PLANIFICACION,
	OBTENER_BITACORA,
	PROX_TAREA,
	NUEVO_TRIP, //MONGO
	ESCRIBIR_BITACORA,
	SABOTAJE,//MONGO
	AGREGAR, //una tarea agrega letras al file
	CONSUMIR, //una tarea saca letras del file
	ACTUALIZAR_ESTADO,//RAM
	RECIBIR_UBICACION_TRIPULANTE, //RAM
	SUPER_BLOQUE // ojo que este no o usa nadie, asi que tiene que estar ultimo si o si
}op_code;



typedef struct
{
	int size;
	void* stream;
} t_buffer;


typedef struct
{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;


typedef  struct
{
	uint32_t TID;
	char     estado;
	uint32_t posicionX;
	uint32_t posicionY;
	uint32_t proximaInstruccion;
	uint32_t* punteroPCB; // PID de la patota

} Tripulante;


typedef struct
{
	uint32_t PID;
	uint32_t tareas;
} Patota;

typedef struct{
int idTrip;
char* accion;
}bitacora_mensaje;

typedef struct{
  char* recurso;
  char* caracter;
  int cantidad;
}recurso_mensaje;

// bit_numbering_t esta en la common

// t_bitarray esta en la common


//SEMAFORS
pthread_mutex_t bitmapS;
pthread_mutex_t superBloqueSem;
pthread_mutex_t blockSem;
//pthread_mutex_t diccionarioFilesSem;
pthread_mutex_t diccionarioBitacorasSem;
pthread_mutex_t listaTareasSem;
pthread_mutex_t sincronizadorSem;
pthread_mutex_t oxigenoSem;
pthread_mutex_t basuraSem;
pthread_mutex_t comidaSem;

t_dictionary*  diccionarioFiles;
t_list*  listaBitacora;
t_list*  listaTareas;

// socket sabotajes
int socket_sabotajes;

//contador de sabotajes
int contadorDeSabotajes;



void* block_copia;
void* block_file_final;

t_bitarray* bitmapFS;
void* punteroAlSBdeMMAP;

t_log* logger;


//-----------CONFIG IMONGO
t_config* config;
char* IP;
char* puerto;
int tiempo_sincronizacion;
char* punto_montaje;
char** posicionesDeSabotaje;
int tam_clusters;
int cantidad_clusters;


//t_config* metadata_config;

//------------PATH
char* directorio;
char* superBloquePath;
char* blocksPath;
char* filesPath;
char* bitacoraPath;




int iniciar_servidor(void);
int esperar_cliente(int);
char* recibirUnPaqueteDeSabotajes(int);
int recibir_operacion(int);
void* recibir_buffer_simple(int*, int);
char* recibir_mensaje(int);
char* recibir_mensajeSabotaje(int);
bitacora_mensaje* recibir_mensajeBitacora(int);
bitacora_mensaje* deserializar_bitacora(t_buffer*);

recurso_mensaje* recibir_mensajeRecurso(int);
recurso_mensaje* deserializar_recurso(t_buffer*);


void* serializar_paquete(t_paquete*, int);
void crear_buffer(t_paquete*);
void eliminar_paquete(t_paquete*);
void enviar_mensaje(char*, int);
t_paquete* crear_paquete(op_code);

//t_list* parsear(char**); //para array config

#endif /* UTILS_I_MONGO_STORE_H_ */
