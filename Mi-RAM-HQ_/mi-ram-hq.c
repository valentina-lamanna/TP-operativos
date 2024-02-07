/*
 * mi-ram-hq.c
 *
 *  Created on: May 22, 2021
 *      Author: utnso
 */

#include "mi-ram-hq.h"

int main(void)
{
		logger = log_create("mi-ram-hq.log", "MiRamHQ", 0, LOG_LEVEL_DEBUG);
		config = iniciar_config();
		leerConfig();
		memoria = malloc(tamMemoria);
		inicializar_semaforos();
		memoriaListSeg = list_create(); //capaz esto no hbria que hacerlo en inicializador.c ????
		memoriaListPag = list_create();
		listaPatotas = list_create();
		listaFramesTodos = list_create();
		listaPaginasGeneral = list_create();
		listaPagsVirtual = list_create();
		listaInfoPaginasPatota = list_create();

		//-- POR CONSIGNA --//
		tamTCB = 21;
		tamPCB = 8;
		//-- POR CONSIGNA --//

		inicializarMemoriaSegunEsquema();

		esperarSenialParaCompactar(); // me quiero quedar siempre escuchando a ver si tiran la senial de compactar
		esperarSenialParaDump();

		dibujarMapa();
		esperarConexion();

	return EXIT_SUCCESS;
}

t_config* iniciar_config(void)
{
	return config_create("mi-ram-hq.config");
}

void leerConfig(void){
	tamMemoria = config_get_int_value(config,"TAMANIO_MEMORIA");
	esquemaMemoria = strdup(config_get_string_value(config,"ESQUEMA_MEMORIA"));
	tamPagina = config_get_int_value(config,"TAMANIO_PAGINA");
	tamSwap = config_get_int_value(config,"TAMANIO_SWAP");
	pathSwap = strdup(config_get_string_value(config,"PATH_SWAP"));
	algoritmoReemplazo = strdup(config_get_string_value(config,"ALGORITMO_REEMPLAZO"));
	criterioSeleccion = strdup(config_get_string_value(config,"CRITERIO_SELECCION")); // FF first fit BF best fit
	IP = strdup(config_get_string_value(config,"IP"));
	puerto= strdup(config_get_string_value(config,"PUERTO"));
}


void inicializarMemoriaSegunEsquema(void){
	if(strcmp(esquemaMemoria,"PAGINACION") == 0)
	{
		uint32_t cantFrames = tamMemoria/tamPagina;
		uint32_t tamMarco = tamPagina;

		for (int i = 0; i < cantFrames; i++)
		{
			Frame* marco = malloc(sizeof(Frame));
			marco->nroFrame = i;
			marco->direccionFisica = i * tamMarco;
			marco->ocupado = 0;
			marco->PID = 0;
			list_add(listaFramesTodos, marco);
		}

		punteroParaClock = 0; // empieza en el frame 0

		iniciarMemoriaVirtual();
	}
	else if(strcmp(esquemaMemoria,"SEGMENTACION") == 0)
	{
		//inicio con un solo hueco del tamaño de la memoria
		Segmento *segmentoUnicoInicial = malloc(sizeof(Segmento));
		segmentoUnicoInicial->base = 0;
		segmentoUnicoInicial->tamanio = tamMemoria; //en lugar de tamMemoria-1 deberia ser list size de nuestra estructura administrativa ???
		segmentoUnicoInicial->ocupado = 0;
		segmentoUnicoInicial->PID = 0;
		agregarSegmento(segmentoUnicoInicial);
	}

	log_info(logger, "Se inicializo la memoria con %s", esquemaMemoria);
}
