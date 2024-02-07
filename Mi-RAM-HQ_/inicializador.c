#include "mi-ram-hq.h"

//--------------HEADER--------------//
int iniciar_servidor(void);
//uint32_t iniciarPatotaSegmentacion(Patota*);
//void iniciarTripulanteSegmentacion(Tripulante*, uint32_t, uint32_t); // no la usamos mas
uint32_t iniciarPatotaConTripsSegmentacion(Patota*, t_list*);
//uint32_t iniciarPatotaConTripsPaginacion(Patota*, t_list*, uint32_t);
void iniciarPatotaPaginacion(Patota*, t_list*, uint32_t);
void serializarContenidoDePaginasPatota(Patota*, PCB*, t_list*);
void iniciarMemoriaVirtual(void);
bool esteArchivoExiste(char*);
int size_char_asterisco(char**);

//-----------SEMAFOROS----------------//
void inicializar_semaforos();



//--------------FUNCIONES--------------//
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

uint32_t iniciarPatotaConTripsSegmentacion(Patota* nuevaPatota, t_list* tripulantes){
		uint32_t direccionLogicaInicioTareas = 0;
		t_list* todosLosTCBs = list_create();
		nuevaPatota->listaDeLosTCB = list_create();

		PCB* nuevoPCB = malloc(sizeof(PCB));
		nuevoPCB->PID = nuevaPatota->PID;

		nuevaPatota->tablaSegmentos = list_create();
		pthread_mutex_init(&nuevaPatota->mutex_tablas, NULL);

		uint32_t direccPCB = -1; // inicializo para evitar warnings
		direccPCB = guardarEnMemoria(nuevoPCB, tamPCB, PCBenum, nuevoPCB->PID);

		nuevaPatota->direccPCB = direccPCB;

		pthread_mutex_lock(&nuevaPatota->mutex_tablas);
		Segmento* segmentoDePCB = devolverSegmentoPCB(nuevoPCB->PID);
		pthread_mutex_unlock(&nuevaPatota->mutex_tablas);

		if(segmentoDePCB != NULL){
			list_add(nuevaPatota->tablaSegmentos, segmentoDePCB);

			log_info(logger, "Se agrego a la tabla de segmentos de la patota %d el segmento del PCB que va de %d a %d", nuevaPatota->PID, segmentoDePCB->base, (segmentoDePCB->base + segmentoDePCB->tamanio - 1));
		}
		else {
			log_error(logger, "Hubo algun problema en obtener el segmento del PCB recien creado");
		}

		void agregarAListaTCBs(Tripulante* trip){
			TCB* tcbTripualnte = malloc(tamTCB);
			tcbTripualnte->TID = trip->TID;
			tcbTripualnte->estado = trip->estado;
			tcbTripualnte->posicionX = trip->posicionX;
			tcbTripualnte->posicionY = trip->posicionY;
			tcbTripualnte->proximaTarea = 0; // apenas se crea, su tarea es la primera
			tcbTripualnte->punteroPCB = direccPCB;

			list_add(todosLosTCBs, tcbTripualnte);
		}

		list_iterate(tripulantes, (void*)agregarAListaTCBs);

		list_add_all(nuevaPatota->listaDeLosTCB, todosLosTCBs);

		void guardarTodosLosTCB(TCB* unTCB){
			guardarEnMemoria(unTCB, tamTCB, TCBenum, nuevaPatota->PID);
			pthread_mutex_lock(&mutex_segmentos);
			Segmento* segmentoDelTCB =  recorrerListaSegmentos(memoriaListSeg, nuevaPatota->PID, unTCB->TID);
			pthread_mutex_unlock(&mutex_segmentos);


			list_add(nuevaPatota->tablaSegmentos, segmentoDelTCB);

			log_info(logger, "Se agrego a la tabla de segmentos de la patota %d el segmento de un TCB del trip %d", nuevaPatota->PID, unTCB->TID);
			log_info(logger, "El segmento del TCB va de %d a %d", segmentoDelTCB->base, (segmentoDelTCB->base + segmentoDelTCB->tamanio - 1));
			//personaje_crear(nivel, tripulante->TID, tripulante->posicionX, tripulante->posicionY);
		}

		list_iterate(todosLosTCBs, (void*)guardarTodosLosTCB);

		uint32_t tamTareas = strlen(nuevaPatota->todasLasTareas);

		char* tareas = nuevaPatota->todasLasTareas;
		tareas[tamTareas] = '\0';

		char** arrayTareas = string_split(tareas, "\n"); // la ultima puede tener un poco de frag interna
		int cuantasHay = size_char_asterisco(arrayTareas);
		nuevaPatota->cuantasTareasHay = cuantasHay;
		nuevaPatota->longitudTareas = tamTareas;

		direccionLogicaInicioTareas = guardarEnMemoria(tareas, tamTareas, TAREAenum, nuevaPatota->PID);

		nuevoPCB->tareas = direccionLogicaInicioTareas; //OJO QUE ESTO NO QUEDA GUARDADO EN MEMORIA

		pthread_mutex_lock(&nuevaPatota->mutex_tablas);
		Segmento* segmentoDeTareas = recorrerListaSegmentos(memoriaListSeg, nuevoPCB->PID, 0);
		pthread_mutex_unlock(&nuevaPatota->mutex_tablas);

		if(segmentoDeTareas != NULL){
			list_add(nuevaPatota->tablaSegmentos, segmentoDeTareas);

			log_info(logger, "Se agrego a la tabla de segmentos de la patota %d el segmento de tareas que va de %d a %d", nuevaPatota->PID, segmentoDeTareas->base, (segmentoDeTareas->base + segmentoDeTareas->tamanio - 1));
		}
		else {
			log_error(logger, "Hubo algun problema en obtener el segmento de tareas");
		}

		return direccPCB;
}

int size_char_asterisco(char** data){
	int i=0;
	while(data[i]!= NULL){
		i++;
	}
	return i;
}

void iniciarMemoriaVirtual(void){

	// es solo para crear el archivo, lo abro y lo cierro

	memoriaVirtual = malloc(tamSwap);

	if(esteArchivoExiste(pathSwap)){
		remove(pathSwap);
	}

	int fileDescriptor = open(pathSwap, O_WRONLY, 0777); // el ultimo parametro son permisos

	ftruncate(fileDescriptor, tamSwap);

	close(fileDescriptor);
}

void inicializar_semaforos(){

	pthread_mutex_init(&mutex_segmentos, NULL);
	pthread_mutex_init(&mutex_memoria, NULL);
	pthread_mutex_init(&mutex_memoriaVirtual, NULL);
	pthread_mutex_init(&mutex_compactacion, NULL);
	pthread_mutex_init(&mutex_patotas, NULL);
	pthread_mutex_init(&mutex_info_paginas, NULL);
	pthread_mutex_init(&mutex_paginas_memVirtual, NULL);
	pthread_mutex_init(&mutex_pags_ram, NULL);
	pthread_mutex_init(&mutex_frames_ram, NULL);
	pthread_mutex_init(&mutex_clock, NULL);
	pthread_mutex_init(&mutex_expulsar_trip, NULL);

	//pthread_mutex_init(&mutex_atender_operacion, NULL); // al final no

	//sem_init(&sem_consola, 0, 0);
	//sem_init(&sem_sabotaje, 0, 0);
}

bool esteArchivoExiste(char* fname)
{
  int fd=open(fname, O_RDONLY);
  if (fd < 0)         /* error */
    return false;
  /* Si no hemos salido ya, cerramos */
  close(fd);
  return true;
}


//-----------NUEVAS FUNCIONES----------------//
void iniciarPatotaPaginacion(Patota* nuevaPatota, t_list* tripulantes, uint32_t tamAGuardar){

	void* todoElContenido = calloc(1, tamAGuardar);
	int offset = 0;

	//--------------TAREAS--------------//
	int tamTareas = strlen(nuevaPatota->todasLasTareas);
	nuevaPatota->todasLasTareas[tamTareas] = '\0';

	memcpy(todoElContenido, nuevaPatota->todasLasTareas, tamTareas);
	offset += tamTareas;

	//--------------PCB--------------//
	PCB* nuevoPCB = malloc(tamPCB);
	nuevoPCB->PID = nuevaPatota->PID;
	nuevoPCB->tareas = 0;

	memcpy(todoElContenido + offset, &(nuevoPCB->PID), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(todoElContenido + offset, &(nuevoPCB->tareas), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	nuevaPatota->tablaDePaginas = list_create();

	pthread_mutex_init(&nuevaPatota->mutex_tablas, NULL);

	PaginasPatota* infoPaginasPatota = malloc(sizeof(PaginasPatota));
	infoPaginasPatota->PID = nuevaPatota->PID;
	infoPaginasPatota->cantPaginas = (uint32_t)ceil((double)tamAGuardar / tamPagina);
	infoPaginasPatota->cantTCBs = list_size(tripulantes);
	infoPaginasPatota->longitudTareas = tamTareas;
	infoPaginasPatota->direccPCB = tamTareas;
	infoPaginasPatota->idsTripulantes = list_create();
	infoPaginasPatota->infoTareas = list_create();

	void* obtenerListaDeIds(Tripulante* trip){
		return trip->TID;
	}

	infoPaginasPatota->idsTripulantes = list_map(tripulantes, (void*)obtenerListaDeIds);


	char** arrayTareas = string_split(nuevaPatota->todasLasTareas, "\n");
	int identficadorTarea = 0;
	int offsetTarea = 0;
	char* laTarea = arrayTareas[identficadorTarea];

	while (laTarea != NULL){
		TareasPatota* infoTarea = malloc(sizeof(TareasPatota));
		int longitudTarea = strlen(laTarea)+1;
		infoTarea->offset = offsetTarea;
		infoTarea->longitud = longitudTarea;
		log_info(logger, "La tarea %d tiene offset %d", identficadorTarea, offsetTarea);
		offsetTarea += longitudTarea;
		identficadorTarea ++;
		list_add(infoPaginasPatota->infoTareas, infoTarea);
		laTarea = arrayTareas[identficadorTarea];
	}

	//--------------TCBs--------------//

	t_list* todosLosTCBs = list_create();

	void agregarAListaTCBs(Tripulante* trip){
		TCB* tcbTripualnte = malloc(tamTCB);
		tcbTripualnte->TID = trip->TID;
		tcbTripualnte->estado = trip->estado;
		tcbTripualnte->posicionX = trip->posicionX;
		tcbTripualnte->posicionY = trip->posicionY;
		tcbTripualnte->proximaTarea = 0; // apenas se crea, su tarea es la primera
		tcbTripualnte->punteroPCB = tamTareas + tamPCB;

		list_add(todosLosTCBs, tcbTripualnte);
	}

	list_iterate(tripulantes, (void*)agregarAListaTCBs);

	void agregarAlContenidoSerializado(TCB* unTcb){
		memcpy(todoElContenido + offset, &(unTcb->TID), sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(todoElContenido + offset, &(unTcb->estado), sizeof(char));
		offset += sizeof(char);
		memcpy(todoElContenido + offset, &(unTcb->posicionX), sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(todoElContenido + offset, &(unTcb->posicionY), sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(todoElContenido + offset, &(unTcb->proximaTarea), sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(todoElContenido + offset, &(unTcb->punteroPCB), sizeof(uint32_t));
		offset += sizeof(uint32_t);
	}

	list_iterate(todosLosTCBs, (void*)agregarAlContenidoSerializado);

	uint32_t direccDeTodo = guardarEnMemoria(todoElContenido, tamAGuardar, 0, nuevaPatota->PID);

	agregarInfoPaginasPatota(infoPaginasPatota);
}

/*
// NO USAMOS MAS
uint32_t iniciarPatotaSegmentacion(Patota* nuevaPatota){
	//nos pasan PATOTA_ID(un numero) y  LISTA_TAREAS(char*) y cantidad de tareas

	uint32_t direccionLogicaInicioTareas = 0;
	PCB* nuevoPCB = malloc(sizeof(PCB));
	nuevoPCB->PID = nuevaPatota->PID;

	nuevaPatota->tablaSegmentos = list_create();
	pthread_mutex_init(&nuevaPatota->mutex_tablas, NULL);

	//uint32_t* direccLogicaInicioTareas = malloc(sizeof(uint32_t));
	//list_add(memoriaListSeg, nuevoPCB); --> lo saco porque este list_add se hace adentro de guardarEnMemoria

	uint32_t direccPCB = -1; // inicializo para evitar warnings
	direccPCB = guardarEnMemoria(nuevoPCB, tamPCB, PCBenum, nuevoPCB->PID);

	direccionLogicaInicioTareas = guardarEnMemoria(nuevaPatota->todasLasTareas, strlen(nuevaPatota->todasLasTareas), TAREAenum, nuevaPatota->PID);

	nuevoPCB->tareas = direccionLogicaInicioTareas;

	/*Segmento* segmentoDeTareas = malloc(sizeof(Segmento)); // preguntar si esta bien pensado lo de agregar los segmentos a la lista de patota
	segmentoDeTareas->base = direccionLogicaInicioTareas;
	segmentoDeTareas->tamanio = strlen(nuevaPatota->todasLasTareas) + 1;
	segmentoDeTareas->ocupado = 1;						// ESTAMOS CREANDO DOS SEGMENTOS IGUALES, MEJOR ENCONTRAR EL QUE YA EXISTE Y QUE LA PATOTA LO GUARDE TAMBIEN
	segmentoDeTareas->contenido = TAREAenum;
	segmentoDeTareas->PID = nuevaPatota->PID;*/

	//pthread_mutex_lock(&nuevaPatota->mutex_tablas);
	//Segmento* segmentoDeTareas = recorrerListaSegmentos(memoriaListSeg, nuevoPCB->PID, 0);

	/*Segmento* segmentoDePCB = malloc(sizeof(Segmento));
	segmentoDePCB->base = direccPCB;
	segmentoDePCB->tamanio = tamPCB;
	segmentoDePCB->ocupado = 1;
	segmentoDePCB->contenido = PCBenum;
	segmentoDePCB->PID = nuevaPatota->PID;

	Segmento* segmentoDePCB = devolverSegmentoPCB(nuevaPatota->PID);
	pthread_mutex_unlock(&nuevaPatota->mutex_tablas);

	if(segmentoDeTareas != NULL && segmentoDePCB != NULL){
		list_add(nuevaPatota->tablaSegmentos, segmentoDeTareas);
		list_add(nuevaPatota->tablaSegmentos, segmentoDePCB);

		log_info(logger, "Se agregaron a la tabla de segmentos de la patota %d el segmento de tareas y de PCB", nuevaPatota->PID);
		log_info(logger, "El segmento de tareas va de %d a %d y el del PCB de %d a %d", segmentoDeTareas->base, (segmentoDeTareas->base + segmentoDeTareas->tamanio - 1), segmentoDePCB->base, (segmentoDePCB + segmentoDePCB->tamanio - 1));
	}
	else {
		log_error(logger, "Hubo algun problema en obtener el segmento de tareas o PCB recien creados");
	}

	return direccPCB;

}*/


//NO USAMOS MAS
/*
void iniciarTripulanteSegmentacion(Tripulante* tripulante, uint32_t direccPCB, uint32_t PID){

	TCB* tcbTripulante = malloc(sizeof(TCB));
	tcbTripulante->TID = tripulante->TID;
	tcbTripulante->estado = tripulante->estado;
	tcbTripulante->posicionX = tripulante->posicionX;
	tcbTripulante->posicionY = tripulante->posicionY;
	tcbTripulante->proximaTarea = 0; // apenas se crea, su tarea es la primera
	tcbTripulante->punteroPCB = direccPCB;

	//uint32_t direccTCB = -1;

	//void* tcbSerializado = serializarTCB(tcbTripualnte);

	direccTCB = guardarEnMemoria(tcbTripulante, tamTCB, TCBenum, PID);

	/*Segmento* segmentoDelTCB = malloc(sizeof(Segmento));
	segmentoDelTCB->base = direccTCB;
	segmentoDelTCB->tamanio = tamTCB;
	segmentoDelTCB->ocupado = 1;
	segmentoDelTCB->contenido = TCBenum;
	segmentoDelTCB->PID = PID;

	pthread_mutex_lock(&mutex_segmentos);
	Segmento* segmentoDelTCB =  recorrerListaSegmentos(memoriaListSeg, PID, tcbTripulante->TID);
	pthread_mutex_unlock(&mutex_segmentos);

	/*void agregarSegmentoAPatota(void* elementoPatota){ // ESTO NO FUNCIONABA, PORQUE LA memoriaListSeg GUARDA SOLO COSAS DE TIPO Segmento, NO DE Patota
		Patota* patota = elementoPatota;
		if(patota->PID == tripulante->patotaPID){
			list_add(patota->tablaSegmentos, segmentoDelTCB);
			log_info(logger, "Se agrego a la tabla de segmentos de la patota %d el segmento de un TCB del trip %d", patota->PID, tripulante->TID);
			log_info(logger, "El segmento del TCB va de %d a %d", segmentoDelTCB->base, segmentoDelTCB->limite);
		}
	}

	list_iterate(memoriaListSeg, agregarSegmentoAPatota);

	Patota* patotaDelTripulante = buscarPatotaPorId(listaPatotas, tripulante->patotaPID);

	list_add(patotaDelTripulante->tablaSegmentos, segmentoDelTCB);

	log_info(logger, "Se agrego a la tabla de segmentos de la patota %d el segmento de un TCB del trip %d", patotaDelTripulante->PID, tripulante->TID);
	log_info(logger, "El segmento del TCB va de %d a %d", segmentoDelTCB->base, (segmentoDelTCB->base + segmentoDelTCB->tamanio - 1));


	//personaje_crear(nivel, tripulante->TID, tripulante->posicionX, tripulante->posicionY);
}
*/

//-------------------------------------------//

/*		VERSION CORTITA:
void serializarContenidoDePaginasPatota(Patota* patota, PCB* elPCB, t_list* TCBs) {

	// tenemos que inventar una estructura adminsitrativa extra para poder identificar que estamos guardando en cada pagina!!

	int offset = 0;

	memcpy(patota->contenidoSerializadoParaPaginas + offset, &(elPCB->PID), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(patota->contenidoSerializadoParaPaginas + offset, &(elPCB->tareas), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	void agregarAlContenidoSerializado(TCB* unTcb){
		memcpy(patota->contenidoSerializadoParaPaginas + offset, &(unTcb->TID), sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(patota->contenidoSerializadoParaPaginas + offset, &(unTcb->estado), sizeof(char));
		offset += sizeof(char);
		memcpy(patota->contenidoSerializadoParaPaginas + offset, &(unTcb->posicionX), sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(patota->contenidoSerializadoParaPaginas + offset, &(unTcb->posicionY), sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(patota->contenidoSerializadoParaPaginas + offset, &(unTcb->proximaTarea), sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(patota->contenidoSerializadoParaPaginas + offset, unTcb->punteroPCB, sizeof(uint32_t));
		offset += sizeof(uint32_t);
	}

	list_iterate(TCBs, agregarAlContenidoSerializado);

	memcpy(patota->contenidoSerializadoParaPaginas + offset, patota->todasLasTareas, strlen(patota->todasLasTareas) + 1);
	offset += strlen(patota->todasLasTareas) + 1;
}*/
