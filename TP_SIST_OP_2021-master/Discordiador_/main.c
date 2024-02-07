/*
 * main.c
 *
 *  Created on: May 22, 2021
 *      Author: utnso
 */

#include "main.h"


int main(void)
{
	idsPatota = 1;
	idsTripulante = 1;
	haySabotaje = false;
	planificacionActivada = false;

	tareasDeIO = "GENERAR_OXIGENO CONSUMIR_OXIGENO GENERAR_COMIDA CONSUMIR_COMIDA GENERAR_BASURA DESCARTAR_BASURA";

	logger = iniciar_logger();
	config = iniciar_config();
	leer_config();
	inicializar_estados();
	crear_listas_globales();
	inicializar_semaforos();
	log_info(logger, "COMENZO LA EJECUCION DEL DISCORDIADOR");
	crear_planificacion();

	log_info(logger, "El grado de multitarea inicial es de %d", copiaDeGradoMultitarea);

	// crear conexion con i-Mongo para escuchar a ver si hay sabotajes
	esperarARecibirSabotajes();

	//pthread_create(&consola, NULL, (void*)leer_consola, NULL);
	//pthread_join(consola, NULL);

	leer_consola();

	/*//T O D O LO RELACIONADO CON ENVIAR MENSAJE, PAQUETE Y LIBERAR CONEXION (desplegar para ver)
	antes de continuar, tenemos que asegurarnos que el servidor esté corriendo porque lo necesitaremos para lo que sigue.

	enviar al servidor
	enviar_mensaje("hola ram",conexionRAM);
	enviar_mensaje("hola mongo",conexionImongo);

	paquete(conexionRAM, tripulante, logger);
	paquete(conexionImongo, tripulante, logger);


	liberar_conexion(conexionRAM);
	liberar_conexion(conexionImongo);
	*/

	terminar_programa();
}

t_log* iniciar_logger(void)
{
	return log_create("main.log", "MAIN", 0, LOG_LEVEL_DEBUG);//con 0 lo guarda solo en el archivo y con 1 lo guarda en la consola tmb
}

t_config* iniciar_config(void)
{
	return config_create("main.config");
}

void terminar_programa()
{
	//destroy semaforos
	log_destroy(logger);
	config_destroy(config);
}
