/*
 * utils-mi-ram-hq.h
 *
 *  Created on: May 22, 2021
 *      Author: utnso
 */

#ifndef UTILS_MI_RAM_HQ_H_
#define UTILS_MI_RAM_HQ_H_

#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netdb.h>
#include<commons/log.h>
#include<commons/collections/list.h>
#include<string.h>
#include<stdbool.h>
#include<pthread.h>
#include<semaphore.h>
#include<nivel-gui/nivel-gui.h>
#include<nivel-gui/tad_nivel.h>
#include<commons/config.h>
#include<math.h>
#include<signal.h>//para recibir la senial del dump de memoria o de compactacion
#include <fcntl.h> // para el "open" de archivos mapeados a memoria
#include <sys/mman.h> // para el mmap
#include <sys/types.h>
#include <sys/stat.h>
#include<commons/temporal.h>



//----------- ESTRUCTURAS ---------------//

typedef enum
{
	INICIAR_PATOTA, // esto NO ES para recibir mensajes, es interno para guardar TCB en vez de PCB
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
	ACTUALIZAR_ESTADO,
	RECIBIR_UBICACION_TRIPULANTE
}op_code;


typedef struct{
	int size;
	void* stream;
} t_buffer;


typedef struct{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;


typedef struct{
	uint32_t 	TID;
	char     	estado;
	uint32_t 	posicionX;
	uint32_t 	posicionY;
	uint32_t 	proximaTarea;
	uint32_t 	punteroPCB; // PID de la patota
} TCB;


typedef struct{ 	// esto se guarda en un segmento
	uint32_t 	PID;
	uint32_t 	tareas; //Dirección lógica del inicio de las Tareas
} PCB;
//creamos la patota, los pcb, los id trip y se lo mandamos a discordiador

/* typedef struct {	// esto es "como vive la patota en el modulo de MiRamHQ", principalmente, su id y su tabla de segmentos
	uint32_t	PID;
	uint32_t	cantTripulantes;
	char*		todasLasTareas; // se tiene que hacer una logica para "separarlas" y devolver solo la prox tarea a ejecutar
    t_list* 	tablaSegmentos;
} Patota; */

typedef struct{
	uint32_t 			PID;
	uint32_t			cantTripulantes;
	char*				todasLasTareas;
	t_list* 			tablaSegmentos;
	t_list*				tablaDePaginas;
	pthread_mutex_t		mutex_tablas; // hago uno solo porque nunca es segmentacion y paginacion a la vez
	t_list*				listaDeLosTCB;
	uint32_t			direccPCB;
	uint32_t			cuantasTareasHay;
	uint32_t			longitudTareas;
} Patota;

typedef struct{
	uint32_t 	TID;
	char     	estado;
	uint32_t 	posicionX;
	uint32_t 	posicionY;
	uint32_t	patotaPID;
} Tripulante; // cosas que se pueden recibir del discordiador


typedef enum{
	TCBenum,
	PCBenum,
	TAREAenum
}tipoDeContenido; //lo usamos en segmentacion

typedef struct{
	uint32_t 		base;
	uint32_t 		tamanio;
	uint32_t    	ocupado;
	tipoDeContenido contenido;
	uint32_t		PID; //me sirve en el dump
} Segmento;

typedef struct{
	uint32_t 	nroPagina;
	uint32_t 	nroFrame;
	uint32_t 	estaEnRAM;
	uint32_t 	direccionFisica;
	uint64_t	ultimaReferencia;
	bool		bitDeUso;
	uint64_t	indexEnMemVirtual;
	uint32_t	PID;
	uint32_t	bytesOcupados; // esto me sirve para la parte de expulsar
} Pagina;

typedef struct{
	uint32_t 	nroFrame;
	uint32_t 	nroPagina;
	uint32_t 	ocupado;
	uint32_t 	direccionFisica;
	uint32_t	PID; // esto me sirve para el dump
} Frame;

typedef struct{
	uint32_t	offset;
	uint32_t	longitud;
} TareasPatota;

typedef struct{
	uint32_t	PID;
	uint32_t 	cantPaginas;
	uint32_t 	cantTCBs;
	uint32_t	longitudTareas;
	uint32_t	direccPCB;
	//t_list*		paginas;
	t_list*		idsTripulantes;
	t_list*		infoTareas; // del tipo TareasPatota
} PaginasPatota;

typedef struct{
	char* 		nombreTarea;
	uint32_t  	parametros;
	uint32_t 	posX;
	uint32_t 	posY;
	uint32_t  	tiempo;
} Tarea;


typedef struct{
	uint32_t  	PID;
	uint32_t 	TID;
} IdsPatotaYTrip; //creamos este struct porque cada vez que recibamos el PID vamos a recibir el TID tambien

typedef  struct
{
	uint32_t 	TID;
	uint32_t 	PID;
	char		nuevoEstado;
} EstadoTrip; // parecido al de arriba (IdsPatotaYTrip) para para ACTUALIZAR ESTADO

typedef struct{
	uint32_t 	TID;
	uint32_t	PID;
	uint32_t	posX;
	uint32_t	posY;
} UbicacionTrip;


//-----------VARIABLES GLOBALES---------//
t_config* config;
t_log* logger;
NIVEL* mapa;

int 	tamMemoria;
char* 	esquemaMemoria;
int 	tamPagina;
int 	tamSwap;
char* 	pathSwap;
char* 	algoritmoReemplazo;
char* 	criterioSeleccion;
char* 	IP;
char* 	puerto;

void*   	memoria;
void*		memoriaVirtual;

uint32_t tamTCB;
uint32_t tamPCB;

uint32_t punteroParaClock;




//-----------SEMAFOROS---------//
pthread_mutex_t mutex_segmentos;
pthread_mutex_t mutex_info_paginas;
pthread_mutex_t mutex_memoria;
pthread_mutex_t mutex_memoriaVirtual;
pthread_mutex_t mutex_compactacion;
pthread_mutex_t mutex_patotas;
pthread_mutex_t mutex_paginas_memVirtual;
pthread_mutex_t mutex_pags_ram;
pthread_mutex_t mutex_frames_ram;
pthread_mutex_t mutex_clock;
pthread_mutex_t mutexMapa;
pthread_mutex_t mutex_expulsar_trip;



//------------LISTAS-----------//
t_list* memoriaListSeg;  //segmentacion --> por cada PCB tabla de segmentos
t_list* memoriaListPag; // del tipo InfoPaginasPatota
t_list* listaPatotas;
t_list* listaFramesTodos; // agregue esta para tener todos los frames que existan en memoria
t_list* listaPaginasGeneral; // del tipo Pagina
t_list* listaPagsVirtual;

t_list* listaInfoPaginasPatota; // del tipo PaginasPatota


//-------- FUNCIONES DE PAQUETES/MENSAJES ---------//
int esperar_cliente(int);
void atenderCliente(int);
void esperarConexion(void);
int recibir_operacion(int);
void* recibir_buffer_simple(int*, int);
char* recibir_mensaje(int);
void enviar_mensaje(char*, int, op_code);
Tripulante* recibir_tripulante(int);
IdsPatotaYTrip recibirTIDyPID(int);
EstadoTrip recibirActualizarEstado(int);
UbicacionTrip recibirNuevaUbicacion(int);
Patota* recibir_patota(int);
Tripulante* deserializar_tripulante(t_buffer*);
Patota* deserializar_patota(t_buffer*);
void* serializar_paquete(t_paquete*, int);
void crear_buffer(t_paquete*);
void eliminar_paquete(t_paquete*);



// ---------------FUNCIONES------------- //
void agregarPatotaDeListaDePatotas(Patota*);
void eliminarPatotaDeListaDePatotas(Patota*);
char** separarTareasPorTarea (char*);
Patota* buscarPatotaPorId(t_list*, uint32_t);
uint64_t timestamp(void);
t_list* deserializarTCBs(void*, uint32_t);
TCB* deserializarTCB(void*);
void* serializarTCB(TCB*);
char uintToChar(uint32_t);



#endif /* UTILS_MI_RAM_HQ_H_ */
