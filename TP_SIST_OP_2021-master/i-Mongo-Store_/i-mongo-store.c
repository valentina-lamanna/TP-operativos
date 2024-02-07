/*  Created on: May 22, 2021
 *      Author: utnso
 */

#include "i-mongo-store.h"
#include "sabotaje.h"
#include "iniciarFS.h"

int main(void)
{


	config = iniciar_config();
	leerConfig();

	logger = log_create("i-mongo-store.log", "iMongoStore", 1, LOG_LEVEL_DEBUG);

	iniciar_semaforos();

	iniciar_FS();

	esperarSabotaje();

	listaBitacora = list_create();
	listaTareas = list_create();
	ponerEnListaTareas(listaTareas, "OXIGENO");
	ponerEnListaTareas(listaTareas, "BASURA");
	ponerEnListaTareas(listaTareas, "COMIDA");



	esperarConexion();



	return EXIT_SUCCESS;
}

t_config* iniciar_config(void)
{
	return config_create("i-mongo-store.config");
}

void leerConfig(){
	IP = strdup(config_get_string_value(config,"IP"));
	puerto = strdup(config_get_string_value(config,"PUERTO"));
	tiempo_sincronizacion = config_get_int_value(config,"TIEMPO_SINCRONIZACION");
	punto_montaje = strdup(config_get_string_value(config,"PUNTO_MONTAJE"));
	posicionesDeSabotaje = config_get_array_value(config, "POSICIONES_SABOTAJE");
	cantidad_clusters=config_get_int_value(config,"CANTCLUSTER");
	tam_clusters=config_get_int_value(config,"TAMCLUSTER");
}

void iniciar_semaforos(){
	pthread_mutex_init(&bitmapS,NULL);
//	pthread_mutex_init(&diccionarioFilesSem,NULL);
	pthread_mutex_init(&diccionarioBitacorasSem,NULL);
	pthread_mutex_init(&listaTareasSem,NULL);
	pthread_mutex_init(&sincronizadorSem,NULL);
	pthread_mutex_init(&superBloqueSem,NULL);
	pthread_mutex_init(&blockSem,NULL);
	pthread_mutex_init(&oxigenoSem,NULL);
	pthread_mutex_init(&basuraSem,NULL);
	pthread_mutex_init(&comidaSem,NULL);




}
void atenderCliente(int socket_cliente){
	int cod_op = 0;
	while(cod_op != -1){
		cod_op = recibir_operacion(socket_cliente);

		switch(cod_op)
		{
		case NUEVO_TRIP:;
			bitacora_mensaje* mensaje1 = malloc(sizeof(bitacora_mensaje));
			mensaje1= recibir_mensajeBitacora(socket_cliente);
			log_info(logger, "Ya recibio el mensaje para iniciar la bitacora del tripulante de TID %d \n", mensaje1->idTrip);


			ponerEnListaBitacora(listaBitacora, string_itoa(mensaje1->idTrip));

			char* path =string_new();
			string_append(&path,bitacoraPath);
			string_append(&path,"/Tripulante");
			string_append(&path,string_itoa(mensaje1->idTrip));
			string_append(&path,".ims");

			if(esteArchivoExiste(path)){
				t_bitacora bita;
				bita= leerBitacoraConfig(path);
				int tam= (int)ceil((double)bita.size / tam_clusters);
				for(int i=0 ; i< tam ; i++){
					liberarBitmap(bita.blocks[i]);
				}
				free(bita.blocks);
			}
				crearBitacora(mensaje1->idTrip);
				escrbirBitacora(mensaje1->idTrip, mensaje1->accion);
				enviar_respuesta_a_paquete("LISTO", socket_cliente, NUEVO_TRIP);

				free(path);
			break;
		case OBTENER_BITACORA:;
			bitacora_mensaje* mensaje2 = malloc(sizeof(bitacora_mensaje));
			mensaje2= recibir_mensajeBitacora(socket_cliente);

			if(estaEnListaBitacoras(listaBitacora, string_itoa(mensaje2->idTrip))){
				char* bit=obtenerBitacora(mensaje2->idTrip);
				enviar_respuesta_a_paquete(bit, socket_cliente, OBTENER_BITACORA);
				free(bit);
			}
			else{
				log_error(logger, "esta pidiendo la bitacora de un tripulante que no tiene \n");
				enviar_respuesta_a_paquete("ERROR", socket_cliente, OBTENER_BITACORA);
			}

			break;
		case ESCRIBIR_BITACORA:;
			bitacora_mensaje* mensaje3 = malloc(sizeof(bitacora_mensaje));
			mensaje3= recibir_mensajeBitacora(socket_cliente);

			if(estaEnListaBitacoras(listaBitacora, string_itoa(mensaje3->idTrip))){
				escrbirBitacora(mensaje3->idTrip, mensaje3->accion);
			}else{
				log_error(logger, "esta queriendo escribir en una bitacora que no existe \n");
			}

			enviar_respuesta_a_paquete("LISTO", socket_cliente, ESCRIBIR_BITACORA);

			break;
		case SABOTAJE:;

			socket_sabotajes = socket_cliente;
			char* rta = recibir_mensajeSabotaje(socket_sabotajes);
			log_info(logger, "EL DISCORDAIDOR NOS MANDO ESTE MENSAJE :: %s", rta);
			cod_op = -1; // asi sale del while, no nos interesa mas que siga aca, sigue en sabotaje.c
			break;

		case AGREGAR:;
		recurso_mensaje* recurso = recibir_mensajeRecurso(socket_cliente);
		char* path2 =string_new();
		string_append(&path2, filesPath);
		string_append(&path2, "/");
		string_append(&path2,recurso->recurso);
		string_append(&path2,".ims");



		if(esteArchivoExiste(path2)){
			escribirFile(recurso->recurso,recurso->cantidad);
		}else{
		    crearFile(recurso->recurso, recurso->caracter);
			escribirFile(recurso->recurso,recurso->cantidad);
		}




		enviar_respuesta_a_paquete("LISTO", socket_cliente, AGREGAR);
		free(path2);
			break;

		case CONSUMIR:;
			recurso_mensaje* recurso2 = recibir_mensajeRecurso(socket_cliente);

			char* path3 =string_new();
			string_append(&path3, filesPath);
			string_append(&path3, "/");
			string_append(&path3,recurso2->recurso);
			string_append(&path3,".ims");
			if(strcmp(recurso2->recurso, "BASURA")== 0){
				if(esteArchivoExiste(path3)){
					t_file file;
					file= leer_files(path3);
					int tam= (int)ceil((double)file.size / tam_clusters);
					for(int i=0 ; i< tam ; i++){
						liberarBitmap(file.blocks[i]);
					}
					free(file.blocks);
					remove(path3);

				}else {
					log_error(logger, "No se puede borrar la tarea  s% proque nunca se creo \n", recurso2->recurso);
				}
			}else{
				if(esteArchivoExiste(path3)){
					borrarDelFile(recurso2->recurso, recurso2->cantidad);
				}
				else{
					log_error(logger, "No se puede borrar la tarea  s% proque nunca se creo \n", recurso2->recurso);
				}
			}

			enviar_respuesta_a_paquete("LISTO", socket_cliente, CONSUMIR);
			free(path3);
			break;
		case -1:;
			log_error(logger, "el cliente se desconecto. Terminando servidor \n");
			break;
		default:;
//			log_warning(logger, "Operacion desconocida. No quieras meter la pata \n");
			break;
		}
	}
}
int esteArchivoExiste(char* fname)
{
  int fd=open(fname, O_RDONLY);
  if (fd<0)         /* error */
    return 0;
  /* Si no hemos salido ya, cerramos */
  close(fd);
  return 1;
}

void ponerEnListaBitacora(t_list* diccionarioBlocks,char* key){
	pthread_mutex_lock(&diccionarioBitacorasSem);
	list_add(diccionarioBlocks, key);
	pthread_mutex_unlock(&diccionarioBitacorasSem);
}
bool estaEnListaBitacoras(t_list* diccionarioBlocks,char* key){
	char* retorno = NULL;

 	pthread_mutex_lock(&diccionarioBitacorasSem);

 	bool mifuncion(char* elem){
 		return strcmp(key, elem) == 0;
 	}
 	retorno = list_find(diccionarioBlocks,(void*)mifuncion );
	pthread_mutex_unlock(&diccionarioBitacorasSem);

	return retorno != NULL;
}
void ponerEnListaTareas(t_list* listaTareas,char* tarea ){
	pthread_mutex_lock(&listaTareasSem);
	list_add(listaTareas, tarea);
	pthread_mutex_unlock(&listaTareasSem);
}

void esperarConexion(){
	int socket_servidor = iniciar_servidor();
	log_info(logger, "Servidor listo para recibir mensajes \n");

	while(1)
		{
			int socket_cliente = esperar_cliente(socket_servidor);
			pthread_t nueva_conexion;
			pthread_create(&nueva_conexion,NULL, (void*)atenderCliente, (void*)socket_cliente);
			pthread_detach(nueva_conexion);
		}
}
