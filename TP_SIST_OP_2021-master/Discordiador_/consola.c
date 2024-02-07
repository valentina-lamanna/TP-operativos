#include "consola.h"


void leer_consola(void)
{
	char* leido;

	mostrar_indicaciones_por_consola();
	log_info(logger, "Consola escuchando");

	leido = readline(">");

	// A T O D O LO QUE MANDES EL READLINE LE AGREGA EL '/0' DE LOS STRINGS!

	while(strcmp(leido, "") != 0)
	{
		atender_consola(leido);
		free(leido);
		mostrar_indicaciones_por_consola();
		leido = readline(">");
	}

	//leido = "INICIAR_PATOTA 5 /home/utnso/tareas/tareasPatota5.txt 1|1 3|4";

	//free(leido);

}

void mostrar_indicaciones_por_consola(void)
{
	printf("Para INICIAR_PATOTA ingrese 0 \n");
	printf("Para LISTAR_TRIPULANTES ingrese 1 \n");
	printf("Para EXPULSAR_TRIPULANTE ingrese 2 \n");
	printf("Para INICIAR_PLANIFICACION ingrese 3 \n");
	printf("Para PAUSAR_PLANIFICACION ingrese 4 \n");
	printf("Para OBTENER_BITACORA ingrese 5 \n");
	//printf("Para ver el estado de las colas ingrese 6 \n");
}

op_code definirOperacion(char* operacionEnString)
{
	if(strcmp(operacionEnString, "INICIAR_PATOTA") == 0 || strcmp(operacionEnString, "0") == 0)// esto significa "si son iguales"
	{
		return INICIAR_PATOTA;
	}

	if(strcmp(operacionEnString, "LISTAR_TRIPULANTES") == 0 || strcmp(operacionEnString, "1") == 0)
	{
		return LISTAR_TRIPULANTES;
	}

	if(strcmp(operacionEnString, "EXPULSAR_TRIPULANTE") == 0 || strcmp(operacionEnString, "2") == 0)
	{
		return EXPULSAR_TRIPULANTE;
	}

	if(strcmp(operacionEnString, "INICIAR_PLANIFICACION") == 0 || strcmp(operacionEnString, "3") == 0)
	{
		return INICIAR_PLANIFICACION;
	}

	if(strcmp(operacionEnString, "PAUSAR_PLANIFICACION") == 0 || strcmp(operacionEnString, "4") == 0)
	{
		return PAUSAR_PLANIFICACION;
	}

	if(strcmp(operacionEnString, "OBTENER_BITACORA") == 0 || strcmp(operacionEnString, "5") == 0)
	{
		return OBTENER_BITACORA;
	}

	if(strcmp(operacionEnString, "6") == 0)
	{
		return 6;
	}

	printf("Operacion no valida, se toma como default INICIAR_PATOTA");
	// si no entro a ningun if, devuelve esto:
	return INICIAR_PATOTA;
}

void atender_consola(char* leido)
{
	uint32_t idTripLocal;
	operacion = definirOperacion(leido);

	switch(operacion){
		case INICIAR_PATOTA:;
			log_info(logger, "[OPERACION] INICIAR_PATOTA");
			//"INICIAR_PATOTA 5 /home/utnso/tareas/tareasPatota5.txt 1|1 3|4"

			//DECLARO VARIABLES
			char* leerConsola;
			uint32_t cantTripulantes;
			char* rutaTareas;
			char* posicionEnMapa; //malloc(4 * sizeof(char)); // 3 caracteres de posi de mem + /0
			Patota* patota;
			char* todasLasTareas;

			//PIDO PARAMETROS
			printf("Ingrese la cantidad de tripulantes que quieras tener en la patota");
			leerConsola = readline(">");
			cantTripulantes =(uint32_t)atoi(leerConsola);

			free(leerConsola);

			printf("Ingrese la ruta del archivo de tareas");
			leerConsola = readline(">");

			rutaTareas = leerConsola;

			todasLasTareas = archivo_leer(rutaTareas);

			free(leerConsola);

			int socketPatota = crear_conexion(IPRAM, puertoRAM);
			//int socketPatota = -1;
			patota = crearPatota(idsPatota, cantTripulantes, todasLasTareas);

			// tamPaquete: PID, cant trip, size todasLasTareas + todasLasTareas
			uint32_t tamPaquete = sizeof(patota->PID) + sizeof(patota->cantTripulantes) + sizeof(uint32_t) + strlen(todasLasTareas) + 1;

			pthread_mutex_lock(&mutex_socket_patota);
			paquete(socketPatota, patota, tamPaquete, INICIAR_PATOTA, false);
			pthread_mutex_unlock(&mutex_socket_patota);

			char* buffer = recibirUnPaqueteConUnMensaje(socketPatota);

			if(strcmp(buffer, "OK") != 0){
				log_error(logger, "La patota %d no sera creada por Mi RAM HQ por falta de espacio", idsPatota);
				free(buffer);
				printf("ERROR - ver logs \n");
				break;
			}

			log_info(logger, "La patota %d sera creada por Mi RAM HQ", idsPatota);

			free(buffer);

			agregarPatota(patota);

			pthread_mutex_lock(&mutex_patota_id);
			idsPatota++;
			pthread_mutex_unlock(&mutex_patota_id);

			for(int i = 0; i < cantTripulantes; i++)
			{
				printf("Ingrese la posicion del mapa del tripulante %d con el formato X|Y, o -1 si no desea especificarla", i + 1);
				leerConsola = readline(">");

				posicionEnMapa = leerConsola;

				if(strcmp(posicionEnMapa, "-1") == 0){
					posicionEnMapa = "0|0";
				}

				crearHiloTripulante(patota, idsTripulante, posicionEnMapa, socketPatota);
				pthread_mutex_lock(&mutex_trip_id);
				idsTripulante ++;
				pthread_mutex_unlock(&mutex_trip_id);

				free(leerConsola);
			}

			char* okDeRam = recibirUnPaqueteConUnMensaje(socketPatota); // me devuelve un "CREADO" lo hago xq sino ya se apura a pedir una tarea antes que la RAM llegue a guardarlo
			log_info(logger, "RAM me dijo: %s", okDeRam);

			close(socketPatota);

			void avisarQueSeCrearon(Tripulante* tripu){
				sem_post(&tripu->yaFueCreadoEnRAM);
			}

			list_iterate(patota->tripulantes, (void*)avisarQueSeCrearon);

			break;

		case LISTAR_TRIPULANTES:;
			log_info(logger, "[OPERACION] LISTAR_TRIPULANTES");

			char* fecha = temporal_get_string_time("%d/%m/%y %H:%M:%S");

			void iteradorDeTripulantes(Tripulante* tripulante)
			{
				if(tripulante->estaVivo){
					printf("------------------------------------------------------- \n");
					printf("Tripulante: %d \t Patota: %d \t Status: %c \n", tripulante->TID, *(tripulante->punteroPID), tripulante->estado);
					printf("------------------------------------------------------- \n");
				}
			}

			void iteradorDePatotas(Patota* patota)
			{
				bool tieneTripulantesVivos(Tripulante* tripulante){
					return tripulante->estaVivo;
				}

				if(list_any_satisfy(patota->tripulantes, (void*)tieneTripulantesVivos)){
					list_iterate(patota->tripulantes, (void*)iteradorDeTripulantes);
				}
				else{
					printf("------------------------------------------------------- \n");
					printf("La patota %d no tiene tripulantes (todos fueron expulsados) \n", patota->PID);
					printf("------------------------------------------------------- \n");
				}
			}


			if(!list_is_empty(listaDePatotas)){
				printf("Estado de la Nave \t %s \n", fecha);

				pthread_mutex_lock(&mutex_patota_list); // para que no se pueda iniciar una patota mientras las listo
				list_iterate(listaDePatotas, (void*)iteradorDePatotas);
				pthread_mutex_unlock(&mutex_patota_list);
			}
			else {
				printf("No hay patotas en la Nave \n");
			}

			free(fecha);

			break;

		case EXPULSAR_TRIPULANTE:;
			log_info(logger, "[OPERACION] EXPULSAR_TRIPULANTE");

			printf("Ingrese el TID del tripulante a expulsar");

			char* leiID = readline(">");
			idTripLocal =(uint32_t)atoi(leiID);

			Tripulante* trip2 = buscarTripulantePorId(idTripLocal);

			free(leiID);

			log_info(logger, "Se inicia la expulsion del tripulante %d", trip2->TID);

			expulsarTripulante(trip2, false); // lo expulsan pero no porque termino sus tareas!

			//EXPULSAR_TRIPULANTE 5

			//borrarDePatota(idTripulante, patotaTripulante); OJO, HAY QUE VER EN QUE COLA ESTA Y SACARLO!

			// - MONGO: la bitcora liberarla o algo asi
			// - RAM: sacarlo de la memoria
			// - DISCORDIADOR: sacarlo de la cola de excec

			// tambien necesito acceder al tripulante delsde el PID pasa saber la conexRAM del tripulante
			//para poder mandar el paquete
			//paquete(conexionRAM, Tripulante, EXPULSAR_TRIPULANTE) -> informarle a RAM que haga lo suyo

			break;

		case INICIAR_PLANIFICACION:;
			log_info(logger, "[OPERACION] INICIAR_PLANIFICACION");
			planificacionActivada = true;

			void pasarAReady(Tripulante* tripulante)
			{
				cambiarDeEstado(estadoNew, estadoReady, tripulante, 'R');
				sem_post(&sem_tripulante_ready);
			}

			list_iterate(estadoNew, (void*)pasarAReady);

			//sem_post(&sem_planificador);

			cambiarSemaforos();


			break;

		case PAUSAR_PLANIFICACION:;
			log_info(logger, "[OPERACION] PAUSAR_PLANIFICACION");
			planificacionActivada = false;

			//sem_wait(&sem_planificador); // hace que toda la consola se bloquee a veces :(
			// TENGO QUE HACER UN WAIT POR CADA TRIP?

			cambiarSemaforos();

			//mostar las posiciones de los tripulantes
			break;

		case OBTENER_BITACORA:;
			log_info(logger, "[OPERACION] OBTENER_BITACORA");

			printf("Ingrese el TID del tripulante deseado");
			char* leiID2 = readline(">");
			idTripLocal =(uint32_t)atoi(leiID2);

			Tripulante* trip = buscarTripulantePorId(idTripLocal);

			free(leiID2);

			if(trip != NULL){

				log_info(logger, "Pido la bitacora del tripulante %d", trip->TID);

				quieroLaBitacora(trip, "Quiero mi bitacora");

				char* todaLaBitacora = recibirUnPaqueteConUnMensaje(trip->socket_Mongo);

				log_info(logger, "La bitacora dice: \n %s", todaLaBitacora);

				free(todaLaBitacora);
			}
			else {
				log_error(logger, "No existe el tripulante %d en el discordiador", idTripLocal);
			}


			break;
		case 6:;
			log_info(logger, "[OPERACION] Estado de las colas");

			void listarPresentes(Tripulante* tripulante)
			{
				//log_info(logger, "Tripulante %d", tripulante->TID);
				printf("Tripulante %d\n", tripulante->TID);
			}

			//log_info(logger, "[NEW] Presentes:");
			printf("[NEW] Presentes:\n");
			list_iterate(estadoNew, (void*)listarPresentes);

			//log_info(logger, "[READY] Presentes:");
			printf("[READY] Presentes:\n");
			list_iterate(estadoReady, (void*)listarPresentes);

			//log_info(logger, "[EXEC] Presentes:");
			printf("[EXEC] Presentes:\n");
			list_iterate(estadoExec, (void*)listarPresentes);

			//log_info(logger, "[BLOCKED IO] Presentes:");
			printf("[BLOCKED IO] Presentes:\n");
			list_iterate(estadoBlockedIO, (void*)listarPresentes);

			//log_info(logger, "[BLOCKED EMERGENCIA] Presentes:");
			printf("[BLOCKED EMERGENCIA] Presentes:\n");
			list_iterate(estadoBlockedEmergencia, (void*)listarPresentes);

			//log_info(logger, "[EXIT] Presentes:");
			printf("[EXIT] Presentes:\n");
			list_iterate(estadoExit, (void*)listarPresentes);

			break;
	}
}



/*
char**  comandoConParametros = string_n_split(leido_consola, 20, " ");
	// ["INICIAR_PATOTA", "5", "/home/utnso/tareas/tareasPatota5.txt", "1|1", "3|4"]

	char* operacionEnString = comandoConParametros[0];

	//"INICIAR_PATOTA 5 /home/utnso/tareas/tareasPatota5.txt 1|1 3|4"

	operacion = definirOperacion(operacionEnString);

	// declaramos parametros de comandos
	int cantDeTripulantes;
	char* direccionDeTareas;
	char* direccionesDeTripulantes[20] = {"0|0"}; //puse 20 como exagerado, reveer
	FILE * fArchivoTareas;


	//dentro de case INICIAR_PATOTA
	 * cantDeTripulantes = atoi(comandoConParametros[1]);
			direccionDeTareas = comandoConParametros[2];

			for(int i = 0; i < cantDeTripulantes; i++){
				strcpy(direccionesDeTripulantes[i], asignarDireccDeMemoria(comandoConParametros, cantDeTripulantes)[i]);
			}

			//leo el archivo
			fArchivoTareas = fopen("/home/utnso/workspace/tp-2021-1c-GrupoChicasSistOp2021/tareasPatota.txt","r");

			Tarea reg;

			//crear lista
			t_list* listaTareas;
			listaTareas = list_create();

			fread(&reg,sizeof(Tarea), 1, fArchivoTareas);
			while(!feof (fArchivoTareas))
			{
				printf("%s",reg.nombreTarea);
				printf("%f",reg.parametros);
				printf("%u",reg.posX);
				printf("%u",reg.posY);
				printf("%f",reg.tiempo);
				//añadir el reg leido en la lsita
				list_add(listaTareas, reg);
				fread(&reg,sizeof(Tarea), 1, fArchivoTareas);
			}

			//enviar paquete de la lista a mi ram hq
			//paquete(tripulante->socket_RAM, (void*)listaTareas, logger, INICIAR_PATOTA);


			// SACADO DE ISSUES:
			// PREG: El Discordiador y Mi-RAM HQ pueden leer archivos (como las tareas) o deben hacerlo a través de i-MongoStore?
			// RTA: ... en el caso de las tareas, lo abre el Discordiador, luego el contenido que se lee en ese momento se pasa
			// (creación de patotas y sus tripulantes mediante) por sockets a Mi-RAM HQ para que éste sea el encargado de mantenerlo en memoria.


			Patota* patota;
			patota = crearPatota(idsPatota , (uint32_t)0, cantDeTripulantes); // mi conclusion: inicializar en 0 y cuando la ram responda, lo completo
			list_add(listaDePatotas, patota);

			for(int i = 0; i < cantDeTripulantes; i++){
				// estara bien crearlo asi? o necesitamos 5 variables del tipo pthread_t para manejar cada hilo despues?
				crearHiloTripulante(patota, i + 1, direccionesDeTripulantes[i]);
				// asignarACola(new, trip);
				// avisar a planificador que puede ponerlo en ready cuando quiera, segun su criterio
			}
			idsPatota ++;
*/
