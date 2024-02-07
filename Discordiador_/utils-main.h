/*
 * utils-main.h
 *
 *  Created on: May 22, 2021
 *      Author: utnso
 */

#ifndef UTILS_MAIN_H_
#define UTILS_MAIN_H_


#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include <stdbool.h>
#include<commons/collections/list.h>
#include<pthread.h>
#include<semaphore.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<commons/temporal.h>

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
	ACTUALIZAR_ESTADO, //RAM
	RECIBIR_UBICACION_TRIPULANTE //RAM
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


typedef struct
{
	char* 		nombreTarea;
	uint32_t  	parametros;
	uint32_t 	posX;
	uint32_t 	posY;
	uint32_t  	tiempo;
	bool		esDeIO;
	char*		recurso;
	char*		caracterDelRecurso;
	bool		agregar; // true = agregar, false = consumir
} Tarea;


typedef  struct
{
	uint32_t 			TID;
	char     			estado;
	uint32_t 			posicionX;
	uint32_t 			posicionY;
	uint32_t* 			punteroPID; // PID de la patota
	int					quantumRestante; //necesito que tenga negativos!
	int					socket_RAM;
	int					socket_Mongo;
	Tarea*				tareaActual;
	sem_t				ejecutando;
	pthread_t 			hiloTrip; // lo agrego aca para cancelarlo cuando lo expulsan
	char*				movimientoParaBitacora;
	bool				estaVivo;
	bool				terminoTareaActual;
	int					tiempoRestanteTarea;
	sem_t 				sem_planificador;
	sem_t				yaFueCreadoEnRAM;
	sem_t				sabotaje;
	bool				yoResuelvoSabotaje;
	pthread_mutex_t		mutex_socket;
} Tripulante; //podria agregarse semaforos como parte del struct!

typedef struct
{
	uint32_t 	PID;
	uint32_t 	cantTripulantes;
	t_list*		tripulantes;
	char*		todasLasTareas;
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

// VARIABLES GLOBALES

	t_log* logger;
	t_config* config;

	char* IPRAM;
	char* puertoRAM;
	char* IPiMongo;
	char* puertoImongo;
	char* modoDePlanificacion;
	int quantum;
	int gradoMultitarea;
	int duracionSabotaje;
	int retardoCicloCPU;

	int copiaDeGradoMultitarea;


	t_list* listaDePatotas;
	t_list* listaDeSemaforosTrips;
	t_list* estadoNew;
	t_list* estadoReady;
	t_list* estadoBlockedIO;
	t_list* estadoBlockedEmergencia;
	t_list* estadoExec;
	t_list* estadoExit;

	op_code operacion;

	pthread_t consola;
	pthread_t planificador;
	pthread_t sabotajesManager;
	pthread_t hilo_conexion;

	// SEMAFOROS
	//sem_t sem_consola;
	sem_t sem_sabotaje;
	sem_t sem_sabotaje_tripu_elegido;
	sem_t sem_planificador;
	sem_t sem_multitarea; // cant de gente en exec
	sem_t sem_tripulante_ready;
	sem_t sem_tripulante_exec;

	//SEMAFOROS MUTEX
	pthread_mutex_t mutex_cambio_est;
	pthread_mutex_t mutex_bloqueados;
	pthread_mutex_t mutex_trip_id;
	pthread_mutex_t mutex_patota_id;
	pthread_mutex_t mutex_patota_list;
	pthread_mutex_t mutex_socket_patota;
	pthread_mutex_t mutex_sabotaje;
	pthread_mutex_t mutex_popular_paquetes;
	pthread_mutex_t mutex_lista_semaforos;

	uint32_t idsPatota;
	uint32_t idsTripulante;

	char* tareasDeIO;

	bool haySabotaje;
	bool planificacionActivada;


	void cambiarSemaforos(void);

void leer_consola(void);

void paquete(int, void*, uint32_t, op_code, bool);


void quieroLaBitacora(Tripulante* , char*); // valen : perdon nos abia donde ponerlo

//serializar
int crear_conexion(char*, char*);
void enviar_mensaje(char*, int, op_code);
t_paquete* crear_paquete(op_code);
void conectarseA(Tripulante*, char*);

// recibir mensajes
void esperarMensajes(uint32_t);
uint32_t recibir_operacion(int);
void reaccionarAMensaje(uint32_t);
void* recibir_buffer_simple(int*, int);
char* recibirUnPaqueteConUnMensaje(int);

//crear buffer
void popular_paquete(t_paquete*, void*, uint32_t, bool);
void agregar_a_paquete(t_paquete*,Tripulante*, int);
void enviar_paquete(t_paquete*, int);
void eliminar_paquete(t_paquete*);
void liberar_conexion(int);


t_list* leerArchivoDeTareasReturnTareasList(char*);
int archivo_obtenerTamanio (char*);
char* archivo_leer (char*);

bool estaEnEstado(Tripulante*, t_list*);

void mandarMensajeBitacora(Tripulante*, char*);
void mandarMensajeIO(Tripulante*, uint32_t, int, char*, char*, bool);

void pedirTarea (Tripulante*);

Patota* eliminarPatota(Patota*);
void agregarPatota(Patota*);

#endif /* UTILS_MAIN_H_ */
